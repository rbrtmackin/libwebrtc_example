// webrtc_server.cpp
// WebRTC server that streams video to web browser clients via WebSocket signaling

#include "video_source.h"
#include "signaling_server.h"
#include "peer_connection_handler.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Global flag for clean shutdown
std::atomic<bool> g_running(true);

void SignalHandler(int signal) {
    std::cout << "\nShutdown signal received..." << std::endl;
    g_running = false;
}

int main(int argc, char* argv[]) {
    // Configuration
    const int WIDTH = 1920;
    const int HEIGHT = 1080;
    const int FPS = 30;
    const int WS_PORT = 8080;  // WebSocket signaling port
    
    std::cout << "========================================\n";
    std::cout << "WebRTC Server with STUN Support\n";
    std::cout << "========================================\n";
    std::cout << "Video: " << WIDTH << "x" << HEIGHT << " @ " << FPS << " FPS\n";
    std::cout << "WebSocket Port: " << WS_PORT << "\n";
    std::cout << "STUN Server: stun:stun.l.google.com:19302\n";
    std::cout << "========================================\n\n";

#ifdef _WIN32
    // Initialize Winsock on Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    // Setup signal handler
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    try {
        // Create video source
        auto video_source = std::make_shared<TestVideoSource>(WIDTH, HEIGHT, FPS);
        video_source->Start();
        std::cout << "Video source started\n";

        // Create signaling server
        SignalingServer signaling_server(WS_PORT, video_source);
        
        if (!signaling_server.Start()) {
            std::cerr << "Failed to start signaling server" << std::endl;
            return 1;
        }
        
        std::cout << "\nServer running!\n";
        std::cout << "Open this URL in your browser:\n";
        std::cout << "  http://localhost:" << WS_PORT << "/\n\n";
        std::cout << "Press Ctrl+C to stop...\n\n";

        // Main loop
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Cleanup
        std::cout << "\nCleaning up...\n";
        signaling_server.Stop();
        video_source->Stop();
        
        std::cout << "Total frames generated: " << video_source->GetFramesSent() << "\n";

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
