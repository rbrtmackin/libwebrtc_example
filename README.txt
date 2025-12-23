Using prebuild libwebrtc static library from https://github.com/bengreenier/webrtc

Resolution	FPS	Expected Bitrate	Use Case
1920x1080	30		2-4 Mbps				Baseline
1920x1080	60		4-8 Mbps				High FPS
2560x1440	30		4-8 Mbps				2K
3840x2160	30		10-20 Mbps			4K baseline
3840x2160	60		20-40 Mbps			4K stress test
7680x4320	30		40-80 Mbps			8K extreme

rmdir /s /q build
build.bat
cd build\bin\Release