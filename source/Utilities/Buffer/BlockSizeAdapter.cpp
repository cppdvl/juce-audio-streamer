//
// Created by Julian Guarin on 15/02/24.
//

#include "BlockSizeAdapter.h"


Utilities::Buffer::BlockSizeAdapter::BlockSizeAdapter(size_t sz) : outputBlockSize(sz) {}

void Utilities::Buffer::BlockSizeAdapter::push(const std::vector<float>& buffer) {
    std::unique_lock<std::recursive_mutex> lock(mutex);
    internalBuffer.insert(internalBuffer.end(), buffer.begin(), buffer.end());
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

void Utilities::Buffer::BlockSizeAdapter::setOutputBlockSize(size_t sz) {
    this->outputBlockSize = sz;
}

bool Utilities::Buffer::BlockSizeAdapter::isEmpty() const {
    return internalBuffer.empty();
}

