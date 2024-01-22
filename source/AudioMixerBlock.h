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
    constexpr size_t BlockSize = 480;

    Block SubBlocks(const Block& a, const Block& b);
    Block AddBlocks(const Block& a, const Block& b);

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

        std::unordered_map<int32_t, size_t> sourceIDToColumnIndex {{0, 0}};
        Row playbackDataBlock {{0, Block (BlockSize, 0.0f)}};
        void layoutCheck(int64_t time, int32_t sourceID);

        /*!
         * @brief Add a source to the mixer.
         * @param sourceID
         */
        void addSource(int32_t sourceId);
        void addColumn(int64_t time);

        /*!
         * @brief Add a audioBlock to the mixer, from any given source, at any given time.
         *
         * @param time The time in samples.
         * @param audioBlock The audio block to mix.
         * @param sourceID The source ID.
         * @param blocksToStream The blocks to stream.
         */
        void mix (int64_t time, const Block audioBlock, int32_t sourceID, std::unordered_map<int32_t, std::vector<Block>>& blocksToStream);

    public:
        AudioMixerBlock() : std::map<int64_t , Column>{
            {0, Column(1, Block(BlockSize, 0.0f))}}
        {}

        /*!
         * @brief Mix a block from a given source at a given time.
         * @param Vector Mixing Blocks (one per channel).
         * @param time The timestamp in samples.
         * @param splittedBlocks The blocks to mix.
         * @param sourceID The source ID. By default 0 if not provided (the local audio header).
         */
        static void mix (std::vector<AudioMixerBlock>& mixers, int64_t time, const std::vector<Block>& splittedBlocks, int32_t sourceID = 0);


        /*!
         * @brief Get the block at a given time.
         *
         * @param time The time in samples.
         * @return The block at the given time.
         */
        Block getBlock(int64_t time);

        const Row& getPlaybackDataBlock();

    };




}


#endif //AUDIOSTREAMPLUGIN_AUDIOMIXERBLOCK_H
