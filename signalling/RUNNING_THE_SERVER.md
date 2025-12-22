# Building and Running the WebRTC Server

## ⚠️ IMPORTANT: Current Limitation

The C++ code I provided earlier has an incomplete signaling server because implementing WebSocket in C++ requires additional libraries. Here are your options:

---

## 🎯 Option 1: Use Node.js Signaling Server (EASIEST)

### What You Need:
1. Node.js installed (download from https://nodejs.org/)
2. The files: `signaling-server.js`, `package.json`, `client.html`

### Steps:

#### 1. Install Node.js dependencies:
```cmd
npm install
```

#### 2. Start the signaling server:
```cmd
npm start
```

You should see:
```
==================================================
WebRTC Signaling Server Running
==================================================
Server: http://localhost:8080
WebSocket: ws://localhost:8080

Open http://localhost:8080/client.html in your browser
==================================================
```

#### 3. Open browser:
Navigate to: `http://localhost:8080/client.html`

### ❌ Problem:
This setup only has the signaling server, but **your C++ code doesn't connect to it yet**. The C++ server needs to be modified to connect to this Node.js signaling server.

---

## 🎯 Option 2: Skip C++ Server, Use go2rtc (RECOMMENDED FOR TESTING)

Since you're familiar with go2rtc, use that for testing instead:

### 1. Install go2rtc:
```cmd
# Download from https://github.com/AlexxIT/go2rtc/releases
# Extract go2rtc.exe
```

### 2. Create go2rtc.yaml:
```yaml
streams:
  test:
    - "ffmpeg:testsrc=size=1920x1080:rate=30#video=h264"

webrtc:
  candidates:
    - stun:stun.l.google.com:19302
```

### 3. Run go2rtc:
```cmd
go2rtc.exe -config go2rtc.yaml
```

### 4. Open browser:
```
http://localhost:1984/
```

This gives you a working WebRTC test immediately with STUN support and bitrate stats!

---

## 🎯 Option 3: Build C++ Server Without Signaling

For now, your C++ server compiles but **cannot be run** because the signaling is incomplete.

### What the C++ server currently does:
✅ Compiles successfully  
✅ Creates video source  
✅ Initializes WebRTC  
❌ **Cannot accept browser connections** (no working signaling)

### To actually use it, you need to:

1. **Add a WebSocket library** (like websocketpp or uWebSockets)
2. **Implement proper signaling** in `signaling_server.cpp`
3. **Connect the browser** to the C++ server

This is complex and time-consuming.

---

## 💡 My Recommendation

**For testing bitrate with STUN:**

1. Use **go2rtc** (you already know it)
2. It already supports:
   - WebRTC streaming
   - STUN servers
   - Bitrate measurement
   - Web browser client

**For learning C++ WebRTC:**

The code I provided is a great **reference** for:
- How to use bengreenier/webrtc API
- Video source implementation
- Peer connection setup
- STUN configuration

But it needs more work to be functional.

---

## 🔧 What Would Make the C++ Server Work?

You would need to:

### 1. Add WebSocket library to CMakeLists.txt:
```cmake
# Add websocketpp or similar
find_package(websocketpp REQUIRED)
target_link_libraries(webrtc_server websocketpp::websocketpp)
```

### 2. Implement signaling_server.cpp with real WebSocket handling

### 3. Integrate with your existing infrastructure

This is probably 20+ hours of work to do properly.

---

## ✅ Quick Test Right Now

**Want to see something working immediately?**

### Test Video Source Only:

Create a simple test program:

```cpp
// test_video_source.cpp
#include "video_source.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    TestVideoSource source(1920, 1080, 30);
    source.Start();
    
    std::cout << "Generating frames for 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    source.Stop();
    std::cout << "Total frames generated: " << source.GetFramesSent() << std::endl;
    
    return 0;
}
```

Build it:
```cmd
cl.exe /std:c++20 /EHsc /MT ^
  /I"C:\webrtc-prebuilt\include" ^
  /I"C:\webrtc-prebuilt\include\third_party\abseil-cpp" ^
  /DWEBRTC_WIN /DNOMINMAX /DWIN32_LEAN_AND_MEAN /DRTC_ENABLE_WIN_WGC ^
  test_video_source.cpp video_source.cpp ^
  /link C:\webrtc-prebuilt\release\webrtc.lib ws2_32.lib

test_video_source.exe
```

This will at least verify your video source works!

---

## 📊 Summary

| Option | Complexity | Time to Working | Recommended |
|--------|-----------|----------------|-------------|
| go2rtc | Easy | 5 minutes | ✅ YES |
| Node.js signaling | Medium | 1 hour | ⚠️ Incomplete |
| Full C++ server | Hard | 20+ hours | ❌ Not now |
| Test video source | Easy | 10 minutes | ✅ Good for learning |

---

## 🤔 What Do You Want To Do?

1. **Test bitrate/STUN immediately?** → Use go2rtc
2. **Learn the C++ WebRTC API?** → Build and test the video source
3. **Build a production C++ server?** → Need to implement proper signaling (major project)

Let me know which direction you want to go!
