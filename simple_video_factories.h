// simple_video_factories.h
// Simple video encoder/decoder factories for bengreenier/webrtc

#ifndef SIMPLE_VIDEO_FACTORIES_H
#define SIMPLE_VIDEO_FACTORIES_H

#include <api/video_codecs/video_encoder_factory.h>
#include <api/video_codecs/video_decoder_factory.h>
#include <api/video_codecs/sdp_video_format.h>
#include <api/video_codecs/vp8_temporal_layers.h>
#include <modules/video_coding/codecs/vp8/include/vp8.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>
#include <memory>
#include <vector>

namespace webrtc {

// Video encoder factory that creates real VP8/VP9 encoders
class SimpleVideoEncoderFactory : public VideoEncoderFactory {
public:
    std::vector<SdpVideoFormat> GetSupportedFormats() const override {
        std::vector<SdpVideoFormat> formats;
        formats.push_back(SdpVideoFormat("VP8"));
        formats.push_back(SdpVideoFormat("VP9"));
        return formats;
    }

    std::unique_ptr<VideoEncoder> CreateVideoEncoder(const SdpVideoFormat& format) override {
        if (format.name == "VP8") {
            return VP8Encoder::Create();
        }
        if (format.name == "VP9") {
            return VP9Encoder::Create();
        }
        return nullptr;
    }
};

// Video decoder factory that creates real VP8/VP9 decoders
class SimpleVideoDecoderFactory : public VideoDecoderFactory {
public:
    std::vector<SdpVideoFormat> GetSupportedFormats() const override {
        std::vector<SdpVideoFormat> formats;
        formats.push_back(SdpVideoFormat("VP8"));
        formats.push_back(SdpVideoFormat("VP9"));
        return formats;
    }

    std::unique_ptr<VideoDecoder> CreateVideoDecoder(const SdpVideoFormat& format) override {
        if (format.name == "VP8") {
            return VP8Decoder::Create();
        }
        if (format.name == "VP9") {
            return VP9Decoder::Create();
        }
        return nullptr;
    }
};

} // namespace webrtc

#endif // SIMPLE_VIDEO_FACTORIES_H
