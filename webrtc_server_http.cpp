// webrtc_server_http.cpp
// C++ WebRTC server with simple HTTP signaling (no WebSocket needed!)

#include "video_source.h"
#include "peer_connection_handler.h"
#include "simple_video_factories.h"

#include <api/create_peerconnection_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <rtc_base/logging.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

// Global state
std::atomic<bool> g_running(true);
std::shared_ptr<TestVideoSource> g_video_source;
std::shared_ptr<PeerConnectionHandler> g_peer_handler;
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> g_factory;

// Threads
std::unique_ptr<rtc::Thread> g_network_thread;
std::unique_ptr<rtc::Thread> g_worker_thread;
std::unique_ptr<rtc::Thread> g_signaling_thread;

void SignalHandler(int signal) {
    std::cout << "\nShutdown signal received..." << std::endl;
    g_running = false;
}

std::string ExtractJsonField(const std::string& json, const std::string& field) {
    // Simple JSON parsing (replace with proper parser in production)
    std::string search = "\"" + field + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos += search.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end != std::string::npos) {
            return json.substr(pos, end - pos);
        }
    }
    
    return "";
}

std::string HandleSignalingMessage(const std::string& body) {
    std::string type = ExtractJsonField(body, "type");
    
    std::cout << "Received signaling message: " << type << std::endl;
    
    if (type == "offer") {
        std::string sdp = ExtractJsonField(body, "sdp");
        if (!sdp.empty() && g_peer_handler) {
            g_peer_handler->HandleOffer(sdp);
            // Response will be sent via the callback
            return "{\"type\":\"processing\"}";
        }
    }
    else if (type == "ice-candidate") {
        std::string candidate = ExtractJsonField(body, "candidate");
        std::string sdpMid = ExtractJsonField(body, "sdpMid");
        // Handle ICE candidate
        // g_peer_handler->HandleIceCandidate(candidate, sdpMid, 0);
        return "{\"type\":\"ok\"}";
    }
    
    return "{\"type\":\"error\",\"message\":\"Unknown message type\"}";
}

void RunHTTPServer(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port << std::endl;
#ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
        return;
    }
    
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Failed to listen" << std::endl;
#ifdef _WIN32
        closesocket(server_fd);
#else
        close(server_fd);
#endif
        return;
    }
    
    std::cout << "HTTP server listening on port " << port << std::endl;
    
    while (g_running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (g_running) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }
        
        // Read HTTP request
        char buffer[8192];
#ifdef _WIN32
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
#else
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
#endif
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);
            
            // Extract body from POST request
            size_t body_pos = request.find("\r\n\r\n");
            std::string response;
            
            if (body_pos != std::string::npos && request.find("POST /signaling") == 0) {
                std::string body = request.substr(body_pos + 4);
                std::string result = HandleSignalingMessage(body);
                
                std::ostringstream oss;
                oss << "HTTP/1.1 200 OK\r\n";
                oss << "Content-Type: application/json\r\n";
                oss << "Access-Control-Allow-Origin: *\r\n";
                oss << "Content-Length: " << result.length() << "\r\n";
                oss << "\r\n";
                oss << result;
                
                response = oss.str();
            }
            else if (request.find("OPTIONS") == 0) {
                // Handle CORS preflight
                std::ostringstream oss;
                oss << "HTTP/1.1 200 OK\r\n";
                oss << "Access-Control-Allow-Origin: *\r\n";
                oss << "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n";
                oss << "Access-Control-Allow-Headers: Content-Type\r\n";
                oss << "Content-Length: 0\r\n";
                oss << "\r\n";
                
                response = oss.str();
            }
            else {
                response = "HTTP/1.1 404 Not Found\r\n\r\n";
            }
            
#ifdef _WIN32
            send(client_fd, response.c_str(), response.length(), 0);
            closesocket(client_fd);
#else
            write(client_fd, response.c_str(), response.length());
            close(client_fd);
#endif
        }
    }
    
#ifdef _WIN32
    closesocket(server_fd);
#else
    close(server_fd);
#endif
}

int main(int argc, char* argv[]) {
    // Configuration
    const int WIDTH = 1920;
    const int HEIGHT = 1080;
    const int FPS = 30;
    const int HTTP_PORT = 9090;
    
    std::cout << "========================================\n";
    std::cout << "WebRTC C++ Server with libwebrtc + STUN\n";
    std::cout << "========================================\n";
    std::cout << "Video: " << WIDTH << "x" << HEIGHT << " @ " << FPS << " FPS\n";
    std::cout << "HTTP Port: " << HTTP_PORT << "\n";
    std::cout << "STUN Server: stun.l.google.com:19302\n";
    std::cout << "========================================\n\n";

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    try {
        // Initialize WebRTC threads
        g_network_thread = rtc::Thread::CreateWithSocketServer();
        g_network_thread->SetName("network", nullptr);
        g_network_thread->Start();
        
        g_worker_thread = rtc::Thread::Create();
        g_worker_thread->SetName("worker", nullptr);
        g_worker_thread->Start();
        
        g_signaling_thread = rtc::Thread::Create();
        g_signaling_thread->SetName("signaling", nullptr);
        g_signaling_thread->Start();
        
        std::cout << "WebRTC threads initialized\n";
        
        // Create peer connection factory with simple custom factories
        g_factory = webrtc::CreatePeerConnectionFactory(
            g_network_thread.get(),
            g_worker_thread.get(),
            g_signaling_thread.get(),
            nullptr,
            webrtc::CreateBuiltinAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            std::make_unique<webrtc::SimpleVideoEncoderFactory>(),
            std::make_unique<webrtc::SimpleVideoDecoderFactory>(),
            nullptr,
            nullptr);
        
        if (!g_factory) {
            std::cerr << "Failed to create peer connection factory" << std::endl;
            return 1;
        }
        
        std::cout << "Peer connection factory created\n";
        
        // Create video source
        g_video_source = std::make_shared<TestVideoSource>(WIDTH, HEIGHT, FPS);
        g_video_source->Start();
        std::cout << "Video source started\n\n";
        
        std::cout << "Server running!\n";
        std::cout << "Waiting for browser connections on port " << HTTP_PORT << "...\n\n";
        std::cout << "Press Ctrl+C to stop...\n\n";
        
        // Run HTTP server
        RunHTTPServer(HTTP_PORT);
        
        // Cleanup
        std::cout << "\nCleaning up...\n";
        g_peer_handler.reset();
        g_video_source->Stop();
        g_video_source.reset();
        g_factory = nullptr;
        
        g_signaling_thread->Stop();
        g_worker_thread->Stop();
        g_network_thread->Stop();
        
        g_signaling_thread.reset();
        g_worker_thread.reset();
        g_network_thread.reset();
        
        std::cout << "Total frames generated: " << g_video_source->GetFramesSent() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    std::cout << "Server stopped.\n";
    return 0;
}
