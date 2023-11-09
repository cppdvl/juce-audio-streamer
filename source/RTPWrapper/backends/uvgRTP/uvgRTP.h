//
// Created by Julian Guarin on 6/10/23.
//

#ifndef RTPWRAPPER_UVGRTP_H
#define RTPWRAPPER_UVGRTP_H

#include "RTPWrap.h"
#include <uvgrtp/lib.hh>

//Type aliases
using SpSess = std::shared_ptr<uvgrtp::session>;
using SpStrm = std::shared_ptr<uvgrtp::media_stream>;
using WpSess = std::weak_ptr<uvgrtp::session>;
using WpStrm = std::weak_ptr<uvgrtp::media_stream>;

using StrmIndex     = std::map<uint64_t, SpStrm>;
using SessIndex     = std::map<uint64_t, SpSess>;
using SessStrmIndex = std::map<uint64_t, StrmIndex>;
using StrmSessIndex = std::map<uint64_t, uint64_t>;

class UVGRTPWrap : public RTPWrap {
   
 public:

    // UVG implementation for initialization
    uint64_t Initialize() override;

    // UVG implementation for creating a session
    uint64_t CreateSession(const std::string& localEndPoint) override;

    // UVG implementation for creating a stream
    /*!
     *
     * @param sessionId
     * @param srcPort
     * @param direction
     * @return
     */
    uint64_t CreateStream(uint64_t sessionId, int srcPort, int direction) override;

    /*! @brief Create a stream with a specific configuration. However this one will try to configure an OpusEncoder/OpusDecoder.
     *
     * @param sessionId The session to create the stream in.
     * @param streamConfiguration A struct with stream configuration parameters
     * @return
     */
    uint64_t CreateStream(uint64_t sessionId, const RTPStreamConfig& streamConfiguration) override;


    // UVG implementation for destroying a stream
    bool DestroyStream(uint64_t streamId) override;

    // UVG implementation for destroying a session
    bool DestroySession(uint64_t sessionId) override;

    // UVG implementation for shutdown
    void Shutdown() override;

    // UVG implementation for getting a session
    SpSess GetSession(uint64_t sessionId);

    SpStrm GetStream(uint64_t streamId);

    static std::shared_ptr<UVGRTPWrap> GetSP(SPRTP p)
    {
        return std::dynamic_pointer_cast<UVGRTPWrap>(p);
    }

    /**
     * @brief Set the error handler callback.
     * @param streamId The handle of the stream to set the callback for.
     * @param pData A to a float vector.
     * @param size DataSize.
     * @return True if the frame was successfully pushed, false otherwise.
     */
    bool PushFrame (uint64_t streamId, std::vector<std::byte> pData) noexcept override;

    /**
     * @brief Use this for the stream input handler callback.
     * @param streamId
     * @return A vector of bytes.
     */
    std::vector<std::byte> GrabFrame (uint64_t streamId, std::vector<std::byte> pData   ) noexcept override;


    ~UVGRTPWrap() override;

 private:
     uvgrtp::context ctx;
     std::tuple<size_t, size_t, size_t, size_t> hdrunpck(std::vector<std::byte>& outData, std::vector<std::byte>&pData);
     void hdrpck(std::vector<std::byte>& pData,  size_t channels, size_t nSamples, size_t nBytesInChannel0 = 0, size_t nBytesInChannel1 = 0);
     std::vector<std::byte> encode(uint64_t streamId, std::vector<std::byte> data);
     std::vector<std::byte> decode(uint64_t streamId, std::vector<std::byte> data);

};

#endif //RTPWRAPPER_UVGRTP_H
