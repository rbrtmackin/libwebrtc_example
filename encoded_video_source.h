// encoded_video_source.h
// Video source that generates and reuses pre-encoded VP8 frames

#ifndef ENCODED_VIDEO_SOURCE_H
#define ENCODED_VIDEO_SOURCE_H

#include <api/video/video_frame.h>
#include <api/video/i420_buffer.h>
#include <api/media_stream_interface.h>
#include <api/video_codecs/video_encoder.h>
#include <modules/video_coding/codecs/vp8/include/vp8.h>
#include <rtc_base/thread.h>
#include <rtc_base/ref_counted_object.h>
#include <media/base/video_broadcaster.h>

#include <atomic>
#include <memory>
#include <vector>

// Structure to hold an encoded frame
struct EncodedFrameData {
    std::vector<uint8_t> data;
    bool is_keyframe;
    int64_t timestamp_us;
};

// Video source that sends pre-encoded frames
class EncodedVideoSource : public webrtc::VideoTrackSourceInterface {
public:
    EncodedVideoSource(int width, int height, int fps, int gop_size = 30);
    ~EncodedVideoSource() override;

    // Start/stop frame generation
    void Start();
    void Stop();
    
    // Get statistics
    int GetFramesSent() const { return frames_sent_; }
    size_t GetEncodedGOPSize() const { return encoded_gop_.size(); }

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
    
    // RefCountInterface implementation
    void AddRef() const override { ref_count_.IncRef(); }
    rtc::RefCountReleaseStatus Release() const override {
        const auto status = ref_count_.DecRef();
        if (status == rtc::RefCountReleaseStatus::kDroppedLastRef) {
            delete this;
        }
        return status;
    }

private:
    // Pre-encode a GOP
    void EncodeGOP();
    
    // Send pre-encoded frames
    void SendFrames();
    
    int width_;
    int height_;
    int fps_;
    int gop_size_;
    
    std::atomic<bool> running_;
    std::atomic<int> frames_sent_;
    
    // Pre-encoded GOP
    std::vector<EncodedFrameData> encoded_gop_;
    bool gop_encoded_;
    
    // WebRTC thread for frame sending
    std::unique_ptr<rtc::Thread> frame_thread_;
    
    // Broadcaster to distribute frames to sinks
    rtc::VideoBroadcaster broadcaster_;
    
    // Reference counting
    mutable webrtc::webrtc_impl::RefCounter ref_count_{0};
};

#endif // ENCODED_VIDEO_SOURCE_H
