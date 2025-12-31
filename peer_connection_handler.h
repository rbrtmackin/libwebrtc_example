// peer_connection_handler.h
// Handles WebRTC peer connection for a single client

#ifndef PEER_CONNECTION_HANDLER_H
#define PEER_CONNECTION_HANDLER_H

#include "throughput_receiver.h"

#include <api/peer_connection_interface.h>
#include <api/create_peerconnection_factory.h>
#include <api/media_stream_interface.h>
#include <rtc_base/thread.h>

#include <memory>
#include <functional>

// Forward declarations
class EncodedVideoSource;
class PeerConnectionHandler;

// Callback for sending signaling messages
using SignalingCallback = std::function<void(const std::string& type, const std::string& message)>;

// Observer for peer connection events
class PeerObserver : public webrtc::PeerConnectionObserver {
public:
    PeerObserver(SignalingCallback callback);
    ~PeerObserver() override = default;

    void SetThroughputReceiver(std::shared_ptr<ThroughputReceiver> receiver);
    void SetPeerConnectionHandler(PeerConnectionHandler* handler);

    // PeerConnectionObserver implementation
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;
    void OnRenegotiationNeeded() override;
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
    void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;

private:
    SignalingCallback signaling_callback_;
    std::shared_ptr<ThroughputReceiver> receiver_;
    bool answer_created_;
    bool gathering_complete_;
    PeerConnectionHandler* handler_;
    
    friend class PeerConnectionHandler;
};

// Session description observer callbacks
class CreateSDPObserver : public webrtc::CreateSessionDescriptionObserver {
public:
    CreateSDPObserver(std::function<void(webrtc::SessionDescriptionInterface*)> callback);
    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
    void OnFailure(webrtc::RTCError error) override;
    
    // Reference counting - required for scoped_refptr
    void AddRef() const override {}
    rtc::RefCountReleaseStatus Release() const override {
        return rtc::RefCountReleaseStatus::kDroppedLastRef;
    }
    
private:
    std::function<void(webrtc::SessionDescriptionInterface*)> callback_;
};

class SetSDPObserver : public webrtc::SetSessionDescriptionObserver {
public:
    SetSDPObserver(std::function<void()> callback);
    void OnSuccess() override;
    void OnFailure(webrtc::RTCError error) override;
    
    // Reference counting - required for scoped_refptr
    void AddRef() const override {}
    rtc::RefCountReleaseStatus Release() const override {
        return rtc::RefCountReleaseStatus::kDroppedLastRef;
    }
    
private:
    std::function<void()> callback_;
};

// Handles a single peer connection
class PeerConnectionHandler {
public:
    PeerConnectionHandler(
        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory,
        std::shared_ptr<EncodedVideoSource> video_source,
        SignalingCallback signaling_callback);
    ~PeerConnectionHandler();

    // Handle incoming signaling messages
    void HandleOffer(const std::string& sdp);
    void HandleIceCandidate(const std::string& candidate, const std::string& sdp_mid, int sdp_mline_index);
    void CreateAnswer();  // Public so observer can call when gathering completes
    
    // Get stats
    std::shared_ptr<ThroughputReceiver> GetReceiver() { return receiver_; }

private:
    
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
    std::shared_ptr<EncodedVideoSource> video_source_;
    std::shared_ptr<ThroughputReceiver> receiver_;
    std::unique_ptr<PeerObserver> observer_;
    SignalingCallback signaling_callback_;
};

#endif // PEER_CONNECTION_HANDLER_H
