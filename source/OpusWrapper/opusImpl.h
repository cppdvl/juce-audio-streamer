//
// Created by Julian Guarin on 7/11/23.
//

#ifndef AUDIOSTREAMPLUGIN_OPUSIMPL_H
#define AUDIOSTREAMPLUGIN_OPUSIMPL_H

#include "opus.h"
#include "signalsslots.h"

#include <map>
#include <memory>
#include <sstream>
#include <iostream>


namespace OpusImpl
{
    struct CODEC;
    enum Result
    {
        OK,
        ERROR
    };

}
using SPEncoder         = std::shared_ptr<OpusEncoder>;
using SPDecoder         = std::shared_ptr<OpusDecoder>;
using SPCodec           = std::shared_ptr<OpusImpl::CODEC>;
using StrmIOCodec       = std::map<uint64_t, SPCodec>;
using EncodingResult    = std::tuple<OpusImpl::Result, std::vector<std::byte>, int>; // Result, EncodedData, EncodedDataSizeInBytes
using DecodingResult    = std::tuple<OpusImpl::Result, std::vector<std::byte>, int>; // Result

namespace OpusImpl
{

    struct EncoderDeallocator
    {
        void operator()(OpusEncoder * ptr)
        {
            if (ptr != nullptr)
            {
                opus_encoder_destroy(ptr);
            }
        }
    };
    struct DecoderDeallocator
    {
        void operator()(OpusDecoder * ptr)
        {
            if (ptr != nullptr)
            {
                opus_decoder_destroy(ptr);
            }
        }
    };
    struct CODECConfig
    {
        int32_t             mSampRate{48000};
        int                 mBlockSize{480};
        int                 mChannels{2};
        bool                voice{false};
        uint32_t            ownerID{0};

        CODECConfig() = default;
    };
    struct CODEC
    {
        int error{};
        int*const pError{&error};
        CODECConfig cfg;
        std::vector<SPEncoder> mEncs{};
        std::vector<SPDecoder> mDecs{};

        CODEC()
        {
            CODECConfig _cfg;
            mEncs = std::vector<SPEncoder>(std::vector(static_cast<size_t>(_cfg.mChannels & 1 ? (_cfg.mChannels + 1) >> 1 : _cfg.mChannels >> 1), std::shared_ptr<OpusEncoder>(opus_encoder_create(_cfg.mSampRate, 2, _cfg.voice ? OPUS_APPLICATION_VOIP : OPUS_APPLICATION_AUDIO, pError), EncoderDeallocator())));
            mDecs = std::vector(static_cast<size_t>(_cfg.mChannels & 1 ? (_cfg.mChannels + 1) >> 1 : _cfg.mChannels >> 1), std::shared_ptr<OpusDecoder>(opus_decoder_create(_cfg.mSampRate, 2, pError), DecoderDeallocator()));

            std::cout << "Created a CODEC with " << mEncs.size() << " encoders and " << mDecs.size() << " decoders" << std::endl;
        }
        CODEC(const CODEC&) = default;
        CODEC(const CODECConfig _cfg) : cfg (_cfg),
                                          mEncs(std::vector(static_cast<size_t>(_cfg.mChannels & 1 ? (_cfg.mChannels + 1) >> 1 : _cfg.mChannels >> 1), std::shared_ptr<OpusEncoder>(opus_encoder_create(_cfg.mSampRate, 2, _cfg.voice ? OPUS_APPLICATION_VOIP : OPUS_APPLICATION_AUDIO, pError), EncoderDeallocator()))),
                                          mDecs(std::vector(static_cast<size_t>(_cfg.mChannels & 1 ? (_cfg.mChannels + 1) >> 1 : _cfg.mChannels >> 1), std::shared_ptr<OpusDecoder>(opus_decoder_create(_cfg.mSampRate, 2, pError), DecoderDeallocator())))
        {
        }

        ~CODEC()
        {
            mEncs.clear();
            mDecs.clear();
        }

        std::tuple<OpusImpl::Result, std::vector<std::byte>, size_t> encodeChannel (float* pfPCM, const size_t encoderIndex);
        std::tuple<OpusImpl::Result, std::vector<float>, size_t> decodeChannel (std::byte* pEncodedData, size_t channelSizeInBytes, const size_t channelIndex);

        inline static DAWn::Events::Signal<uint32_t, const char*, float*>     sEncoderErr{};
        inline static DAWn::Events::Signal<uint32_t, const char*, std::byte*> sDecoderErr{};
    };



}



#endif //AUDIOSTREAMPLUGIN_OPUSIMPL_H
