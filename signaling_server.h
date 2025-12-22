// signaling_server.h
// WebSocket-based signaling server for WebRTC

#ifndef SIGNALING_SERVER_H
#define SIGNALING_SERVER_H

#include "video_source.h"
#include "peer_connection_handler.h"

#include <memory>
#include <string>
#include <map>
#include <thread>

// Simple WebSocket server for signaling
class SignalingServer {
public:
    SignalingServer(int port, std::shared_ptr<TestVideoSource> video_source);
    ~SignalingServer();

    bool Start();
    void Stop();

private:
    void Run();
    void HandleClient(int client_socket);
    std::string GetHTMLPage();
    
    int port_;
    std::shared_ptr<TestVideoSource> video_source_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    int server_socket_;
    
    // Map of client connections to peer handlers
    std::map<int, std::shared_ptr<PeerConnectionHandler>> clients_;
};

#endif // SIGNALING_SERVER_H
