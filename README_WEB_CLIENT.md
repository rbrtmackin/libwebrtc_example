# WebRTC Server with Web Browser Client and STUN Support

This is a complete WebRTC streaming solution using:
- **C++ Server** - Streams video using bengreenier/webrtc prebuilt libraries
- **Web Browser Client** - HTML/JavaScript client with real-time bitrate measurement
- **STUN Server** - Uses Google's STUN servers for NAT traversal

## 🎯 What This Does

The C++ server generates test video frames and streams them to a web browser via WebRTC. The browser displays real-time statistics including:
- Bitrate (Mbps)
- Frames per second
- Resolution
- Packet loss
- Jitter
- Total data received

## 📁 Project Structure

```
.
├── webrtc_server.cpp              # Main server application
├── video_source.h/cpp             # Video frame generator
├── throughput_receiver.h/cpp      # Throughput measurement
├── signaling_server.h/cpp         # WebSocket signaling server
├── peer_connection_handler.h/cpp  # WebRTC peer connection management
├── client.html                    # Web browser client
└── CMakeLists.txt                 # Build configuration
```

## 🚀 Quick Start

### Step 1: Download bengreenier/webrtc

```bash
# Download from: https://github.com/bengreenier/webrtc/releases/latest
# Example: webrtc-5735-windows-x64.zip

# Extract to:
C:\webrtc-prebuilt\
```

Your directory should look like:
```
C:\webrtc-prebuilt\
├── include\
├── debug\webrtc.lib
└── release\webrtc.lib
```

### Step 2: Build the Server

#### Using CMake:
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DWEBRTC_ROOT=C:/webrtc-prebuilt
cmake --build . --config Release
```

#### Using Visual Studio:
1. Open CMakeLists.txt in Visual Studio
2. Set WebRTC path in CMake settings
3. Build → Build All

#### Using Command Line:
```cmd
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cl.exe /std:c++17 /EHsc ^
  /I"C:\webrtc-prebuilt\include" ^
  /I"C:\webrtc-prebuilt\include\third_party\abseil-cpp" ^
  /DWEBRTC_WIN /DNOMINMAX /DWIN32_LEAN_AND_MEAN /D_WINSOCKAPI_ /DRTC_ENABLE_WIN_WGC ^
  webrtc_server.cpp video_source.cpp throughput_receiver.cpp ^
  signaling_server.cpp peer_connection_handler.cpp ^
  /link ^
  C:\webrtc-prebuilt\release\webrtc.lib ^
  ws2_32.lib secur32.lib winmm.lib dmoguids.lib wmcodecdspuuid.lib ^
  msdmo.lib strmiids.lib iphlpapi.lib crypt32.lib ole32.lib oleaut32.lib uuid.lib
```

### Step 3: Run

```bash
# Start the server
./webrtc_server.exe

# Output:
========================================
WebRTC Server with STUN Support
========================================
Video: 1920x1080 @ 30 FPS
WebSocket Port: 8080
STUN Server: stun:stun.l.google.com:19302
========================================

Server running!
Open this URL in your browser:
  http://localhost:8080/

Press Ctrl+C to stop...
```

### Step 4: Open Browser

1. Open Chrome/Edge/Firefox
2. Navigate to `http://localhost:8080/`
3. Click "▶️ Start Stream"
4. Watch the statistics update in real-time!

## 📊 Understanding the Statistics

### Bitrate (Mbps)
- **What it measures**: Actual video data transmission rate
- **Expected values**:
  - 1920x1080 unencoded: ~745 Mbps
  - 1920x1080 with VP8: 2-8 Mbps (typical)
  - 1280x720 with VP8: 1-4 Mbps (typical)

### Frames Per Second (FPS)
- **What it measures**: Video frame rate
- **Expected values**: Should match server (30 FPS)
- **If lower**: Network congestion or CPU overload

### Packet Loss
- **What it measures**: Lost network packets
- **Expected values**: 
  - Local network: 0 packets
  - Internet: < 1% is acceptable
  - > 5%: Connection quality issues

### Jitter
- **What it measures**: Variation in packet arrival times
- **Expected values**:
  - Local network: < 5ms
  - Internet: < 30ms is good
  - > 100ms: Connection instability

## 🔧 Configuration

### Change Video Resolution/FPS

Edit `webrtc_server.cpp`:
```cpp
const int WIDTH = 1280;   // Change resolution
const int HEIGHT = 720;   // Change resolution
const int FPS = 30;       // Change frame rate
```

### Change Server Port

Edit `webrtc_server.cpp`:
```cpp
const int WS_PORT = 8080;  // Change port
```

