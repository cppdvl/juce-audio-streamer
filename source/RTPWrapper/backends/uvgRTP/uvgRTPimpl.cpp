#include "uvgRTP.h"

#include <iostream>
#include <map> 
#include <mutex> 
#include <numeric>
#include <algorithm>

#include "opusImpl.h"





namespace _uvgrtp 
{
    std::recursive_mutex rmtx;
    static uint64_t _id{0};

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
        static SessStrmIndex masterIndex{};   //SessionId -> Map(StreamId -> Shared Pointer to Stream)
        static SessIndex     sessionIndex{};  //SessionId -> Shared Pointer to Session
        static StrmSessIndex streamIndex{};   //StreamId -> SessionId.
        static StrmIOCodec   streamIOCodec{}; //StreamId -> Codec

        /*! @brief Get the stream from the session index.
         *
         * @param sessionId
         * @param streamId
         * @return A shared Ptr to the Stream.nullptr will be returned if the stream in not found.
         */
        SpStrm GetStream(uint64_t sessionId, uint64_t streamId);
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

        /*! @brief Get the stream from the stream index.
         *
         * @param streamId
         * @return A shared Ptr to the Stream.nullptr will be returned if the stream in not found.
         */
        SpStrm GetStream(uint64_t streamId);
        SpStrm GetStream(uint64_t streamId)
        {
            auto sessionId = 0ul;
            {
                std::lock_guard<std::recursive_mutex> lock(rmtx);
                if (streamIndex.find(streamId) != streamIndex.end()) {
                    sessionId = streamIndex[streamId];
                }
            }
            return !sessionId ? nullptr : GetStream(sessionId, streamId);
        }

        /*! @brief Get the session from the session index.
         *
         * @param sessionId
         * @return A shared Ptr to the Session.nullptr will be returned if the session in not found.
         */
        SpSess GetSession(uint64_t sessionId);
        SpSess GetSession(uint64_t sessionId)
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            if (sessionIndex.find(sessionId) != sessionIndex.end())
            {
                return sessionIndex[sessionId];
            }
            return nullptr;
        }

        /*! @brief Index the stream in the session index.
         *
         * @param sessionId
         * @param stream
         * @return The streamId of the indexed stream. 0 will be returned if the stream is null.
         */
        uint64_t IndexStream(uint64_t sessionId, SpStrm stream);
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

        /*! @brief Index the session in the session index.
         *
         * @param session
         * @return The sessionId of the indexed session. 0 will be returned if the session is null.
         */
        uint64_t IndexSession(SpSess session);
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

        /*! @brief Remove the stream from the session index.
         *
         * @param sessionId
         * @param streamId
         * @return true if the stream was removed. false if the stream was not found.
         */
        bool RemoveStream(uint64_t sessionId, uint64_t streamId);
        bool RemoveStream(uint64_t sessionId, uint64_t streamId)
        {
            std::lock_guard<std::recursive_mutex> lock(rmtx);
            auto sessionFound = masterIndex.find(sessionId) != masterIndex.end();
            if (sessionFound && masterIndex[sessionId].find(streamId) != masterIndex[sessionId].end())
            {
                masterIndex[sessionId].erase(streamId);
                streamIndex.erase(streamId);
                auto codecFound = streamIOCodec.find(streamId) != streamIOCodec.end();
                if (codecFound) streamIOCodec.erase(streamId);
                return true;
            }
            return false;
        }

        /*! @brief Remove the stream from the session index.
         *
         * @param streamId
         * @return true if the stream was removed. false if the stream was not found.
         */
        bool RemoveStream(uint64_t streamId);
        bool RemoveStream(uint64_t streamId)
        {
            auto sessionId = 0ul;
            {
                std::lock_guard<std::recursive_mutex> lock(rmtx);
                if (streamIndex.find(streamId) != streamIndex.end()) {
                    sessionId = streamIndex[streamId];
                }
            }
            return RemoveStream(sessionId, streamId);
        }

        /*! @brief Remove the session from the session index.
         *
         * @param sessionId
         * @return true if the session was removed. false if the session was not found.
         */
        bool RemoveSession(uint64_t sessionId);
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

    return _uvgrtp::data::RemoveStream(sessionId);
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
void UVGRTPWrap::Shutdown(){

}

UVGRTPWrap::~UVGRTPWrap(){
}

