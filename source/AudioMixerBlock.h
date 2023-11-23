//
// Created by Julian Guarin on 17/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
#define AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H

#include <map>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace Mixer
{
    using Block = std::vector<float>;
    using Column = std::vector<Block>;
    using Row = std::map<int64_t, Block>;

    /*!
     * @brief The AudioMixerBlock class.
     * @details This class is a mixer that can mix audio blocks from different sources. The AudioMixer is a map of bi-blocks, where each block is a pile of audio blocks from different sources, ata given time to provide syncronization.
     *
     *
     *
     * @note This class is thread safe.
     */
    class AudioMixerBlock : public std::map<int64_t, Column>
    {
        std::recursive_mutex data_mutex;

        inline static const size_t BlockSize = 480;
        std::unordered_map<int32_t, size_t> sourceIDToColumnIndex {{0, 0}};
        Row playbackData{{0, Block (BlockSize, 0.0f)}};


        /*!
         * @brief Add a source to the mixer.
         * @param sourceID
         */
        void addSource(int32_t sourceId, int64_t time = -1, const Block& block = Block(BlockSize, 0.0f));
        void addColumn(int64_t time);

    public:
        AudioMixerBlock() : std::map<int64_t , Column>{
            {-1, Column(1, Block(BlockSize, 0.0f))},
            {0, Column(1, Block(BlockSize, 0.0f))}}
        {}

        /*!
         * @brief Add a block to the mixer, from any given source, at any given time.
         *
         * @param sourceID The source ID.
         * @param time The time in samples.
         * @param block The block to add.
         */
        void mix (int64_t time, const Block& block, int32_t sourceID = 0);

        /*!
         * @brief Get the block at a given time.
         *
         * @param time The time in samples.
         * @return The block at the given time.
         */
        Block& getBlock(int64_t time);

        Column getStreamColumn(int64_t timeIndex);

    };




}


#endif //AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
