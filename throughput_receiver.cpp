// throughput_receiver.cpp
// Implementation of throughput measurement receiver

#include "throughput_receiver.h"
#include <rtc_base/logging.h>
#include <iostream>
#include <iomanip>

ThroughputReceiver::ThroughputReceiver()
    : frames_(0),
      bytes_(0),
      start_(std::chrono::steady_clock::now()) {
    RTC_LOG(LS_INFO) << "ThroughputReceiver created";
}

void ThroughputReceiver::OnFrame(const webrtc::VideoFrame& frame) {
    frames_++;
    
    // Calculate frame size (I420 format: width * height * 1.5)
    auto buffer = frame.video_frame_buffer();
    int frame_size = buffer->width() * buffer->height() * 3 / 2;
    bytes_ += frame_size;
    
    // Print stats every 60 frames (every 2 seconds at 30fps)
    if (frames_ % 60 == 0) {
        PrintStats();
    }
}

void ThroughputReceiver::PrintStats() {
    double elapsed = GetElapsedSeconds();
    
    if (elapsed > 0) {
        double fps = frames_ / elapsed;
        double mbps = (bytes_ * 8.0) / (elapsed * 1000000.0);
        double mbytes_total = bytes_ / (1024.0 * 1024.0);
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Frames: " << std::setw(6) << frames_
                  << " | FPS: " << std::setw(6) << fps
                  << " | Throughput: " << std::setw(8) << mbps << " Mbps"
                  << " | Total: " << std::setw(8) << mbytes_total << " MB"
                  << std::endl;
    }
}

void ThroughputReceiver::Reset() {
    frames_ = 0;
    bytes_ = 0;
    start_ = std::chrono::steady_clock::now();
    RTC_LOG(LS_INFO) << "ThroughputReceiver reset";
}

double ThroughputReceiver::GetElapsedSeconds() const {
    auto elapsed = std::chrono::steady_clock::now() - start_;
    return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0;
}
