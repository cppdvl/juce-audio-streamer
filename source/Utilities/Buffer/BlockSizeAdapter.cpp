//
// Created by Julian Guarin on 15/02/24.
//

#include "BlockSizeAdapter.h"


void Utilities::Buffer::BlockSizeAdapter::push(const std::vector<float>& buffer, uint32_t tsample)
{
    if (tsample <= mTimeStamp)
    {
        //RESET THE BUFFER in case the time stamp tp write is less than the current time stamp for read.
        std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
        peekAt = 0; writeAt = 0;
        mTimeStamp = tsample;
    }
    push(buffer.data(), buffer.size());
}

void Utilities::Buffer::BlockSizeAdapter::push(const float* buffer, size_t size)
{
    {
        std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
        if (!internalBuffer)
        {
            return;
        }
        auto internalBufferData = internalBuffer.get();
        auto dstPtr = &internalBufferData[writeAt];
        if (writeAt + size >= MAXROUNDBUFFERSIZE)
        {
            auto remaining = MAXROUNDBUFFERSIZE - writeAt;
            std::copy(buffer, buffer + remaining, dstPtr);
            dstPtr = internalBufferData;
            std::copy(buffer + remaining, buffer + size, dstPtr);
            writeAt = size - remaining;

        }
        else
        {
            std::copy(buffer, buffer + size, dstPtr);
            writeAt += size;
        }
    }
}


void Utilities::Buffer::BlockSizeAdapter::pop(std::vector<float>& buffer, uint32_t& timeStamp)
{
    return pop(buffer.data(), timeStamp);
}

void Utilities::Buffer::BlockSizeAdapter::pop(float* buffer, uint32_t& timeStamp)
{

    std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
    if (!internalBuffer)
    {
        return;
    }

    timeStamp = mTimeStamp;
    mTimeStamp += mTimeStampStep;
    auto internalBufferData = internalBuffer.get();
    auto srcPtr = &internalBufferData[peekAt];
    if (peekAt + outputBlockSize >= MAXROUNDBUFFERSIZE)
    {
        auto remaining = MAXROUNDBUFFERSIZE - peekAt;
        std::copy(srcPtr, srcPtr + remaining, buffer);
        std::copy(internalBufferData, internalBufferData + outputBlockSize - remaining, buffer + remaining);
        peekAt = outputBlockSize - remaining;
    }
    else
    {
        std::copy(srcPtr, srcPtr + outputBlockSize, buffer);
        peekAt += outputBlockSize;
    }
}

void Utilities::Buffer::BlockSizeAdapter::setChannelsAndOutputBlockSize (size_t channs, size_t sz)
{
    std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
    if (!internalBuffer)
    {
        return;
    }
    this->mTimeStampStep = static_cast<uint32_t >(sz);
    this->outputBlockSize = channs * sz;
}

bool Utilities::Buffer::BlockSizeAdapter::isEmpty() const
{
    return peekAt == writeAt;
}

bool Utilities::Buffer::BlockSizeAdapter::dataReady() const
{
    auto peekAtEnd = peekAt + outputBlockSize;
    peekAtEnd %= MAXROUNDBUFFERSIZE;
    return peekAtEnd < writeAt;
}


void Utilities::Buffer::BlockSizeAdapter::setTimeStamp(uint32_t tsample, bool flush)
{
    std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
    mTimeStamp = tsample;
    if (flush)
    {
        peekAt = 0;
        writeAt = 0;
    }
}