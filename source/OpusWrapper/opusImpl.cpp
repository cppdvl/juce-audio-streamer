//
// Created by Julian Guarin on 7/11/23.
//

#include "opusImpl.h"

std::tuple<OpusImpl::Result, std::vector<std::byte>, size_t> OpusImpl::CODEC::encodeChannel (float* pfPCM, const size_t encoderIndex)
{
    auto blockSize = static_cast<size_t>(8 * cfg.mBlockSize * cfg.mChannels);
    if ((size_t) encoderIndex >= mEncs.size())
    {
        std::stringstream  ss;
        ss << "Bad channel encoder index [" << encoderIndex << "]" << std::endl;
        OpusImpl::CODEC::sEncoderErr.Emit(cfg.ownerID, ss.str().c_str(), pfPCM);
        return EncodingResult (std::make_tuple (Result::ERROR, std::vector<std::byte>(), 0));
    }
    std::vector<std::byte> encodedBlock (blockSize, std::byte{0});
    auto refEnc = mEncs[(size_t) encoderIndex];

    auto encodedBytes = opus_encode_float (
        refEnc.get(),
        pfPCM,
        cfg.mBlockSize,
        reinterpret_cast<unsigned char*>(encodedBlock.data()),
        static_cast<int32_t>(blockSize));

    if (encodedBytes < 0)
    {
        std::stringstream ss;
        ss << "Error Message: " << opus_strerror (encodedBytes) << std::endl;
        OpusImpl::CODEC::sEncoderErr.Emit(cfg.ownerID, ss.str().c_str(), pfPCM);
        return EncodingResult (std::make_tuple (Result::ERROR, std::vector<std::byte>(), 0));
    }

    encodedBlock.resize ((size_t) encodedBytes);
    return EncodingResult (std::make_tuple (Result::OK, encodedBlock, encodedBytes));
}

std::tuple<OpusImpl::Result, std::vector<float>, size_t> OpusImpl::CODEC::decodeChannel (std::byte* pEncodedData, size_t channelSizeInBytes, const size_t channelIndex)
{
    auto maxDecodedBlockSize = static_cast<size_t>(cfg.mBlockSize * cfg.mChannels);
    auto i32DataSize = static_cast<int32_t>(channelSizeInBytes);
    std::vector<float> decodedData (maxDecodedBlockSize, 0.0f);
    auto pfPCM = decodedData.data();

    if (channelIndex >= mDecs.size())
    {
        std::stringstream ss;
        ss << "Bad channel decoder index [" << channelIndex  << "]";
        OpusImpl::CODEC::sDecoderErr.Emit(cfg.ownerID, ss.str().c_str(), pEncodedData);
        return std::make_tuple(Result::ERROR, std::vector<float>{}, 0);
    }

    auto refDecoder = mDecs[channelIndex];

    auto decodedSamples = opus_decode_float(
        refDecoder.get(),
        reinterpret_cast<unsigned char*>(pEncodedData),
        i32DataSize,
        pfPCM,
        cfg.mBlockSize, 0);
    auto decodedBlockSize = static_cast<size_t>(decodedSamples * cfg.mChannels);

    if (decodedSamples < 0)
    {
        std::stringstream ss;
        ss << "Error Message: " << opus_strerror(decodedSamples);
        OpusImpl::CODEC::sDecoderErr.Emit(cfg.ownerID, ss.str().c_str(), pEncodedData);
        return std::make_tuple(Result::ERROR, std::vector<float>{}, 0);
    }
    else if ((size_t)decodedBlockSize > maxDecodedBlockSize)
    {
        std::stringstream ss;
        ss << "Error: decoded samples [" << decodedSamples << ":" << decodedBlockSize << " bytes]  max block size [" << maxDecodedBlockSize << "]";
        OpusImpl::CODEC::sDecoderErr.Emit(cfg.ownerID, ss.str().c_str(), pEncodedData);
        return std::make_tuple(Result::ERROR, std::vector<float>{}, 0);
    }

    return std::make_tuple(Result::OK, decodedData, decodedBlockSize);
}