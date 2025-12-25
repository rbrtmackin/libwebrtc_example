// webrtc_server_http.cpp
// C++ WebRTC server with simple HTTP signaling (no WebSocket needed!)

#include "encoded_video_source.h"
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
#include <mutex>
#include <map>

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
std::shared_ptr<EncodedVideoSource> g_video_source;
std::map<std::string, std::shared_ptr<PeerConnectionHandler>> g_peer_handlers;  // Support multiple clients
std::mutex g_peers_mutex;
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
    // Simple JSON parsing - look for "field":"value" or "field":value
    std::string search = "\"" + field + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos += search.length();
    
    // Skip whitespace and colon
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == ':')) pos++;
    
    if (pos >= json.length()) return "";
    
    // Check if it's a string (starts with quote)
    if (json[pos] == '"') {
        pos++; // Skip opening quote
        size_t end = pos;
        
        // Find closing quote, handling escaped quotes
        while (end < json.length()) {
            if (json[end] == '"' && (end == pos || json[end-1] != '\\')) {
                break;
            }
            end++;
        }
        
        if (end < json.length()) {
            std::string result = json.substr(pos, end - pos);
            
            // Unescape common sequences
            size_t escPos = 0;
            while ((escPos = result.find("\\n", escPos)) != std::string::npos) {
                result.replace(escPos, 2, "\n");
                escPos++;
            }
            escPos = 0;
            while ((escPos = result.find("\\r", escPos)) != std::string::npos) {
                result.replace(escPos, 2, "\r");
                escPos++;
            }
            escPos = 0;
            while ((escPos = result.find("\\\"", escPos)) != std::string::npos) {
                result.replace(escPos, 2, "\"");
                escPos++;
            }
            escPos = 0;
            while ((escPos = result.find("\\\\", escPos)) != std::string::npos) {
                result.replace(escPos, 2, "\\");
                escPos++;
            }
            
            return result;
        }
    }
    // Check if it's a number
    else if (isdigit(json[pos]) || json[pos] == '-') {
        size_t end = pos;
        while (end < json.length() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) {
            end++;
        }
        return json.substr(pos, end - pos);
    }
    
    return "";
}

