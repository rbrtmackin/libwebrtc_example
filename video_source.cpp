// video_source.cpp
// Implementation of test video source with WebRTC thread management

#include "video_source.h"
#include <rtc_base/logging.h>
#include <thread>
#include <chrono>

TestVideoSource::TestVideoSource(int width, int height, int fps)
    : width_(width),
      height_(height),
      fps_(fps),
      running_(false),
      frames_sent_(0) {
    
    // Create a dedicated thread for frame generation
    frame_thread_ = rtc::Thread::Create();
    frame_thread_->SetName("FrameGenerator", nullptr);
    frame_thread_->Start();
    
    RTC_LOG(LS_INFO) << "TestVideoSource created: " << width << "x" << height 
                     << " @ " << fps << " fps";
}

TestVideoSource::~TestVideoSource() {
    Stop();
    
    if (frame_thread_) {
        frame_thread_->Stop();
        frame_thread_ = nullptr;
    }
    
    RTC_LOG(LS_INFO) << "TestVideoSource destroyed";
}

void TestVideoSource::AddOrUpdateSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
    broadcaster_.AddOrUpdateSink(sink, wants);
}

void TestVideoSource::RemoveSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
    broadcaster_.RemoveSink(sink);
}

void TestVideoSource::Start() {
    if (running_) {
        RTC_LOG(LS_WARNING) << "Already running";
        return;
    }
    
    running_ = true;
    frames_sent_ = 0;
    
    // Post the frame generation task to the WebRTC thread
    frame_thread_->PostTask([this]() {
        GenerateFrames();
    });
    
    RTC_LOG(LS_INFO) << "Frame generation started";
}

void TestVideoSource::Stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Wait a bit for the generation loop to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    RTC_LOG(LS_INFO) << "Frame generation stopped. Total frames: " << frames_sent_;
}

void TestVideoSource::GenerateFrames() {
    auto frame_interval = std::chrono::microseconds(1000000 / fps_);
    auto next_frame_time = std::chrono::steady_clock::now();
    
    RTC_LOG(LS_INFO) << "Starting frame generation loop";
    
    while (running_) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Create I420 frame buffer
        rtc::scoped_refptr<webrtc::I420Buffer> buffer =
            webrtc::I420Buffer::Create(width_, height_);
        
        // Fill with gradient pattern for realistic data
        // Y plane (luminance)
        uint8_t* y_data = buffer->MutableDataY();
        int y_size = width_ * height_;
        for (int i = 0; i < y_size; i++) {
            y_data[i] = static_cast<uint8_t>((i + frames_sent_) % 256);
        }
        
        // U and V planes (chrominance) - set to neutral gray
        int uv_size = buffer->ChromaWidth() * buffer->ChromaHeight();
        memset(buffer->MutableDataU(), 128, uv_size);
        memset(buffer->MutableDataV(), 128, uv_size);
        
        // Create VideoFrame
        // Use microseconds since epoch for timestamp
        int64_t timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(buffer)
            .set_timestamp_us(timestamp_us)
            .build();
        
        // Broadcast frame to all sinks
        broadcaster_.OnFrame(frame);
        frames_sent_++;
        
        // Calculate sleep time to maintain target FPS
        next_frame_time += frame_interval;
        auto now = std::chrono::steady_clock::now();
        
        if (next_frame_time > now) {
            auto sleep_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                next_frame_time - now);
            std::this_thread::sleep_for(sleep_duration);
        } else {
            // Running behind schedule, reset timing
            next_frame_time = now;
            RTC_LOG(LS_WARNING) << "Frame generation running behind schedule";
        }
        
        // Log progress every second
        if (frames_sent_ % fps_ == 0) {
            RTC_LOG(LS_VERBOSE) << "Generated " << frames_sent_ << " frames";
        }
    }
    
    RTC_LOG(LS_INFO) << "Frame generation loop ended";
}
