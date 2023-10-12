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

#include <string>
#include <cstdint> // Include for uint64_t
#include <functional>

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
     * @param destPort The destination port for the stream.
     * @return uint64_t A handle for the created stream.
     */
    virtual uint64_t CreateStream(uint64_t sessionId, int srcPort, int destPort) = 0;

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
     * @brief Shutdown the RTP wrapper.
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

#endif /* defined(__RTPWrap__) */