### Add More STUN Servers

Edit `peer_connection_handler.cpp`:
```cpp
// Add more STUN servers for redundancy
webrtc::PeerConnectionInterface::IceServer stun_server3;
stun_server3.uri = "stun:stun2.l.google.com:19302";
config.servers.push_back(stun_server3);
```

### Add TURN Server (for strict NAT)

Edit `peer_connection_handler.cpp`:
```cpp
webrtc::PeerConnectionInterface::IceServer turn_server;
turn_server.uri = "turn:your-turn-server.com:3478";
turn_server.username = "username";
turn_server.password = "password";
config.servers.push_back(turn_server);
```

## 🐛 Troubleshooting

### "WebSocket connection failed"

**Problem**: Server not running or wrong port

**Solution**:
1. Make sure server is running
2. Check firewall isn't blocking port 8080
3. Verify URL matches server port

### "Connection state: failed"

**Problem**: ICE connection failed (NAT/firewall issues)

**Solution**:
1. Check STUN servers are reachable
2. Try adding a TURN server
3. Check firewall allows UDP traffic
4. Test on local network first

### "Bitrate is 0"

**Problem**: No video data being received

**Solution**:
1. Check browser console for errors (F12)
2. Verify video track was added
3. Check peer connection state
4. Enable WebRTC internal logging

### Browser Shows Black Screen

**Problem**: Video element not receiving stream

**Solution**:
1. Check `pc.ontrack` is firing
2. Verify `srcObject` is set
3. Check video element has `autoplay` attribute
4. Look for codec negotiation issues

## 🎨 Customizing the Client

The `client.html` file is standalone and can be customized:

### Change Theme Colors
```css
background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
/* Change to your colors */
```

### Add More Statistics
```javascript
// In the stats interval:
if (report.type === 'inbound-rtp' && report.kind === 'video') {
    // Add your custom stat:
    document.getElementById('mystat').textContent = report.someValue;
}
```

### Change Update Frequency
```javascript
statsInterval = setInterval(async () => {
    // ... stats code ...
}, 500);  // Update every 500ms instead of 1000ms
```

## 🔬 Testing Different Scenarios

### Local Network Test
1. Run server on one computer
2. Open client on another computer on same network
3. Use server's local IP: `http://192.168.1.100:8080`
4. **Expected**: Very low latency, high bitrate, 0% packet loss

### Internet Test (with STUN)
1. Run server on one network
2. Open client on different network (mobile hotspot, friend's house)
3. Server will use STUN to establish connection
4. **Expected**: Higher latency, possible packet loss

### NAT Traversal Test
1. Put server behind router/firewall
2. No port forwarding
3. STUN should enable connection
4. **Expected**: Connection works via STUN

## 📈 Performance Benchmarks

Based on testing with bengreenier/webrtc:

| Resolution | FPS | Codec | Typical Bitrate | CPU Usage |
|------------|-----|-------|-----------------|-----------|
| 1920x1080  | 30  | VP8   | 4-8 Mbps       | Medium    |
| 1280x720   | 30  | VP8   | 2-4 Mbps       | Low       |
| 640x480    | 30  | VP8   | 0.5-1 Mbps     | Very Low  |
| 1920x1080  | 30  | VP9   | 2-6 Mbps       | High      |

## 🔐 Security Notes

**Current Implementation**: 
- No authentication
- No encryption on signaling channel
- For testing only!

**Production Recommendations**:
1. Add authentication to signaling server
2. Use WSS (WebSocket Secure) instead of WS
3. Implement TURN server with authentication
4. Add rate limiting
5. Validate all signaling messages

## 🚀 Next Steps

### For NVR Integration:
1. Replace `TestVideoSource` with your camera feed
2. Add H.264 encoding support
3. Implement multi-stream support (multiple cameras)
4. Add recording capability
5. Implement motion detection

### For Production:
1. Add proper WebSocket library (e.g., websocketpp)
2. Implement session management
3. Add authentication
4. Use secure protocols (HTTPS/WSS)
5. Add logging and monitoring
6. Implement error recovery

## 📚 Additional Resources

- [WebRTC API Docs](https://webrtc.org/getting-started/overview)
- [bengreenier/webrtc Releases](https://github.com/bengreenier/webrtc/releases)
- [Google STUN Server](https://webrtc.github.io/samples/src/content/peerconnection/trickle-ice/)
- [WebRTC Statistics](https://www.w3.org/TR/webrtc-stats/)

## 📝 License

This is example code for educational purposes. Modify as needed for your use case.