std::string EscapeJson(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

std::string g_pending_answer;  // Store answer to send back
std::mutex g_answer_mutex;

std::string HandleSignalingMessage(const std::string& body) {
    std::string type = ExtractJsonField(body, "type");
    std::string sessionId = ExtractJsonField(body, "sessionId");
    
    // Generate session ID if not provided
    if (sessionId.empty()) {
        sessionId = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }
    
    std::cout << "Session [" << sessionId << "] - Message: " << type << std::endl;
    std::cout << "Body length: " << body.length() << " bytes" << std::endl;
    
    if (type == "offer") {
        std::string sdp = ExtractJsonField(body, "sdp");
        std::cout << "Extracted SDP length: " << sdp.length() << " bytes" << std::endl;
        
        if (sdp.length() > 100) {
            std::cout << "SDP preview: " << sdp.substr(0, 100) << "..." << std::endl;
        }
        
        if (!sdp.empty()) {
            std::lock_guard<std::mutex> lock(g_peers_mutex);
            
            // Create peer handler for this client
            if (g_peer_handlers.find(sessionId) == g_peer_handlers.end() && g_factory && g_video_source) {
                std::cout << "Creating peer connection handler for session " << sessionId << "..." << std::endl;
                
                auto callback = [sessionId](const std::string& msg_type, const std::string& message) {
                    std::cout << "Session [" << sessionId << "] callback: " << msg_type << std::endl;
                    
                    // Store the answer to send back
                    if (msg_type == "answer") {
                        std::lock_guard<std::mutex> lock(g_answer_mutex);
                        std::string escaped_sdp = EscapeJson(message);
                        g_pending_answer = "{\"type\":\"answer\",\"sdp\":\"" + escaped_sdp + "\",\"sessionId\":\"" + sessionId + "\"}";
                        std::cout << "Answer ready for session " << sessionId << std::endl;
                    }
                };
                
                g_peer_handlers[sessionId] = std::make_shared<PeerConnectionHandler>(
                    g_factory,
                    g_video_source,
                    callback
                );
                
                std::cout << "Peer connection handler created. Total clients: " << g_peer_handlers.size() << std::endl;
            }
            
            // Handle the offer
            auto it = g_peer_handlers.find(sessionId);
            if (it != g_peer_handlers.end()) {
                std::cout << "Processing offer for session " << sessionId << "..." << std::endl;
                it->second->HandleOffer(sdp);
                
                // Wait a bit for answer to be generated
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // Return the answer
                std::lock_guard<std::mutex> lock(g_answer_mutex);
                if (!g_pending_answer.empty()) {
                    std::string answer = g_pending_answer;
                    g_pending_answer.clear();
                    std::cout << "Sending answer back to browser for session " << sessionId << std::endl;
                    return answer;
                }
            }
        } else {
            std::cout << "ERROR: SDP is empty!" << std::endl;
        }
        return "{\"type\":\"error\",\"message\":\"Failed to process offer\",\"sessionId\":\"" + sessionId + "\"}";
    }
    else if (type == "ice-candidate") {
        std::string candidate = ExtractJsonField(body, "candidate");
        std::string sdpMid = ExtractJsonField(body, "sdpMid");
        
        // Extract sdpMLineIndex
        std::string mlineStr = ExtractJsonField(body, "sdpMLineIndex");
        int sdpMLineIndex = 0;
        if (!mlineStr.empty()) {
            sdpMLineIndex = std::stoi(mlineStr);
        }
        
        std::lock_guard<std::mutex> lock(g_peers_mutex);
        auto it = g_peer_handlers.find(sessionId);
        if (it != g_peer_handlers.end() && !candidate.empty()) {
            std::cout << "Adding ICE candidate for session " << sessionId << std::endl;
            it->second->HandleIceCandidate(candidate, sdpMid, sdpMLineIndex);
        }
        
        return "{\"type\":\"ok\",\"sessionId\":\"" + sessionId + "\"}";
    }
    else if (type == "close") {
        std::lock_guard<std::mutex> lock(g_peers_mutex);
        auto it = g_peer_handlers.find(sessionId);
        if (it != g_peer_handlers.end()) {
            std::cout << "Closing session " << sessionId << std::endl;
            g_peer_handlers.erase(it);
            std::cout << "Session closed. Remaining clients: " << g_peer_handlers.size() << std::endl;
        }
        return "{\"type\":\"ok\",\"sessionId\":\"" + sessionId + "\"}";
    }
    
    std::cout << "Unknown message type: " << type << std::endl;
    return "{\"type\":\"error\",\"message\":\"Unknown message type\",\"sessionId\":\"" + sessionId + "\"}";
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
    // Default configuration - can be overridden with command line args
    int WIDTH = 3840;   // 4K width
    int HEIGHT = 2160;  // 4K height  
    int FPS = 60;       // 60 FPS
    const int HTTP_PORT = 9090;
    
    // Parse command line arguments
    // Usage: webrtc_server.exe [width] [height] [fps]
    // Example: webrtc_server.exe 1920 1080 30
    // Example: webrtc_server.exe 3840 2160 60
    if (argc >= 4) {
        WIDTH = std::atoi(argv[1]);
        HEIGHT = std::atoi(argv[2]);
        FPS = std::atoi(argv[3]);
    } else if (argc >= 2) {
        std::cout << "Usage: " << argv[0] << " [width] [height] [fps]\n";
        std::cout << "Example: " << argv[0] << " 1920 1080 30\n";
        std::cout << "Example: " << argv[0] << " 3840 2160 60\n";
        std::cout << "Using defaults...\n\n";
    }
    
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
        
        // Create encoded video source (reuses same frame data - MUCH more efficient!)
        g_video_source = std::make_shared<EncodedVideoSource>(WIDTH, HEIGHT, FPS, 30);  // GOP size = 30
        g_video_source->Start();
        std::cout << "Encoded video source started (ZERO-COPY MODE)\n";
        std::cout << "Using same frame buffer repeatedly - encoder optimized\n\n";
        
        std::cout << "Server running!\n";
        std::cout << "Waiting for browser connections on port " << HTTP_PORT << "...\n\n";
        std::cout << "Press Ctrl+C to stop...\n\n";
        
        // Start stats reporting thread
        std::thread stats_thread([&]() {
            while (g_running) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                
                std::lock_guard<std::mutex> lock(g_peers_mutex);
                if (!g_peer_handlers.empty()) {
                    std::cout << "\n========== SERVER STATS ==========\n";
                    std::cout << "Active Clients: " << g_peer_handlers.size() << "\n";
                    std::cout << "Video Source: " << WIDTH << "x" << HEIGHT << " @ " << FPS << " FPS\n";
                    std::cout << "Frames Generated: " << g_video_source->GetFramesSent() << "\n";
                    std::cout << "Expected Bitrate Per Client: ~" 
                              << (WIDTH * HEIGHT * FPS * 0.1 / 1000000) << " Mbps\n";
                    std::cout << "Expected Aggregate Bitrate: ~" 
                              << (WIDTH * HEIGHT * FPS * 0.1 / 1000000 * g_peer_handlers.size()) << " Mbps\n";
                    std::cout << "==================================\n\n";
                }
            }
        });
        
        // Run HTTP server
        RunHTTPServer(HTTP_PORT);
        
        // Wait for stats thread
        g_running = false;
        if (stats_thread.joinable()) {
            stats_thread.join();
        }
        
        // Cleanup
        std::cout << "\nCleaning up...\n";
        {
            std::lock_guard<std::mutex> lock(g_peers_mutex);
            g_peer_handlers.clear();
        }
        
        int total_frames = g_video_source->GetFramesSent();
        g_video_source->Stop();
        g_video_source.reset();
        g_factory = nullptr;
        
        g_signaling_thread->Stop();
        g_worker_thread->Stop();
        g_network_thread->Stop();
        
        g_signaling_thread.reset();
        g_worker_thread.reset();
        g_network_thread.reset();
        
        std::cout << "Total frames generated: " << total_frames << "\n";

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
