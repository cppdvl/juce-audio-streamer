//
// Created by Julian Guarin on 15/02/24.
//

#ifndef AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
#define AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H

#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <iomanip>
#include <iostream>
#include <algorithm>

#include "signalsslots.h"

namespace Utilities::Buffer
{

    class BlockSizeAdapter
    {
        size_t peekAt{0};
        size_t writeAt{0};
        uint32_t mTimeStamp{0x0};
        uint32_t mTimeStampStep{0};
        std::recursive_mutex internalBufferMutex;
    public:

        void print(std::string msg)
        {
            std::stringstream ss;
            ss << msg << " [" << mTimeStampStep << ", " << writeAt << ", " << peekAt << ", " << outputBlockSize << "]";
            std::cout << "@" << std::hex << ((uint64_t)this & 0xFFFFFFFF) << " BSA (" << std::dec << outputBlockSize << "): " << ss.str() << std::endl;
        }

        BlockSizeAdapter(size_t sz) : outputBlockSize(sz) {}
        BlockSizeAdapter(size_t sz, size_t channs) : mTimeStampStep(static_cast<uint32_t>(sz)), outputBlockSize(channs * sz) {}
        BlockSizeAdapter(const BlockSizeAdapter& other)
        {
            mTimeStamp = other.mTimeStamp;
            mTimeStampStep = other.mTimeStampStep;
            writeAt = other.writeAt;
            peekAt = other.peekAt;
            outputBlockSize = other.outputBlockSize;
        }

        ~BlockSizeAdapter()
        {
            std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
            internalBuffer.reset();
            lock.unlock();
        }

        /*! @brief Push a buffer of data into the adapter.
         *  @param buffer The buffer to push into the adapter.
         */
        void push(const std::vector<float>& buffer, uint32_t tsample);

        /*! @brief Push a buffer of data into the adapter.
         *  @param buffer The buffer to push into the adapter.
         *  @param size The size of the buffer.
         */
        void push(const float* buffer, size_t size);

        /*! @brief Pop a buffer of data from the adapter.
         *  @param buffer The buffer to pop from the adapter.
         */
        void pop(std::vector<float>& buffer, uint32_t& tsample);

        /*! @brief Pop a buffer of data from the adapter.
         *  @param buffer The buffer to pop from the adapter.
         */
        void pop(float* buffer, uint32_t& tsample);

        /*! @brief Check if the adapter is empty.
         *  @return True if the adapter is empty, false otherwise.
         */
        bool isEmpty() const;

        /*! @brief Check if the adapter is ready to be read.
         *  @return True if the adapter is ready to be read, false otherwise.
         */
        bool dataReady() const;

        /*! @brief Set the output block size.
         *  @param sz The output block size.
         */
        void setChannelsAndOutputBlockSize(size_t channs, size_t sz);

        /*!
         * @brief Set the time stamp
         * @param tsample
         * @param flush, flush the internal buffer
         */
        void setTimeStamp(uint32_t tsample, bool flush = true);


    private:
        const size_t MAXROUNDBUFFERSIZE = 48000 * 2 * 10;
        std::unique_ptr<float[]> internalBuffer{new float[MAXROUNDBUFFERSIZE]};
        size_t outputBlockSize;

    };

}


#endif //AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
