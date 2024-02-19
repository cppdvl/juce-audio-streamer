//
// Created by Julian Guarin on 15/02/24.
//

#include "BlockSizeAdapter.h"


Utilities::Buffer::BlockSizeAdapter::BlockSizeAdapter(size_t sz) : outputBlockSize(sz) {}

void Utilities::Buffer::BlockSizeAdapter::push(const std::vector<float>& buffer) {
    std::unique_lock<std::recursive_mutex> lock(mutex);
    internalBuffer.insert(internalBuffer.end(), buffer.begin(), buffer.end());
}

void Utilities::Buffer::BlockSizeAdapter::push(const float* buffer, size_t size) {
    std::unique_lock<std::recursive_mutex> lock(mutex);
    internalBuffer.insert(internalBuffer.end(), buffer, buffer + size);
}


bool Utilities::Buffer::BlockSizeAdapter::pop(std::vector<float>& buffer) {

    if (internalBuffer.size() >= outputBlockSize)
    {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        auto itLast = internalBuffer.begin() + static_cast<int>(outputBlockSize);
        buffer.insert(buffer.end(), internalBuffer.begin(), itLast);
        internalBuffer.erase(internalBuffer.begin(), itLast);
        return true;
    }
    return false;
}

bool Utilities::Buffer::BlockSizeAdapter::pop(float* buffer, size_t size) {
    if (internalBuffer.size() >= outputBlockSize)
    {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        std::copy(internalBuffer.begin(), internalBuffer.begin() + outputBlockSize, buffer);
        internalBuffer.erase(internalBuffer.begin(), internalBuffer.begin() + outputBlockSize);
        return true;
    }
    return false;
}

void Utilities::Buffer::BlockSizeAdapter::setOutputBlockSize(size_t sz) {
    this->outputBlockSize = sz;
}

bool Utilities::Buffer::BlockSizeAdapter::isEmpty() const {
    return internalBuffer.empty();
}

bool Utilities::Buffer::BlockSizeAdapter::dataReady() const {
    return internalBuffer.size() >= outputBlockSize;
}

void Utilities::Buffer::BlockSizeAdapter::monoSplit(BlockSizeAdapter& bsaLeft, BlockSizeAdapter& bsaRight) {
    std::unique_lock<std::recursive_mutex> lock(bsaLeft.mutex);
    std::unique_lock<std::recursive_mutex> lock2(bsaRight.mutex);
    for (auto i = 0lu; i < bsaLeft.internalBuffer.size(); ++i)
    {
        auto& left = bsaLeft.internalBuffer[i];
        auto& right = bsaRight.internalBuffer[i];
        left = (left + right) / 2;
        right = left;
    }
}
size_t Utilities::Buffer::BlockSizeAdapter::size() const {
    return outputBlockSize;
}
