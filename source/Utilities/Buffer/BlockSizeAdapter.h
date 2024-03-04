//
// Created by Julian Guarin on 15/02/24.
//

#ifndef AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
#define AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H

#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <algorithm>

#include "signalsslots.h"

namespace Utilities::Buffer
{

    class BlockSizeAdapter
    {
        uint32_t mTimeStamp{0xdeadbeef};
        uint32_t mTimeStampStep{0};
        std::recursive_mutex internalBufferMutex;
    public:
        BlockSizeAdapter(size_t sz) : outputBlockSize(sz) {}
        BlockSizeAdapter(size_t sz, size_t channs) : mTimeStampStep(static_cast<uint32_t>(sz)), outputBlockSize(channs * sz) {}
        BlockSizeAdapter(const BlockSizeAdapter& other)
            {
                std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
                internalBuffer = other.internalBuffer;
                outputBlockSize = other.outputBlockSize;
                mTimeStamp = other.mTimeStamp;
                mTimeStampStep = other.mTimeStampStep;
                lock.unlock();

            }

        /*! @brief Push a buffer of data into the adapter.
         *  @param buffer The buffer to push into the adapter.
         */
        void push(const std::vector<float>& buffer);

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

        static void monoSplit(BlockSizeAdapter& bsaLeft, BlockSizeAdapter& bsaRight);

    private:
        std::vector<float> internalBuffer;
        size_t outputBlockSize;
    };

}


#endif //AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
