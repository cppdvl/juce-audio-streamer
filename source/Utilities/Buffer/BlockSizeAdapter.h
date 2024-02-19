//
// Created by Julian Guarin on 15/02/24.
//

#ifndef AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
#define AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
#include <mutex>
#include <vector>
#include <algorithm>

namespace Utilities::Buffer
{
    class BlockSizeAdapter {

        std::recursive_mutex mutex;

    public:
        BlockSizeAdapter(size_t);


        void push(const std::vector<float>& buffer);
        void push(const float* buffer, size_t size);

        bool pop(std::vector<float>& buffer);
        bool pop(float* buffer, size_t size);
        bool isEmpty() const;
        bool dataReady() const;
        size_t size() const;
        void setOutputBlockSize(size_t outputBlockSize);
        static void monoSplit(BlockSizeAdapter& bsaLeft, BlockSizeAdapter& bsaRight);
    private:
        std::vector<float> internalBuffer;
        size_t outputBlockSize;
    };
}


#endif //AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
