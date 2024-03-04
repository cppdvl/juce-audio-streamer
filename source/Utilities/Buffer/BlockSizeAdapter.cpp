//
// Created by Julian Guarin on 15/02/24.
//

#include "BlockSizeAdapter.h"


void Utilities::Buffer::BlockSizeAdapter::push(const std::vector<float>& buffer)
{
    push(buffer.data(), buffer.size());
}

void Utilities::Buffer::BlockSizeAdapter::push(const float* buffer, size_t size)
{
    {
        std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
        internalBuffer.insert(internalBuffer.end(), buffer, buffer + size);
    }
}


void Utilities::Buffer::BlockSizeAdapter::pop(std::vector<float>& buffer, uint32_t& timeStamp)
{
    return pop(buffer.data(), timeStamp);
}

void Utilities::Buffer::BlockSizeAdapter::pop(float* buffer, uint32_t& timeStamp)
{
    std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
    timeStamp = mTimeStamp;
    mTimeStamp += mTimeStampStep;
    auto itEnd = internalBuffer.begin() + static_cast<long>(outputBlockSize);
    std::copy(internalBuffer.begin(), itEnd, buffer);
    internalBuffer.erase(internalBuffer.begin(), itEnd);
}

void Utilities::Buffer::BlockSizeAdapter::setChannelsAndOutputBlockSize (size_t channs, size_t sz)
{
    std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);

    this->mTimeStampStep = static_cast<uint32_t >(sz);
    this->outputBlockSize = channs * sz;
}

bool Utilities::Buffer::BlockSizeAdapter::isEmpty() const
{
    return internalBuffer.empty();
}

bool Utilities::Buffer::BlockSizeAdapter::dataReady() const
{
    return internalBuffer.size() >= outputBlockSize;
}

void Utilities::Buffer::BlockSizeAdapter::monoSplit(BlockSizeAdapter& bsaLeft, BlockSizeAdapter& bsaRight) {
    std::unique_lock<std::recursive_mutex> lockLeft (bsaLeft.internalBufferMutex, std::defer_lock);
    std::unique_lock<std::recursive_mutex> lockRight (bsaRight.internalBufferMutex, std::defer_lock);
    std::lock(lockLeft, lockRight);
    for (auto i = 0lu; i < bsaLeft.internalBuffer.size(); ++i)
    {
        auto& left = bsaLeft.internalBuffer[i];
        auto& right = bsaRight.internalBuffer[i];
        left = (left + right) / 2;
        right = left;
    }
}

void Utilities::Buffer::BlockSizeAdapter::setTimeStamp(uint32_t tsample, bool flush)
{
    std::unique_lock<std::recursive_mutex> lock(internalBufferMutex);
    mTimeStamp = tsample;
    if (flush) internalBuffer.clear();
}