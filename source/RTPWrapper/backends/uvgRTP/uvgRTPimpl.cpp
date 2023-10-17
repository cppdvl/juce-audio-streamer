#include "uvgRTP.h"

#include <iostream>
#include <map> 
#include <mutex> 



namespace _uvgrtp 
{
    std::recursive_mutex rmtx; 
    static uint64_t _id{0};

    namespace ks
    {
        //will be gone for good.
        static const std::string local_address  {"127.0.0.1"};
    
        static constexpr uint16_t   _inb__port  {5004};
        static constexpr uint16_t   _oub__port  {5005};

        static constexpr uint16_t   port__inb_  {5005};
        static constexpr uint16_t   port__oub_  {5004};

        static constexpr int        _rce__flg_  {RCE_FRAGMENT_GENERIC|RCE_RECEIVE_ONLY};
        static constexpr int        _snd__flg_  {RCE_FRAGMENT_GENERIC|RCE_SEND_ONLY};
        
        static constexpr int        _outbound_  {0};
        static constexpr int        _inbound__  {1};
    }

    namespace data
    {
        static SessStrmIndex masterIndex{};   //SessionId -> Map(StreamId -> Shared Pointer to Stream) 
        static SessIndex     sessionIndex{};  //SessionId -> Shared Pointer to Session
        static StrmSessIndex streamIndex{};   //StreamId -> SessionId.


        SpStrm GetStream(uint64_t sessionId, uint64_t streamId)
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
            if (sessionFound && masterIndex[sessionId].find(streamId) != masterIndex[sessionId].end())
            {
                return masterIndex[sessionId][streamId];
            }
            return nullptr;
        }
        
        SpStrm GetStream(uint64_t streamId)
        {
            auto sessionId = 0;
            {
                std::lock_guard<std::recursive_mutex> lock(rmtx);
                if (streamIndex.find(streamId) != streamIndex.end()) {
                    sessionId = streamIndex[streamId];
                }
            }
            return !sessionId ? nullptr : GetStream(sessionId, streamId);
        }
        SpSess GetSession(uint64_t sessionId)
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            if (sessionIndex.find(sessionId) != sessionIndex.end())
            {
                return sessionIndex[sessionId];
            }
            return nullptr;
        }
        uint64_t IndexStream(uint64_t sessionId, SpStrm stream)
        {
            if (stream == nullptr)
            {
                return 0;
            }
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            auto streamId = ++_id;
            masterIndex[sessionId][streamId] = stream;
            streamIndex[streamId] = sessionId;
            return streamId;
        }
        uint64_t IndexSession(SpSess session)
        {
            if (session == nullptr)
            {
                return 0;
            }
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            auto sessionId = ++_id;
            sessionIndex[sessionId] = session;
            return sessionId;
        }
        bool RemoveStream(uint64_t sessionId, uint64_t streamId)
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
            if (sessionFound && masterIndex[sessionId].find(streamId) != masterIndex[sessionId].end())
            {
                masterIndex[sessionId].erase(streamId);
                streamIndex.erase(streamId);
                return true;
            }
            return false;
        }
        bool RemoveStream(uint64_t streamId)
        {
            auto sessionId = 0;
            {
                std::lock_guard<std::recursive_mutex> lock(rmtx);
                if (streamIndex.find(streamId) != streamIndex.end()) {
                    sessionId = streamIndex[streamId];
                }
            }
            return RemoveStream(sessionId, streamId);
        }
        bool RemoveSession(uint64_t sessionId)
        {
            {
                std::lock_guard<std::recursive_mutex> lock(rmtx);
                auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
                if (!sessionFound) return false;
            }
            //Remove the streams 
            auto strms = masterIndex[sessionId];
            for (auto& strm : strms)
            {
                RemoveStream(strm.first);
            }
            masterIndex.erase(sessionId);
            sessionIndex.erase(sessionId);
            return true;
        }
    }
    
}


uint64_t UVGRTPWrap::Initialize()   {
    
    //Initialize mock
    return 0; 

}

uint64_t UVGRTPWrap::CreateSession(const std::string& localEndPoint){
    
    //Create Session and index the session 
    auto psess = std::shared_ptr<uvgrtp::session>{ctx.create_session(localEndPoint)};
    return _uvgrtp::data::IndexSession(psess);

}

uint64_t UVGRTPWrap::CreateStream(uint64_t sessionId, int port, int direction){
    
    //Get the Session
    auto psess = _uvgrtp::data::GetSession(sessionId);
    if (!psess)
    {
        return 0;
    }

    //Configure 
    auto flags  = direction == _uvgrtp::ks::_inbound__ ? _uvgrtp::ks::_rce__flg_ : _uvgrtp::ks::_snd__flg_;
    port        = port == 0 ? _uvgrtp::ks::_oub__port : port;
    
    //Create the stream 
    auto pstrm  = std::shared_ptr<uvgrtp::media_stream>{psess->create_stream(port, RTP_FORMAT_GENERIC, flags)};
    return _uvgrtp::data::IndexStream(sessionId, pstrm);
}

bool UVGRTPWrap::DestroyStream(uint64_t streamId){

    return _uvgrtp::data::RemoveStream(streamId);
}

bool UVGRTPWrap::DestroySession(uint64_t sessionId){

    return _uvgrtp::data::RemoveStream(sessionId);
}

SpSess UVGRTPWrap::GetSession(uint64_t sessionId){

    return _uvgrtp::data::GetSession(sessionId);
}

SpStrm UVGRTPWrap::GetStream(uint64_t streamId){
    
    return _uvgrtp::data::GetStream(streamId);
}


void UVGRTPWrap::Shutdown(){

}

UVGRTPWrap::~UVGRTPWrap(){
}


