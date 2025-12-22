// throughput_receiver.h
// Video frame receiver that measures throughput

#ifndef THROUGHPUT_RECEIVER_H
#define THROUGHPUT_RECEIVER_H

#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>

#include <atomic>
#include <chrono>

// Receives video frames and measures throughput
class ThroughputReceiver : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    ThroughputReceiver();
    ~ThroughputReceiver() override = default;

    // VideoSinkInterface implementation
    void OnFrame(const webrtc::VideoFrame& frame) override;

    // Print current statistics
    void PrintStats();
    
    // Reset statistics
    void Reset();
    
    // Get statistics
    int GetFrameCount() const { return frames_; }
    long long GetBytes() const { return bytes_; }
    double GetElapsedSeconds() const;

private:
    std::atomic<int> frames_;
    std::atomic<long long> bytes_;
    std::chrono::steady_clock::time_point start_;
};

#endif // THROUGHPUT_RECEIVER_H
