# 📦 WebRTC Server with Web Browser Client - Complete Package

## ✅ What You Have

A complete C++ WebRTC streaming server with:
- ✅ **Uses bengreenier/webrtc** prebuilt libraries (no 6-hour compilation!)
- ✅ **Web browser client** with real-time statistics
- ✅ **STUN support** for NAT traversal
- ✅ **Bitrate measurement** in the browser
- ✅ **Ready to build and run**

## 📁 Files Included

### Core Server Files:
- `webrtc_server.cpp` - Main server application
- `video_source.h/cpp` - Generates test video frames
- `throughput_receiver.h/cpp` - Measures throughput
- `signaling_server.h/cpp` - HTTP/WebSocket server
- `peer_connection_handler.h/cpp` - WebRTC peer connection logic

### Client:
- `client.html` - Beautiful web client with real-time stats

### Build Files:
- `CMakeLists_webclient.txt` - CMake configuration (rename to CMakeLists.txt)
- `build.bat` - Windows quick build script

### Documentation:
- `README_WEB_CLIENT.md` - Complete guide with troubleshooting

## 🚀 Quick Start (3 Steps!)

### Step 1: Get bengreenier/webrtc

Download from: https://github.com/bengreenier/webrtc/releases/latest

Extract to: `C:\webrtc-prebuilt\`

### Step 2: Build

```cmd
# Option A: Use build script
build.bat

# Option B: Manual
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DWEBRTC_ROOT=C:/webrtc-prebuilt
cmake --build . --config Release
```

### Step 3: Run

```cmd
cd bin\Release
webrtc_server.exe
```

Then open: **http://localhost:8080/**

## 📊 What You'll See

### In Terminal:
```
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

### In Browser:
- 🎥 Live video stream (1920x1080 @ 30fps)
- 📈 Real-time bitrate graph
- 📊 FPS, resolution, jitter, packet loss
- 🟢 Connection status indicators

## 🎯 Key Features

### STUN Server Support
```cpp
// In peer_connection_handler.cpp
webrtc::PeerConnectionInterface::IceServer stun_server;
stun_server.uri = "stun:stun.l.google.com:19302";
config.servers.push_back(stun_server);
```

This enables:
- ✅ NAT traversal
- ✅ Connection through firewalls
- ✅ Real-world internet streaming

### Real-Time Statistics
The browser client measures:
- **Bitrate** - Actual data transmission rate (Mbps)
- **FPS** - Frames per second
- **Resolution** - Video dimensions
- **Packet Loss** - Network quality indicator
- **Jitter** - Connection stability metric
- **Total Data** - Cumulative data received

### Thread Management
Proper WebRTC thread handling:
```cpp
rtc::scoped_refptr<rtc::Thread> network_thread_;
rtc::scoped_refptr<rtc::Thread> worker_thread_;
rtc::scoped_refptr<rtc::Thread> signaling_thread_;
```

## ⚙️ Configuration

### Change Video Settings
Edit `webrtc_server.cpp`:
```cpp
const int WIDTH = 1920;   // Resolution width
const int HEIGHT = 1080;  // Resolution height
const int FPS = 30;       // Frames per second
const int WS_PORT = 8080; // Server port
```

### Add More STUN Servers
Edit `peer_connection_handler.cpp` to add redundancy:
```cpp
webrtc::PeerConnectionInterface::IceServer stun_server2;
stun_server2.uri = "stun:stun2.l.google.com:19302";
config.servers.push_back(stun_server2);
```

### Add TURN Server
For strict NAT environments:
```cpp
webrtc::PeerConnectionInterface::IceServer turn_server;
turn_server.uri = "turn:your-turn-server.com:3478";
turn_server.username = "username";
turn_server.password = "password";
config.servers.push_back(turn_server);
```

## 📈 Expected Performance

| Scenario | Bitrate | Latency | Packet Loss |
|----------|---------|---------|-------------|
| Local Network | 4-8 Mbps | <10ms | 0% |
| Same City Internet | 2-6 Mbps | 20-50ms | <0.1% |
| Cross-Country | 1-4 Mbps | 50-150ms | 0-1% |

## 🔧 Troubleshooting

### Build Error: "Cannot find webrtc.lib"
**Solution**: Check `WEBRTC_ROOT` path in CMakeLists.txt or build.bat

### Runtime Error: "Failed to create peer connection"
**Solution**: Make sure all WebRTC DLLs are in the same folder as .exe

### Browser: "WebSocket connection failed"
**Solution**: 
1. Server must be running first
2. Check firewall isn't blocking port 8080
3. Try `http://localhost:8080` instead of `127.0.0.1`

### Browser: "Connection state: failed"
**Solution**: STUN servers may be blocked. Try:
1. Different STUN server
2. Add TURN server
3. Check corporate firewall settings

### Video Shows But Stats Are 0
**Solution**: 
1. Open browser console (F12)
2. Check for JavaScript errors
3. Verify WebRTC stats API is supported

## 🎨 Customization Ideas

### For Your NVR System:
1. Replace `TestVideoSource` with actual camera feeds
2. Add H.264 encoding
3. Implement multi-camera support
4. Add recording capability
5. Motion detection integration

### Enhance the Client:
1. Add DVR controls (pause, rewind)
2. Multi-camera grid view
3. PTZ camera controls
4. Motion detection alerts
5. Recording playback

## 📚 Next Steps

1. ✅ Build and test locally
2. ✅ Test over local network
3. ✅ Test over internet (STUN)
4. 🔄 Integrate with your camera sources
5. 🔄 Add H.264 encoding
6. 🔄 Deploy to production

## 💡 Pro Tips

1. **Test locally first** - Easier to debug
2. **Monitor CPU usage** - Adjust resolution/fps if needed
3. **Use Chrome DevTools** - Network tab shows actual bandwidth
4. **Check WebRTC internals** - chrome://webrtc-internals for detailed stats
5. **TURN server** - Essential for production use

## 📞 Support

If you have issues:
1. Check README_WEB_CLIENT.md for detailed docs
2. Enable verbose logging in the code
3. Check browser console for errors
4. Verify bengreenier/webrtc version matches

## 🎓 Learning Resources

- **WebRTC Official**: https://webrtc.org/
- **bengreenier/webrtc**: https://github.com/bengreenier/webrtc
- **MDN WebRTC Guide**: https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API
- **WebRTC Stats**: https://www.w3.org/TR/webrtc-stats/

---

**You're all set!** 🎉

Run `build.bat` and then `webrtc_server.exe` to get started.

Questions? Check README_WEB_CLIENT.md for comprehensive documentation.
