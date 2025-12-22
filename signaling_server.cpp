// signaling_server.cpp
// Simple HTTP and WebSocket server for WebRTC signaling

#include "signaling_server.h"
#include <rtc_base/logging.h>
#include <api/create_peerconnection_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <iostream>
#include <sstream>
#include <cstring>

SignalingServer::SignalingServer(int port, std::shared_ptr<TestVideoSource> video_source)
    : port_(port),
      video_source_(video_source),
      running_(false),
      server_socket_(-1) {
}

SignalingServer::~SignalingServer() {
    Stop();
}

bool SignalingServer::Start() {
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        RTC_LOG(LS_ERROR) << "Failed to create socket";
        return false;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    // Bind to port
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        RTC_LOG(LS_ERROR) << "Failed to bind to port " << port_;
#ifdef _WIN32
        closesocket(server_socket_);
#else
        close(server_socket_);
#endif
        return false;
    }
    
    // Listen for connections
    if (listen(server_socket_, 5) < 0) {
        RTC_LOG(LS_ERROR) << "Failed to listen";
#ifdef _WIN32
        closesocket(server_socket_);
#else
        close(server_socket_);
#endif
        return false;
    }
    
    running_ = true;
    server_thread_ = std::thread(&SignalingServer::Run, this);
    
    RTC_LOG(LS_INFO) << "Signaling server started on port " << port_;
    return true;
}

void SignalingServer::Stop() {
    if (running_) {
        running_ = false;
        
        if (server_socket_ >= 0) {
#ifdef _WIN32
            closesocket(server_socket_);
#else
            close(server_socket_);
#endif
            server_socket_ = -1;
        }
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        clients_.clear();
        RTC_LOG(LS_INFO) << "Signaling server stopped";
    }
}

void SignalingServer::Run() {
    while (running_) {
        sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &client_len);
        
        if (client_socket < 0) {
            if (running_) {
                RTC_LOG(LS_ERROR) << "Accept failed";
            }
            continue;
        }
        
        RTC_LOG(LS_INFO) << "Client connected";
        
        // Handle client in a separate thread
        std::thread client_thread([this, client_socket]() {
            HandleClient(client_socket);
        });
        client_thread.detach();
    }
}

