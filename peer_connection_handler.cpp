// peer_connection_handler.cpp
// Implementation of WebRTC peer connection handler

#include "peer_connection_handler.h"
#include "encoded_video_source.h"
#include <rtc_base/logging.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>

#include <iostream>

// PeerObserver implementation
PeerObserver::PeerObserver(SignalingCallback callback)
    : signaling_callback_(callback) {
}

void PeerObserver::SetThroughputReceiver(std::shared_ptr<ThroughputReceiver> receiver) {
    receiver_ = receiver;
}

void PeerObserver::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
    RTC_LOG(LS_INFO) << "Signaling state: " << new_state;
}

void PeerObserver::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
    RTC_LOG(LS_INFO) << "Data channel received";
}

void PeerObserver::OnRenegotiationNeeded() {
    RTC_LOG(LS_INFO) << "Renegotiation needed";
}

void PeerObserver::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
    std::cout << "ICE connection state: " << new_state << std::endl;
}

void PeerObserver::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
    RTC_LOG(LS_INFO) << "ICE gathering state: " << new_state;
}

void PeerObserver::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    std::string sdp;
    if (candidate->ToString(&sdp)) {
        // Send ICE candidate to client
        signaling_callback_("ice-candidate", sdp);
        RTC_LOG(LS_INFO) << "ICE candidate: " << candidate->sdp_mid();
    }
}

void PeerObserver::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
    RTC_LOG(LS_INFO) << "Track received";
}

// CreateSDPObserver implementation
CreateSDPObserver::CreateSDPObserver(std::function<void(webrtc::SessionDescriptionInterface*)> callback)
    : callback_(callback) {
}

void CreateSDPObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    callback_(desc);
}

void CreateSDPObserver::OnFailure(webrtc::RTCError error) {
    RTC_LOG(LS_ERROR) << "Create SDP failed: " << error.message();
}

// SetSDPObserver implementation
SetSDPObserver::SetSDPObserver(std::function<void()> callback)
    : callback_(callback) {
}

void SetSDPObserver::OnSuccess() {
    callback_();
}

void SetSDPObserver::OnFailure(webrtc::RTCError error) {
    RTC_LOG(LS_ERROR) << "Set SDP failed: " << error.message();
}

// PeerConnectionHandler implementation
PeerConnectionHandler::PeerConnectionHandler(
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory,
    std::shared_ptr<EncodedVideoSource> video_source,
    SignalingCallback signaling_callback)
    : factory_(factory),
      video_source_(video_source),
      signaling_callback_(signaling_callback) {
    
    receiver_ = std::make_shared<ThroughputReceiver>();
    observer_ = std::make_unique<PeerObserver>(signaling_callback);
    observer_->SetThroughputReceiver(receiver_);
    
    // Create peer connection with STUN server
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    
    // Add STUN server for NAT traversal
    webrtc::PeerConnectionInterface::IceServer stun_server;
    stun_server.uri = "stun:stun.l.google.com:19302";
    config.servers.push_back(stun_server);
    
    // You can add multiple STUN servers for redundancy
    webrtc::PeerConnectionInterface::IceServer stun_server2;
    stun_server2.uri = "stun:stun1.l.google.com:19302";
    config.servers.push_back(stun_server2);
    
    peer_connection_ = factory_->CreatePeerConnection(
        config,
        nullptr,
        nullptr,
        observer_.get()
    );
    
    if (!peer_connection_) {
        RTC_LOG(LS_ERROR) << "Failed to create peer connection";
        return;
    }
    
    // Add video track - pass video source directly as it now implements VideoTrackSourceInterface
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track = 
        factory_->CreateVideoTrack("video", video_source.get());
    
    auto result = peer_connection_->AddTrack(video_track, {"stream"});
    
    if (!result.ok()) {
        RTC_LOG(LS_ERROR) << "Failed to add track: " << result.error().message();
    }
    
    RTC_LOG(LS_INFO) << "Peer connection created with STUN support";
}

PeerConnectionHandler::~PeerConnectionHandler() {
    if (peer_connection_) {
        peer_connection_->Close();
    }
}

void PeerConnectionHandler::HandleOffer(const std::string& sdp) {
    RTC_LOG(LS_INFO) << "Received offer";
    
    // Create session description from SDP
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description_ptr =
        webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp, &error);
    
    if (!session_description_ptr) {
        RTC_LOG(LS_ERROR) << "Failed to parse offer: " << error.description;
        return;
    }
    
    // Get raw pointer and release ownership - SetRemoteDescription takes ownership
    webrtc::SessionDescriptionInterface* session_description = session_description_ptr.release();
    
    // Set remote description - older API takes raw pointer and observer
    auto set_observer = new rtc::RefCountedObject<SetSDPObserver>([this]() {
        RTC_LOG(LS_INFO) << "Remote description set, creating answer";
        CreateAnswer();
    });
    
    peer_connection_->SetRemoteDescription(set_observer, session_description);
}

void PeerConnectionHandler::CreateAnswer() {
    auto create_observer = new rtc::RefCountedObject<CreateSDPObserver>(
        [this](webrtc::SessionDescriptionInterface* desc) {
            // Set local description - older API
            auto set_observer = new rtc::RefCountedObject<SetSDPObserver>([this, desc]() {
                // Send answer to client
                std::string sdp;
                desc->ToString(&sdp);
                signaling_callback_("answer", sdp);
                RTC_LOG(LS_INFO) << "Answer sent to client";
            });
            
            // SetLocalDescription takes observer first, then raw pointer
            peer_connection_->SetLocalDescription(set_observer, desc);
        }
    );
    
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    peer_connection_->CreateAnswer(create_observer, options);
}

void PeerConnectionHandler::HandleIceCandidate(
    const std::string& candidate,
    const std::string& sdp_mid,
    int sdp_mline_index) {
    
    RTC_LOG(LS_INFO) << "Adding ICE candidate";
    
    webrtc::SdpParseError error;
    std::unique_ptr<webrtc::IceCandidateInterface> ice_candidate(
        webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error)
    );
    
    if (!ice_candidate) {
        RTC_LOG(LS_ERROR) << "Failed to parse ICE candidate: " << error.description;
        return;
    }
    
    if (!peer_connection_->AddIceCandidate(ice_candidate.get())) {
        RTC_LOG(LS_ERROR) << "Failed to add ICE candidate";
    }
}
