//
// Created by Julian Guarin on 9/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_JITTERBUFFER_H
#define AUDIOSTREAMPLUGIN_JITTERBUFFER_H
#include <queue>
#include <mutex>
#include <optional>
#include <chrono>

struct RtpPacket {
    unsigned int& sequenceNumber;
    unsigned int& timestamp;
    std::vector<unsigned char>& payload; // This would hold the Opus-encoded data.
};

class JitterBuffer {
public:
    // Initializes the jitter buffer with a specified size in milliseconds.
    explicit JitterBuffer(std::chrono::milliseconds bufferSize);

    // Non-copyable and non-movable
    JitterBuffer(const JitterBuffer&) = delete;
    JitterBuffer& operator=(const JitterBuffer&) = delete;
    JitterBuffer(JitterBuffer&&) = delete;
    JitterBuffer& operator=(JitterBuffer&&) = delete;

    // Adds a packet to the jitter buffer. Thread-safe.
    void addPacket(const RtpPacket& packet);

    // Retrieves the next packet for playback. If no packet is due, std::nullopt is returned.
    // If a packet is late (beyond the acceptable jitter time), it will be discarded.
    std::optional<RtpPacket> getNextPacket();

    // Resets the buffer to its initial state. Thread-safe.
    void reset();

    // Sets a new buffer size. Can be used to adjust the buffer dynamically. Thread-safe.
    void setBufferSize(std::chrono::milliseconds newBufferSize);

    // Returns the current buffer size. Thread-safe.
    std::chrono::milliseconds getBufferSize() const;

private:
    mutable std::mutex bufferMutex; // For protecting access to the buffer and related data.
    std::queue<RtpPacket> buffer;   // Queue to store the packets.
    std::chrono::milliseconds bufferSize; // Size of the buffer in milliseconds.
    unsigned int lastSequenceNumber; // The sequence number of the last packet retrieved.

    // Other private member variables and helper functions would be defined here.
};

#endif //AUDIOSTREAMPLUGIN_JITTERBUFFER_H
