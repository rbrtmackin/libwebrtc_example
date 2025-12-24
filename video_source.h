// video_source.h
// Video source that generates test frames using WebRTC threads

#ifndef VIDEO_SOURCE_H
#define VIDEO_SOURCE_H

#include <api/video/video_frame.h>
#include <api/video/i420_buffer.h>
#include <api/media_stream_interface.h>
#include <rtc_base/thread.h>
#include <rtc_base/ref_counted_object.h>
#include <media/base/video_broadcaster.h>

#include <atomic>
#include <memory>
#include <cstdlib> // Required for rand() and srand()
#include <ctime>   // Required for time()

// Video track source that generates test frames
// Inherits from VideoTrackSourceInterface for compatibility with CreateVideoTrack
class TestVideoSource : public webrtc::VideoTrackSourceInterface {
public:
    TestVideoSource(int width, int height, int fps);
    ~TestVideoSource() override;

    // Start/stop frame generation
    void Start();
    void Stop();
    
    // Get statistics
    int GetFramesSent() const { return frames_sent_; }

    // VideoSourceInterface implementation
    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                        const rtc::VideoSinkWants& wants) override;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
    
    // MediaSourceInterface implementation
    SourceState state() const override { return kLive; }
    bool remote() const override { return false; }
    
    // NotifierInterface implementation
    void RegisterObserver(webrtc::ObserverInterface* observer) override {}
    void UnregisterObserver(webrtc::ObserverInterface* observer) override {}
    
    // VideoTrackSourceInterface implementation
    bool is_screencast() const override { return false; }
    absl::optional<bool> needs_denoising() const override { return absl::nullopt; }
    bool GetStats(Stats* stats) override { return false; }
    bool SupportsEncodedOutput() const override { return false; }
    void GenerateKeyFrame() override {}
    void AddEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>* sink) override {}
    void RemoveEncodedSink(rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>* sink) override {}
    
    // RefCountInterface implementation (required for scoped_refptr)
    void AddRef() const override { ref_count_.IncRef(); }
    rtc::RefCountReleaseStatus Release() const override {
        const auto status = ref_count_.DecRef();
        if (status == rtc::RefCountReleaseStatus::kDroppedLastRef) {
            delete this;
        }
        return status;
    }

private:
    // Frame generation on dedicated thread
    void GenerateFrames();
    
    int width_;
    int height_;
    int fps_;
    
    std::atomic<bool> running_;
    std::atomic<int> frames_sent_;
    
    // WebRTC thread for frame generation - using unique_ptr
    std::unique_ptr<rtc::Thread> frame_thread_;
    
    // Broadcaster to distribute frames to sinks
    rtc::VideoBroadcaster broadcaster_;
    
    // Reference counting
    mutable webrtc::webrtc_impl::RefCounter ref_count_{0};
};

#endif // VIDEO_SOURCE_H
