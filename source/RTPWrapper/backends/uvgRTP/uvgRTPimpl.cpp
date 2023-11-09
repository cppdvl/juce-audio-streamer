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

uint64_t UVGRTPWrap::CreateStream (uint64_t sessionId, const RTPStreamConfig& streamConfiguration)
{
    //Create Session and index the session
    auto streamId = CreateStream(sessionId, streamConfiguration.mPort, streamConfiguration.mDirection);
    if (!streamId) return 0;

    //Create the Encoder/Decoder
    _uvgrtp::data::streamIOCodec[streamId] = std::make_shared<OpusImpl::CODEC>(streamConfiguration);
    return streamId;
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
    auto pstrm  = std::shared_ptr<uvgrtp::media_stream>{psess->create_stream(port_u16, RTP_FORMAT_GENERIC, flags)};
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

std::vector<std::byte> UVGRTPWrap::decode(uint64_t streamId, std::vector<std::byte> pData)
{
    auto codec = _uvgrtp::data::streamIOCodec[streamId];
    if (codec == nullptr)
    {
        std::cout << "Error: No Codec" << std::endl;
        return {};
    }
    if (codec->cfg.mUseOpus == false) return pData;

    std::vector<std::byte> decodedData{};
    auto [
        nChan,
        nSamp,
        nBytesInDecodedChan0,
        nBytesInDecodedChan1] = hdrunpck(decodedData, pData);


    std::vector<std::byte> decodedInformation{};
    std::vector<size_t> channelSizesInBytes { nBytesInDecodedChan0, nBytesInDecodedChan1 };
    size_t offset = 0;
    for (int channIndex = 0; channIndex < 2; ++channIndex)
    {
        auto chanSzBytes = channelSizesInBytes[(size_t) channIndex];
        if (chanSzBytes == 0) continue;
        auto ptrPayload = &(pData.data()[offset]);
        offset += chanSzBytes;
        auto [result, decChann, decodedDataSz] = codec->decodeChannel(ptrPayload, chanSzBytes, (size_t)channIndex);
        if (result != OpusImpl::Result::OK) return std::vector<std::byte>{};

        //std::copy(decChann.begin(), decChann.end(), std::back_inserter(decodedInformation));
    }
    decodedInformation.insert(decodedInformation.begin(), decodedData.begin(), decodedData.end());
    return decodedInformation;
}
std::vector<std::byte> UVGRTPWrap::encode(uint64_t streamId, std::vector<std::byte> pData)
{
    auto codec = _uvgrtp::data::streamIOCodec[streamId];
    if (codec->cfg.mUseOpus == false) return pData;

    //This name is misleading. It Will BE the encoded data recipient but not yet.
    std::vector<std::byte> encodedData {}; //encoded data is empty

    auto [nChan, nSamp, chan0Size, chan1Size] = hdrunpck(encodedData, pData);

    for (size_t index = 0; index < nChan; ++index)
    {
        auto [result, encChanDt, encChanSz] = codec->encodeChannel(reinterpret_cast<float*>(pData.data()) + index * nSamp, index);
        std::copy(encChanDt.begin(), encChanDt.end(), std::back_inserter(encodedData));
    }

    return encodedData;
}
bool UVGRTPWrap::PushFrame (uint64_t streamId, std::vector<std::byte> pData) noexcept
{
    //If not encoder/decoder is found, just push the data.
    auto codecNotFound = _uvgrtp::data::streamIOCodec.find(streamId) == _uvgrtp::data::streamIOCodec.end();
    return GetStream(streamId)->push_frame(codecNotFound ? reinterpret_cast<uint8_t*>(pData.data()) : reinterpret_cast<uint8_t*>(encode(streamId, pData).data()), pData.size(), RTP_NO_FLAGS) == RTP_OK;
}
std::vector<std::byte> UVGRTPWrap::GrabFrame(uint64_t streamId, std::vector<std::byte> pData) noexcept
{
    return decode(streamId, pData);
}
void UVGRTPWrap::Shutdown(){

}

UVGRTPWrap::~UVGRTPWrap(){
}

void UVGRTPWrap::hdrpck(std::vector<std::byte>& pData, size_t channels, size_t nSamples, size_t nBytesInChannel0, size_t nBytesInChannel1)
{

    if (pData.size()) pData.clear();
    pData.resize(16);

    *reinterpret_cast<int*>(&pData[0]) = static_cast<int>(channels);
    *reinterpret_cast<int*>(&pData[4]) = static_cast<int>(nSamples);
    *reinterpret_cast<int*>(&pData[8]) = static_cast<int>(nBytesInChannel0);
    *reinterpret_cast<int*>(&pData[12]) = static_cast<int>(nBytesInChannel1);
}
std::tuple<size_t, size_t, size_t, size_t> UVGRTPWrap::hdrunpck(std::vector<std::byte>& outData, std::vector<std::byte>& pData)
{
    if (pData.size() < 16) return {0, 0, 0, 0};

    auto resetOut = [&outData]() { outData.clear(); };
    auto copyTheHeader =  [&outData, &pData, resetOut]() {
        resetOut();
        std::copy(pData.begin(), pData.begin() + 16, std::back_inserter(outData));
        pData.erase(pData.begin(), pData.begin() + 16);
    };
    auto unpackSize = [](std::byte*pByte, size_t index) -> size_t { return static_cast<size_t>(*reinterpret_cast<int*>(&pByte[index])); };
    auto unpackHeader = [&unpackSize](std::vector<std::byte>&hdr) { return std::make_tuple(unpackSize(hdr.data(), 0), unpackSize(hdr.data(), 4), unpackSize(hdr.data(), 8), unpackSize(hdr.data(), 12)); };
    auto resizeOutAndExplode = [&outData, &unpackHeader, &pData]() {
        auto [nChan, nSamp, channel0SzInBytes, channel1SzInBytes] = unpackHeader(outData);
        outData.resize(nChan * nSamp * sizeof(float) + 16);
        if (channel0SzInBytes + channel1SzInBytes != pData.size()){
            std::cout << "====== ERROR ============" << std::endl;
            std::cout << "Calc Data: " << channel0SzInBytes + channel1SzInBytes << " Data Size: " << pData.size() << std::endl;
            std::cout << "HEADER:" << std::endl;
            std::cout << "nChan: " << nChan << std::endl;
            std::cout << "nSamp: " << nSamp << std::endl;
            std::cout << "nBytesInChannel0: " << channel0SzInBytes << std::endl;
            std::cout << "nBytesInChannel1: " << channel1SzInBytes << std::endl;
        };
        return std::make_tuple(nChan, nSamp, channel0SzInBytes, channel1SzInBytes);
    };

    copyTheHeader();
    return resizeOutAndExplode();

}