void SignalingServer::HandleClient(int client_socket) {
    char buffer[4096];
    
    // Read HTTP request
#ifdef _WIN32
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#else
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
#endif
    
    if (bytes_read <= 0) {
#ifdef _WIN32
        closesocket(client_socket);
#else
        close(client_socket);
#endif
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::string request(buffer);
    
    // Simple HTTP response for GET requests (serve the HTML page)
    if (request.find("GET / HTTP") == 0) {
        std::string html = GetHTMLPage();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html\r\n";
        response << "Content-Length: " << html.length() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << html;
        
        std::string response_str = response.str();
#ifdef _WIN32
        send(client_socket, response_str.c_str(), response_str.length(), 0);
        closesocket(client_socket);
#else
        write(client_socket, response_str.c_str(), response_str.length());
        close(client_socket);
#endif
        return;
    }
    
    // For WebSocket signaling, we would implement WebSocket protocol here
    // For simplicity, this example uses HTTP polling or WebSocket library
    // In production, use a proper WebSocket library like libwebsockets or uWebSockets
    
#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}

std::string SignalingServer::GetHTMLPage() {
    return R"html(<!DOCTYPE html>
<html>
<head>
    <title>WebRTC Throughput Test</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1200px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background-color: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        #videoContainer {
            text-align: center;
            margin: 30px 0;
        }
        video {
            width: 100%;
            max-width: 960px;
            background-color: #000;
            border-radius: 8px;
        }
        .stats {
            background-color: #f9f9f9;
            padding: 20px;
            border-radius: 8px;
            margin: 20px 0;
        }
        .stats h2 {
            margin-top: 0;
            color: #555;
        }
        .stat-row {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid #eee;
        }
        .stat-label {
            font-weight: bold;
            color: #666;
        }
        .stat-value {
            color: #2196F3;
            font-family: monospace;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 12px 30px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            margin: 10px 5px;
        }
        button:hover {
            background-color: #45a049;
        }
        button:disabled {
            background-color: #ccc;
            cursor: not-allowed;
        }
        .status {
            text-align: center;
            padding: 15px;
            margin: 20px 0;
            border-radius: 4px;
            font-weight: bold;
        }
        .status.connecting {
            background-color: #fff3cd;
            color: #856404;
        }
        .status.connected {
            background-color: #d4edda;
            color: #155724;
        }
        .status.disconnected {
            background-color: #f8d7da;
            color: #721c24;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WebRTC Throughput Test with STUN</h1>
        
        <div id="status" class="status disconnected">Not Connected</div>
        
        <div style="text-align: center;">
            <button id="startBtn" onclick="start()">Start Stream</button>
            <button id="stopBtn" onclick="stop()" disabled>Stop Stream</button>
        </div>
        
        <div id="videoContainer">
            <video id="remoteVideo" autoplay playsinline></video>
        </div>
        
        <div class="stats">
            <h2>Connection Statistics</h2>
            <div class="stat-row">
                <span class="stat-label">Connection State:</span>
                <span id="connState" class="stat-value">-</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">ICE Connection State:</span>
                <span id="iceState" class="stat-value">-</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Video Resolution:</span>
                <span id="resolution" class="stat-value">-</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Frames Received:</span>
                <span id="framesReceived" class="stat-value">0</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Frames Decoded:</span>
                <span id="framesDecoded" class="stat-value">0</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Frames Per Second:</span>
                <span id="fps" class="stat-value">0</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Bitrate (Mbps):</span>
                <span id="bitrate" class="stat-value">0.00</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Bytes Received:</span>
                <span id="bytesReceived" class="stat-value">0</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Packets Lost:</span>
                <span id="packetsLost" class="stat-value">0</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Jitter:</span>
                <span id="jitter" class="stat-value">0</span>
            </div>
        </div>
    </div>

    <script>
        let pc = null;
        let statsInterval = null;
        let lastBytesReceived = 0;
        let lastTimestamp = 0;
        
        const config = {
            iceServers: [
                { urls: 'stun:stun.l.google.com:19302' },
                { urls: 'stun:stun1.l.google.com:19302' }
            ]
        };
        
        function updateStatus(text, className) {
            const status = document.getElementById('status');
            status.textContent = text;
            status.className = 'status ' + className;
        }
        
        async function start() {
            try {
                updateStatus('Connecting...', 'connecting');
                document.getElementById('startBtn').disabled = true;
                
                // Create peer connection
                pc = new RTCPeerConnection(config);
                
                // Handle incoming tracks
                pc.ontrack = (event) => {
                    console.log('Received track:', event.track.kind);
                    document.getElementById('remoteVideo').srcObject = event.streams[0];
                };
                
                // Monitor connection state
                pc.onconnectionstatechange = () => {
                    console.log('Connection state:', pc.connectionState);
                    document.getElementById('connState').textContent = pc.connectionState;
                    
                    if (pc.connectionState === 'connected') {
                        updateStatus('Connected', 'connected');
                        document.getElementById('stopBtn').disabled = false;
                        startStats();
                    } else if (pc.connectionState === 'disconnected' || pc.connectionState === 'failed') {
                        updateStatus('Disconnected', 'disconnected');
                        stop();
                    }
                };
                
                // Monitor ICE connection state
                pc.oniceconnectionstatechange = () => {
                    console.log('ICE connection state:', pc.iceConnectionState);
                    document.getElementById('iceState').textContent = pc.iceConnectionState;
                };
                
                // Handle ICE candidates
                pc.onicecandidate = (event) => {
                    if (event.candidate) {
                        console.log('New ICE candidate:', event.candidate.candidate);
                        // In a real implementation, send this to the server
                        sendToServer('ice-candidate', {
                            candidate: event.candidate.candidate,
                            sdpMid: event.candidate.sdpMid,
                            sdpMLineIndex: event.candidate.sdpMLineIndex
                        });
                    }
                };
                
                // Create offer
                const offer = await pc.createOffer({
                    offerToReceiveVideo: true,
                    offerToReceiveAudio: false
                });
                
                await pc.setLocalDescription(offer);
                console.log('Created offer');
                
                // Send offer to server and get answer
                const answer = await sendOfferToServer(offer.sdp);
                await pc.setRemoteDescription({
                    type: 'answer',
                    sdp: answer
                });
                
                console.log('Set remote description');
                
            } catch (error) {
                console.error('Error starting stream:', error);
                updateStatus('Error: ' + error.message, 'disconnected');
                document.getElementById('startBtn').disabled = false;
            }
        }
        
        function stop() {
            if (statsInterval) {
                clearInterval(statsInterval);
                statsInterval = null;
            }
            
            if (pc) {
                pc.close();
                pc = null;
            }
            
            document.getElementById('remoteVideo').srcObject = null;
            document.getElementById('startBtn').disabled = false;
            document.getElementById('stopBtn').disabled = true;
            updateStatus('Disconnected', 'disconnected');
        }
        
        function startStats() {
            statsInterval = setInterval(async () => {
                if (!pc) return;
                
                try {
                    const stats = await pc.getStats();
                    stats.forEach(report => {
                        if (report.type === 'inbound-rtp' && report.kind === 'video') {
                            // Update stats
                            document.getElementById('framesReceived').textContent = report.framesReceived || 0;
                            document.getElementById('framesDecoded').textContent = report.framesDecoded || 0;
                            document.getElementById('packetsLost').textContent = report.packetsLost || 0;
                            document.getElementById('jitter').textContent = (report.jitter || 0).toFixed(4);
                            
                            // Calculate FPS
                            if (report.framesDecoded && report.timestamp) {
                                const fps = report.framesPerSecond || 0;
                                document.getElementById('fps').textContent = fps.toFixed(2);
                            }
                            
                            // Calculate bitrate
                            if (lastBytesReceived && lastTimestamp) {
                                const bytes = (report.bytesReceived || 0) - lastBytesReceived;
                                const time = (report.timestamp - lastTimestamp) / 1000; // convert to seconds
                                const bitrate = (bytes * 8) / (time * 1000000); // convert to Mbps
                                document.getElementById('bitrate').textContent = bitrate.toFixed(2);
                            }
                            
                            lastBytesReceived = report.bytesReceived || 0;
                            lastTimestamp = report.timestamp;
                            
                            const mb = (report.bytesReceived / (1024 * 1024)).toFixed(2);
                            document.getElementById('bytesReceived').textContent = mb + ' MB';
                            
                            // Get resolution from video element
                            const video = document.getElementById('remoteVideo');
                            if (video.videoWidth && video.videoHeight) {
                                document.getElementById('resolution').textContent = 
                                    video.videoWidth + 'x' + video.videoHeight;
                            }
                        }
                    });
                } catch (error) {
                    console.error('Error getting stats:', error);
                }
            }, 1000);
        }
        
        // Simulated server communication (replace with actual WebSocket or HTTP calls)
        async function sendOfferToServer(sdp) {
            // TODO: Implement actual signaling with your C++ server
            // For now, return a dummy answer
            console.log('Would send offer to server:', sdp);
            
            // This should be replaced with actual server communication
            throw new Error('Server signaling not implemented - use WebSocket library');
        }
        
        function sendToServer(type, data) {
            // TODO: Implement actual signaling
            console.log('Would send to server:', type, data);
        }
    </script>
</body>
</html>
)html";
}
