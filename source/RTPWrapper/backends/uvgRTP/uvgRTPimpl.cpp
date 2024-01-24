#include "uvgRTP.h"

#include <iostream>
#include <map> 
#include <mutex> 
#include <numeric>
#include <algorithm>






namespace _uvgrtp 
{
    namespace ks
    {
        //will be gone for good.
        static const std::string local_address  {"127.0.0.1"};
    

        static constexpr int        _rce__flg_  {RCE_FRAGMENT_GENERIC|RCE_RECEIVE_ONLY};
        static constexpr int        _snd__flg_  {RCE_FRAGMENT_GENERIC|RCE_SEND_ONLY};
        
        //static constexpr int        _outbound_  {0};
        static constexpr int        _inbound__  {1};
    }

    namespace data
    {
        static StrmIOCodec   streamIOCodec{}; //StreamId -> Codec
    }
    
}


uint64_t UVGRTPWrap::Initialize()   {
    
    //Initialize mock
    return 0; 

}

uint64_t UVGRTPWrap::CreateSession(const std::string& localEndPoint){
    
    //Create Session and index the session
    auto psess = this->ctx.create_session(localEndPoint);
    //TODO: I need to change the raw pointer approach for a managed one.
    //auto psess = std::shared_ptr<uvgrtp::session>{ctx.create_session(localEndPoint), [this](uvgrtp::session* p) {ctx.destroy_session(p);}};
    return _uvgrtp::data::IndexSession(psess);
}

uint64_t UVGRTPWrap::CreateStream(uint64_t sessionId, int port, int direction){
    
    //Check if the port is valid
    if (port <= 0) return 0;
    //Get the Session
    auto psess = _uvgrtp::data::GetSession(sessionId);
    if (!psess)
    {
        return 0;
    }

    //Configure 
    auto flags  = direction == _uvgrtp::ks::_inbound__ ? _uvgrtp::ks::_rce__flg_ : _uvgrtp::ks::_snd__flg_;

    //Create the stream
    auto port_u16 = static_cast<uint16_t>(port);
    auto pstrm  = std::shared_ptr<uvgrtp::media_stream>{psess->create_stream(port_u16, RTP_FORMAT_OPUS, flags)};
    if (!pstrm)
    {
        return 0;
    }

    auto streamID= _uvgrtp::data::IndexStream(sessionId, pstrm);
    return streamID;
}

bool UVGRTPWrap::DestroyStream(uint64_t streamId){

    return _uvgrtp::data::RemoveStream(streamId);
}

bool UVGRTPWrap::DestroySession(uint64_t sessionId){

    return _uvgrtp::data::RemoveSession(sessionId);
}

SpSess UVGRTPWrap::GetSession(uint64_t sessionId){

    return _uvgrtp::data::GetSession(sessionId);
}

SpStrm UVGRTPWrap::GetStream(uint64_t streamId){
    
    return _uvgrtp::data::GetStream(streamId);
}

bool UVGRTPWrap::PushFrame (uint64_t streamId, std::vector<std::byte> pData) noexcept
{
    //If not encoder/decoder is found, just push the data.
    return GetStream(streamId)->push_frame(reinterpret_cast<uint8_t*>(pData.data()), pData.size(), RTP_NO_FLAGS) == RTP_OK;
}

bool UVGRTPWrap::PushFrame (uint64_t streamId, std::vector<std::byte> pData, uint32_t ts) noexcept
{
    return GetStream(streamId)->push_frame(reinterpret_cast<uint8_t*>(pData.data()), pData.size(), ts, RTP_NO_FLAGS) == RTP_OK;
}

std::vector<std::byte> UVGRTPWrap::GrabFrame (uint64_t, std::vector<std::byte>, uint32_t&) noexcept
{
    return {};

}
void UVGRTPWrap::Shutdown(){

}

UVGRTPWrap::~UVGRTPWrap(){
}

