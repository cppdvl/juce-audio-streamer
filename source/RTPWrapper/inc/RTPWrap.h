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

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string>
#include <cstdint> // Include for uint64_t
#include <functional>

struct RTPStreamConfig;
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
     * @param sessionId The handle of the session to create the stream in.
     * @param streamConfig The configuration for the stream.
     * @return uint64_t A handle for the created stream.
     */
    virtual uint64_t CreateStream(uint64_t sessionId, const RTPStreamConfig& streamConfig) = 0;

    /**
     * @brief Create a stream and return a stream handle.
     * @param srcPort The source port for the stream.
     * @param direction 1 input / 0 for output. Fast way to remember 1nput 0utput.
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
     * @param streamId The handle of the stream to set the callback for.
     * @param pData A to a general data vector.
     * @param size DataSize.
     * @return
     */
    virtual bool PushFrame (uint64_t streamId, std::vector<std::byte> pData) noexcept = 0;

    /**
     * @brief Use this for the stream input handler callback.
     * @param streamId
     * @return
     */

    virtual std::vector<std::byte> GrabFrame (uint64_t streamId, std::vector<std::byte> pData) noexcept = 0;
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

using SPRTP = std::shared_ptr<RTPWrap>;

struct RTPStreamConfig
{
    uint16_t    mPort;       //!The port of the stream. (udp)
    int32_t     mSampRate;    //!The sampling rate of the stream (hz).
    int         mBlockSize;
    int         mChannels;   //!The number of channels in the stream. 1/2
    int         mDirection;  //! 1 input / 0 for output. Fast way to remember 1nput 0utput.
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

