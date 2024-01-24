/** 
 * @file RTPWrap.h
 * @brief C++ RTP Wrapper
 * @author cppdvl
 * @date 2023
 * @license MIT License
 * 
 * @details
 * This is a C++ wrapper class for managing RTP (Real-time Transport Protocol) streams and sessions.
 * It provides an interface for initializing, creating sessions, streams, setting callbacks, and more.
 * The functions return uint64_t handles to uniquely identify sessions and streams.
 * The Interface design aims to be a common one, so different backends could be used underneath.
 *
 *
 *
 */

#ifndef __RTPWrap__
#define __RTPWrap__


#include <map>
#include <random>
#include <string>
#include <cstdint>
#include <functional>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "opusImpl.h"




class RTPWrap {

    public:
    using Handler       = std::function<void(const uint64_t)>;
    using ErrHandler    = Handler;
    using RcvHandler    = Handler;
    using TrxHandler    = Handler;
    /**
     * @brief Initialize the RTP wrapper.
     * @return uint64_t A handle for the initialized wrapper.
     */
    virtual uint64_t Initialize() = 0;

    /**
     * @brief Create a session and return a session handle.
     * @param localEndPoint The local endpoint to bind to.
     * @return uint64_t A handle for the created session.
     */
    virtual uint64_t CreateSession(const std::string& localEndPoint) = 0;

    /**
     * @brief Create a stream and return a stream handle.
     * @param srcPort The source port for the stream.
     * @param direction 1 input / 0 for output. Fast way to remember 1nput 0utput. 2 bidirectional.
     * @return uint64_t A handle for the created stream.
     */
    virtual uint64_t CreateStream(uint64_t sessionId, int srcPort, int direction) = 0;

    /**
     * @brief Destroy a stream by its handle.
     * @param streamId The handle of the stream to destroy.
     * @return bool True if the stream was successfully destroyed, false otherwise.
     */
    virtual bool DestroyStream(uint64_t streamId) = 0;

    /**
     * @brief Destroy a session by its handle.
     * @param sessionId The handle of the session to destroy.
     * @return bool True if the session was successfully destroyed, false otherwise.
     */
    virtual bool DestroySession(uint64_t sessionId) = 0;

    /**
     * @brief Set the error handler callback.
     * @param pData A to a general data vector.
     * @param streamId The handle of the stream to set the callback for.
     * @param size DataSize.
     * @return
     */
    virtual bool PushFrame (std::vector<std::byte> pData, uint64_t streamId, uint32_t timestamp) = 0;

    /**
     * @brief Shutdown the RTP wrapper.
     *
     */

    virtual void Shutdown() = 0;

    /**
     * @brief Destructor.
     */
    virtual ~RTPWrap() {}

    protected:
    
    ErrHandler  mErrHandler;
    RcvHandler  mRcvHandler;
    TrxHandler  mTrxHandler;

};

//Type aliases
#if defined(uvgRTP)
#elif defined(udpRTP)
#include "udpRTP.h"
#endif


namespace _rtpwrap::data
{
    using SpSess            = sessiontoken*;
    using SpStrm            = std::shared_ptr<streamtoken>;
    using WpSess            = std::weak_ptr<sessiontoken>;
    using WpStrm            = std::weak_ptr<streamtoken>;

    using StrmIndex         = std::map<uint64_t, SpStrm>;
    using SessIndex         = std::map<uint64_t, SpSess>;
    using SessStrmIndex     = std::map<uint64_t, StrmIndex>;
    using StrmSessIndex     = std::map<uint64_t, uint64_t>;

    static SessStrmIndex    masterIndex{};   //SessionId -> Map(StreamId -> Shared Pointer to Stream)
    static SessIndex        sessionIndex{};  //SessionId -> Shared Pointer to Session
    static StrmSessIndex    streamIndex{};   //StreamId -> SessionId.
    static StrmIOCodec      streamIOCodec{}; //StreamId -> Codec.
    /*! @brief Get the stream from the session index.
         *
         * @param sessionId
         * @param streamId
         * @return A shared Ptr to the Stream.nullptr will be returned if the stream in not found.
         */
    SpStrm GetStream(uint64_t sessionId, uint64_t streamId);
    /*! @brief Get the stream from the stream index.
         *
         * @param streamId
         * @return A shared Ptr to the Stream.nullptr will be returned if the stream in not found.
         */
    SpStrm GetStream(uint64_t streamId);

    /*! @brief Get the session from the session index.
         *
         * @param sessionId
         * @return A shared Ptr to the Session.nullptr will be returned if the session in not found.
         */
    SpSess GetSession(uint64_t sessionId);

    /*! @brief Index the stream in the session index.
         *
         * @param sessionId
         * @param stream
         * @return The streamId of the indexed stream. 0 will be returned if the stream is null.
         */
    uint64_t IndexStream(uint64_t sessionId, SpStrm stream);

    /*! @brief Index the session in the session index.
         *
         * @param session
         * @return The sessionId of the indexed session. 0 will be returned if the session is null.
         */
    uint64_t IndexSession(SpSess session);

    /*! @brief Remove the stream from the session index.
         *
         * @param sessionId
         * @param streamId
         * @return true if the stream was removed. false if the stream was not found.
         */
    bool RemoveStream(uint64_t sessionId, uint64_t streamId);
    bool RemoveStream(uint64_t streamId);


    /*! @brief Remove the session from the session index.
         *
         * @param sessionId
         * @return true if the session was removed. false if the session was not found.
         */
    bool RemoveSession(uint64_t sessionId);



}



using SPRTP = std::shared_ptr<RTPWrap>;

struct RTPStreamConfig
{
    uint16_t    mPort;       //!The port of the stream. (udp)
    int32_t     mSampRate;    //!The sampling rate of the stream (hz).
    int         mBlockSize;
    int         mChannels;   //!The number of channels in the stream. 1/2
    int         mDirection;  //! 1 input / 0 for output. Fast way to remember 1nput 0utput.
    bool&       mUseOpus;    //!Use Opus for encoding/decoding.
    RTPStreamConfig(bool& opusFlag) : mUseOpus(opusFlag) {}
};
class RTPWrapUtils
{
public:

    inline static bool udpPortIsInUse (int port)
    {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            return false;
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        int bindResult = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
        close(sockfd);

        return bindResult == -1;
    }
    inline static bool udpPortIsFree(int port)
    {
        return !udpPortIsInUse(port);
    }
};


#endif /* defined(__RTPWrap__) */

