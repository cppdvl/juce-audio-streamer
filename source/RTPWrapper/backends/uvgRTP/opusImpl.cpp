//
// Created by Julian Guarin on 7/11/23.
//

#include "opusImpl.h"

std::tuple<OpusImpl::Result, std::vector<std::byte>, size_t> OpusImpl::CODEC::encodeChannel (float* pfPCM, const size_t channelIndex)
{
    auto blockSize = static_cast<size_t>(cfg.mBlockSize);
    if ((size_t)channelIndex >= mEncs.size())
    {
        std::cout << "Bad channel encoder index [" << channelIndex << "]" << std::endl;
        return EncodingResult (std::make_tuple (Result::ERROR, std::vector<std::byte>(), 0));
    }
    std::vector<std::byte> encodedBlock (blockSize, std::byte{0});
    auto refEnc = mEncs[(size_t)channelIndex];
    auto encodedBytes = opus_encode_float (refEnc.get(), pfPCM, cfg.mBlockSize, reinterpret_cast<unsigned char*>(encodedBlock.data()), cfg.mBlockSize);
    if (encodedBytes < 0)
    {
        std::cout << "Error Message: " << opus_strerror (encodedBytes) << std::endl;
        return EncodingResult (std::make_tuple (Result::ERROR, std::vector<std::byte>(), 0));
    }
    encodedBlock.resize ((size_t) encodedBytes);
    return EncodingResult (std::make_tuple (Result::OK, encodedBlock, encodedBytes));
}

std::tuple<OpusImpl::Result, std::vector<float>, size_t> OpusImpl::CODEC::decodeChannel (std::byte* pEncodedData, size_t channelSizeInBytes, const size_t channelIndex)
{
    auto blockSize = static_cast<size_t>(cfg.mBlockSize);
    auto i32DataSize = static_cast<int32_t>(channelSizeInBytes);
    std::vector<float> decodedData (blockSize, 0.0f);
    auto pfPCM = decodedData.data();

    if (channelIndex >= mDecs.size())
    {
        std::cout << "Bad channel decoder index [" << channelIndex  << "]" << std::endl;
        return std::make_tuple(Result::ERROR, std::vector<float>{}, 0);
    }

    auto refDecoder = mDecs[channelIndex];

    auto decodedSamples = opus_decode_float(
        refDecoder.get(),
        reinterpret_cast<unsigned char*>(pEncodedData),
        i32DataSize,
        pfPCM, cfg.mBlockSize, 0);

    if (decodedSamples < 0)
    {
        std::cout << "Error Message: " << opus_strerror(decodedSamples) << std::endl;
        return std::make_tuple(Result::ERROR, std::vector<float>{}, 0);
    }
    else if ((size_t)decodedSamples > blockSize)
    {
        std::cout << "Error: decoded samples [" << decodedSamples << "] > block size [" << blockSize << "]" << std::endl;
        return std::make_tuple(Result::ERROR, std::vector<float>{}, 0);
    }

    return std::make_tuple(Result::OK, decodedData, decodedSamples);
}