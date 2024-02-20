//
// Created by Julian Guarin on 15/02/24.
//

#include "BlockSizeAdapter.h"


Utilities::Buffer::BlockSizeAdapter::BlockSizeAdapter(size_t sz) : outputBlockSize(sz) {}

void Utilities::Buffer::BlockSizeAdapter::push(const std::vector<float>& buffer){
    push(buffer.data(), buffer.size());
}

void Utilities::Buffer::BlockSizeAdapter::push(const float* buffer, size_t size) {
    {
        std::unique_lock<std::recursive_mutex> lock(pushMutex);
        internalBuffer.insert(internalBuffer.end(), buffer, buffer + size);
    }
    /*{
        std::unique_lock<std::recursive_mutex> lock(popMutex);
        while (internalBuffer.size() >= outputBlockSize)
        {
            auto itEnd = internalBuffer.begin() + static_cast<long>(outputBlockSize);
            std::vector<float> exportBuffer(std::move_iterator(internalBuffer.begin()),
                                            std::move_iterator(itEnd));
            internalBuffer.erase(internalBuffer.begin(), itEnd);
            dataReadySignal.Emit(exportBuffer);
        }
    }*/
}


void Utilities::Buffer::BlockSizeAdapter::pop(std::vector<float>& buffer) {
    return pop(buffer.data());
}

void Utilities::Buffer::BlockSizeAdapter::pop(float* buffer) {
    std::unique_lock<std::recursive_mutex> lock(popMutex);
    auto itEnd = internalBuffer.begin() + static_cast<long>(outputBlockSize);
    std::copy(internalBuffer.begin(), itEnd, buffer);
    internalBuffer.erase(internalBuffer.begin(), itEnd);
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
    std::unique_lock<std::recursive_mutex> lock(bsaLeft.popMutex);
    std::unique_lock<std::recursive_mutex> lock2(bsaRight.popMutex);
    for (auto i = 0lu; i < bsaLeft.internalBuffer.size(); ++i)
    {
        auto& left = bsaLeft.internalBuffer[i];
        auto& right = bsaRight.internalBuffer[i];
        left = (left + right) / 2;
        right = left;
    }
}
