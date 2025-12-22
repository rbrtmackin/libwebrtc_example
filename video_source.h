// video_source.h
// Video source that generates test frames using WebRTC threads

#ifndef VIDEO_SOURCE_H
#define VIDEO_SOURCE_H

#include <api/video/video_frame.h>
#include <api/video/i420_buffer.h>
#include <pc/video_track_source.h>
#include <rtc_base/thread.h>
#include <media/base/video_broadcaster.h>

#include <atomic>
#include <memory>

// Video track source that generates test frames
class TestVideoSource : public webrtc::VideoTrackSource {
public:
    TestVideoSource(int width, int height, int fps);
    ~TestVideoSource() override;

    // Start/stop frame generation
    void Start();
    void Stop();
    
    // Get statistics
    int GetFramesSent() const { return frames_sent_; }

protected:
    // VideoTrackSource implementation
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override;

private:
    // Frame generation on dedicated thread
    void GenerateFrames();
    
    int width_;
    int height_;
    int fps_;
    
    std::atomic<bool> running_;
    std::atomic<int> frames_sent_;
    
    // WebRTC thread for frame generation
    rtc::scoped_refptr<rtc::Thread> frame_thread_;
    
    // Broadcaster to distribute frames to sinks
    rtc::VideoBroadcaster broadcaster_;
};

#endif // VIDEO_SOURCE_H
