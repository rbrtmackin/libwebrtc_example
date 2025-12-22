// simple_video_factories.h
// Simple video encoder/decoder factories for bengreenier/webrtc

#ifndef SIMPLE_VIDEO_FACTORIES_H
#define SIMPLE_VIDEO_FACTORIES_H

#include <api/video_codecs/video_encoder_factory.h>
#include <api/video_codecs/video_decoder_factory.h>
#include <api/video_codecs/sdp_video_format.h>
#include <memory>
#include <vector>

namespace webrtc {

// Simple encoder factory that reports VP8 support but doesn't actually create encoders
// This is enough for the peer connection to be created
class SimpleVideoEncoderFactory : public VideoEncoderFactory {
public:
    std::vector<SdpVideoFormat> GetSupportedFormats() const override {
        std::vector<SdpVideoFormat> formats;
        // Report VP8 support
        formats.push_back(SdpVideoFormat("VP8"));
        return formats;
    }

    std::unique_ptr<VideoEncoder> CreateVideoEncoder(const SdpVideoFormat& format) override {
        // Return nullptr - we're only sending, not encoding
        // The browser will handle encoding on its side
        return nullptr;
    }
};

// Simple decoder factory that reports VP8 support but doesn't actually create decoders
class SimpleVideoDecoderFactory : public VideoDecoderFactory {
public:
    std::vector<SdpVideoFormat> GetSupportedFormats() const override {
        std::vector<SdpVideoFormat> formats;
        // Report VP8 support
        formats.push_back(SdpVideoFormat("VP8"));
        return formats;
    }

    std::unique_ptr<VideoDecoder> CreateVideoDecoder(const SdpVideoFormat& format) override {
        // Return nullptr - we're only receiving, not decoding
        // The browser will handle decoding on its side
        return nullptr;
    }
};

} // namespace webrtc

#endif // SIMPLE_VIDEO_FACTORIES_H
