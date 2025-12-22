# 🚀 WebRTC STUN Bitrate Test - Quick Reference

## ⚡ Super Quick Start

```bash
# 1. Get bengreenier/webrtc
https://github.com/bengreenier/webrtc/releases/latest
↓ Extract to: C:\webrtc-prebuilt\

# 2. Build
build.bat

# 3. Run
cd build\bin\Release
webrtc_server.exe

# 4. Test
Open: http://localhost:8080/
Click: "▶️ Start Stream"
```

## 📦 Required Files (14 total)

### Server Code (9 files):
- webrtc_server.cpp
- video_source.{h,cpp}
- throughput_receiver.{h,cpp}
- signaling_server.{h,cpp}
- peer_connection_handler.{h,cpp}

### Client (1 file):
- client.html

### Build (2 files):
- CMakeLists_webclient.txt → rename to CMakeLists.txt
- build.bat

### Docs (2 files):
- START_HERE.md (read first!)
- README_WEB_CLIENT.md (detailed guide)

## 🎯 What It Does

✅ C++ server streams 1920x1080 @ 30fps test video
✅ Web browser receives and displays video
✅ Measures real-time bitrate (Mbps)
✅ Uses STUN for NAT traversal
✅ Works over internet, not just LAN

## 📊 Statistics Shown

- **Bitrate**: 2-8 Mbps (typical for VP8)
- **FPS**: Should be 30
- **Resolution**: 1920x1080
- **Packet Loss**: <1% is good
- **Jitter**: <30ms is good
- **Data Received**: Total MB transferred

## ⚙️ Configuration

### Change Video Quality
Edit webrtc_server.cpp:
```cpp
const int WIDTH = 1280;   // Lower = less bandwidth
const int HEIGHT = 720;
const int FPS = 30;
```

### Change Port
Edit webrtc_server.cpp:
```cpp
const int WS_PORT = 8080;  // Your port
```

### Add TURN Server
Edit peer_connection_handler.cpp:
```cpp
webrtc::PeerConnectionInterface::IceServer turn;
turn.uri = "turn:yourserver.com:3478";
turn.username = "user";
turn.password = "pass";
config.servers.push_back(turn);
```

## 🔧 Troubleshooting

| Problem | Solution |
|---------|----------|
| Can't find webrtc.lib | Check WEBRTC_ROOT path |
| WebSocket fails | Server not running or port blocked |
| Connection failed | STUN blocked, need TURN |
| No video | Check browser console (F12) |
| Stats all 0 | Browser doesn't support getStats() |

## 🌐 Test Scenarios

### Local Network:
- Server: Same computer
- Client: http://localhost:8080/
- Expected: 4-8 Mbps, <10ms latency

### Same Network:
- Server: Computer A
- Client: http://192.168.1.100:8080/
- Expected: 4-8 Mbps, <20ms latency

### Over Internet:
- Server: Your network
- Client: Different network (phone hotspot)
- Expected: 2-6 Mbps, 50-150ms latency
- Note: STUN enables this!

## 🎨 Client Customization

### Change Colors:
Edit client.html, find:
```css
background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
```

### Update Stats Frequency:
Edit client.html, find:
```javascript
}, 1000);  // Change to 500 for 2x/second
```

## 💻 Build Options

### Option 1: build.bat (Recommended)
```cmd
build.bat
```

### Option 2: CMake
```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Option 3: Command Line
```cmd
cl.exe /std:c++17 /EHsc ^
  /I"C:\webrtc-prebuilt\include" ^
  /I"C:\webrtc-prebuilt\include\third_party\abseil-cpp" ^
  /DWEBRTC_WIN /DNOMINMAX /DWIN32_LEAN_AND_MEAN /DRTC_ENABLE_WIN_WGC ^
  webrtc_server.cpp video_source.cpp throughput_receiver.cpp ^
  signaling_server.cpp peer_connection_handler.cpp ^
  /link C:\webrtc-prebuilt\release\webrtc.lib ^
  ws2_32.lib secur32.lib winmm.lib dmoguids.lib wmcodecdspuuid.lib ^
  msdmo.lib strmiids.lib iphlpapi.lib
```

## 🔍 Debugging

### Enable Verbose Logging:
Add to webrtc_server.cpp:
```cpp
rtc::LogMessage::LogToDebug(rtc::LS_VERBOSE);
```

### Check WebRTC Internals:
Open in Chrome:
```
chrome://webrtc-internals
```

### Monitor Network:
Chrome DevTools (F12) → Network tab

## 📈 Performance Tips

1. **Lower resolution** if CPU struggles
2. **Use Release build** (not Debug)
3. **Close other apps** during testing
4. **Check GPU encoding** is available
5. **Monitor CPU usage** in Task Manager

## 🎓 Next Steps

For your NVR system:
1. ✅ Test this basic setup
2. Replace TestVideoSource with camera feed
3. Add H.264 encoding
4. Implement multi-camera support
5. Add recording capability

## 📞 Help

- Detailed docs: README_WEB_CLIENT.md
- File guide: FILE_GUIDE.md
- Build issues: BUILD_WINDOWS.md
- Prebuilt setup: PREBUILT_GUIDE.md

## 🎉 Success Looks Like

Terminal:
```
Server running!
Open this URL in your browser:
  http://localhost:8080/
```

Browser:
```
🟢 Connected and streaming
Bitrate: 4.25 Mbps
FPS: 30.0
Resolution: 1920x1080
Packet Loss: 0 packets
```

---

**That's it!** You now have a working WebRTC STUN bitrate test system! 🚀
