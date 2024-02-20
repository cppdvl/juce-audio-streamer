//
// Created by Julian Guarin on 15/02/24.
//

#ifndef AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
#define AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
#include <mutex>
#include <vector>
#include <algorithm>
#include "signalsslots.h"

namespace Utilities::Buffer
{
    class BlockSizeAdapter {

        std::recursive_mutex pushMutex;
        std::recursive_mutex popMutex;
    public:
        BlockSizeAdapter(size_t);
        BlockSizeAdapter(const BlockSizeAdapter& other)
            {
                std::unique_lock<std::recursive_mutex> lock(pushMutex);
                std::unique_lock<std::recursive_mutex> lock2(popMutex);
                internalBuffer = other.internalBuffer;
                outputBlockSize = other.outputBlockSize;
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
        void pop(std::vector<float>& buffer);

        /*! @brief Pop a buffer of data from the adapter.
         *  @param buffer The buffer to pop from the adapter.
         */
        void pop(float* buffer);

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
        void setOutputBlockSize(size_t sz);

        static void monoSplit(BlockSizeAdapter& bsaLeft, BlockSizeAdapter& bsaRight);

    private:
        std::vector<float> internalBuffer;
        size_t outputBlockSize;
    };
}


#endif //AUDIOSTREAMPLUGIN_BLOCKSIZEADAPTER_H
