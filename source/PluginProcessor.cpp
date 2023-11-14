#include "PluginProcessor.h"
#include "PluginEditor.h"


#include "opusImpl.h"

#include <cstddef>
#include <thread>


//==============================================================================




AudioStreamPluginProcessor::AudioStreamPluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{





}

AudioStreamPluginProcessor::~AudioStreamPluginProcessor()
{
}

//==============================================================================
const juce::String AudioStreamPluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioStreamPluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioStreamPluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioStreamPluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioStreamPluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioStreamPluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioStreamPluginProcessor::getCurrentProgram()
{
    return 0;
}

void AudioStreamPluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioStreamPluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioStreamPluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioStreamPluginProcessor::prepareToPlay (double , int )
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //Set 48k (More Suitable for Opus according to documentation)
    // I removed forcing the sample rate to 48k, because it was causing issues with the graphical interface and I had no certainty about the real size of the buffer and its duration.
    toneGenerator.prepareToPlay(480, 48000);
    channelInfo.buffer = nullptr;
    channelInfo.startSample = 0;

    pCodecConfig = std::make_unique<RTPStreamConfig>(useOpus);

    auto numChan =          getTotalNumInputChannels();
    pCodecConfig->mSampRate =   48000;
    pCodecConfig->mBlockSize =  480;
    pCodecConfig->mChannels =   numChan;

    pOpusCodec = std::make_shared<OpusImpl::CODEC>(*pCodecConfig);

}

void AudioStreamPluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    toneGenerator.releaseResources();
}

bool AudioStreamPluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
juce::AudioBuffer<float> AudioStreamPluginProcessor::processBlockStreamInNaive(
    juce::AudioBuffer<float>& processBlockBuffer,
        juce::MidiBuffer&)
{
    juce::AudioBuffer<float> inStreamBuffer(processBlockBuffer.getNumChannels(), processBlockBuffer.getNumSamples());
    inStreamBuffer.clear();

    auto naiveUnPackErrorControl = [this, &inStreamBuffer]() -> std::tuple<int, int, int, int> {

        std::lock_guard<std::mutex> lock (mMutexInput);
        auto naiveUnPack = [this](void* ptr, int byteSize) -> bool
        {
            auto p = reinterpret_cast<std::byte*>(ptr);
            for (auto sz = 0l; sz < byteSize; ++sz)
            {
                if (mInputBuffer.empty())
                    return false;
                *reinterpret_cast<std::byte*>(p + sz) = mInputBuffer.front();
                mInputBuffer.pop_front();
            }
            return true;
        };

        int nChan = 0;
        int nSampl  = 0;
        std::vector<size_t> nChanSz{};
        int nChan0Sz = 0;
        int nChan1Sz = 0;
        if (!naiveUnPack (&nChan, sizeof (int)))
            return std::make_tuple (0, 0, 0, 0);
        if (!naiveUnPack (&nSampl, sizeof (int)))
            return std::make_tuple (0, 0, 0, 0);
        if (!naiveUnPack (&nChan0Sz, sizeof (int)))
            return std::make_tuple (0, 0, 0, 0);
        if (!naiveUnPack (&nChan1Sz, sizeof (int)))
            return std::make_tuple (0, 0, 0, 0);

        nChanSz.push_back(16);
        nChanSz.push_back((size_t)nChan0Sz);
        nChanSz.push_back((size_t)nChan1Sz);

        int bufferSize = nChan1Sz + nChan0Sz;



        std::vector<float*> channelPtrs((size_t)nChan, nullptr);
        for (auto index = 0; index < nChan; ++index) channelPtrs[(size_t)index] = inStreamBuffer.getWritePointer (index);
        for (auto bufferIndex = 0; nChan > 0 && bufferIndex < bufferSize; ++bufferIndex)
        {
            int channelIndex = bufferIndex / nSampl;
            jassert(channelIndex < nChan);
            int sampleIndex = bufferIndex % nSampl;

            float fValue = 0.0f;
            if (!naiveUnPack (&fValue, sizeof (float)))
                return std::make_tuple (0, 0, 0, 0);
            channelPtrs[(size_t)channelIndex][sampleIndex] = fValue;
        }
        return std::make_tuple (nChan, nSampl, nChan0Sz, nChan1Sz);
    };
    auto [nOChan, nSampl, nChan0Sz, nChan1Sz] = naiveUnPackErrorControl();

    for (int channel = 0; channel < nOChan; ++channel)
    {
        if (!gettingData)
        {
            std::cout << std::this_thread::get_id() << " Getting Data." << std::endl;
            gettingData = true;
        }
    }
    return inStreamBuffer;

}
void AudioStreamPluginProcessor::processBlockStreamOutNaive (juce::AudioBuffer<float>& outStreamBuffer, juce::MidiBuffer&)
{
    if (!streamIdOutput)
    {
        RTPStreamConfig cfg(useOpus);
        cfg.mSampRate = static_cast<int32_t>(getSampleRate());
        cfg.mBlockSize = getBlockSize();
        cfg.mPort = static_cast<uint16_t>(outPort);
        cfg.mChannels = getTotalNumOutputChannels();
        cfg.mDirection = 0; //0utput

        streamIdOutput = pRTP->CreateStream(streamSessionID, cfg);
        if (!streamIdOutput)
        {
            std::cout << "CRITICAL: Failed to create stream" << std::endl;
        }
        else std::cout << "Stream ID Output: " << streamIdOutput << std::endl;
    }

    //Tone Generator
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto totalNumberOfSamples   = outStreamBuffer.getNumSamples();

    std::vector<std::byte> bNaive {};


    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto naivePack = [&bNaive](void* rawReadPointer, int bytesize) -> void
        {
            auto p = reinterpret_cast<std::byte*>(rawReadPointer);
            for(auto sz = 0l; sz < bytesize; ++sz)
            {
                bNaive.push_back (*reinterpret_cast<std::byte*> (p + sz));
            }
        };
        if (!channel)
        {
            //Pack the Header only in the first loop.
            auto naiveErrorControl = totalNumberOfSamples + totalNumInputChannels;

            naivePack(&totalNumInputChannels, sizeof(int));
            naivePack(&totalNumberOfSamples, sizeof(int));
            naivePack(&naiveErrorControl, sizeof(int));
        }

        //Pack the Data.
        int szSamples = totalNumberOfSamples * (int)sizeof(float);
        naivePack(const_cast<float*>(outStreamBuffer.getReadPointer (channel)), szSamples);

        //Tx in the last loop.
        if (channel == totalNumOutputChannels - 1) streamOutNaive(bNaive);

    }
}

namespace Utilities {

    void splitChannels(std::vector<std::vector<float>>& channels, const juce::AudioBuffer<float>& buffer);
    void splitChannels(std::vector<std::vector<float>>& channels, const juce::AudioBuffer<float>& buffer)
    {
        auto numSamp = static_cast<size_t>(buffer.getNumSamples());
        auto numChan = static_cast<size_t>(buffer.getNumChannels());
        channels.resize(numChan, std::vector<float>(numSamp, 0.0f));
        auto rdPtrs = buffer.getArrayOfReadPointers();
        for (auto channelIndex = 0lu; channelIndex < numChan; ++channelIndex)
        {
            std::copy(rdPtrs[channelIndex], rdPtrs[channelIndex] + numSamp, channels[channelIndex].begin());
        }
    }

    void joinChannels(juce::AudioBuffer<float>& buffer, const std::vector<std::vector<float>>& channels);
    void joinChannels(juce::AudioBuffer<float>& buffer, const std::vector<std::vector<float>>& channels)
    {
        buffer.clear();
        auto wrPtrs = buffer.getArrayOfWritePointers();
        auto numChan = static_cast<size_t>(buffer.getNumChannels());
        for (auto channelIndex = 0lu; channelIndex < numChan; ++channelIndex)
        {
            std::copy(channels[channelIndex].begin(), channels[channelIndex].end(), wrPtrs[channelIndex]);
        }
        buffer.setNotClear();
    }

    void enumerateBuffer(juce::AudioBuffer<float>& buffer);
    void enumerateBuffer(juce::AudioBuffer<float>& buffer){
        auto numSamp = static_cast<size_t>(buffer.getNumSamples());
        auto numChan = static_cast<size_t>(buffer.getNumChannels());
        auto totalSamp = numChan * numSamp;
        std::vector<float> f (totalSamp, 0.0f);
        std::iota(f.begin(), f.end(), 0);
        for(auto idx_ : f)
        {
            auto idx = static_cast<size_t>(idx_);
            buffer.getWritePointer(static_cast<int>(idx/numSamp))[idx%numSamp] = idx_;
        }
    }
    void printAudioBuffer(const juce::AudioBuffer<float>& buffer);
    void printAudioBuffer(const juce::AudioBuffer<float>& buffer)
    {
        auto totalSamp_ = static_cast<size_t>(buffer.getNumSamples() * buffer.getNumChannels());
        auto numSamp = static_cast<size_t>(buffer.getNumSamples());
        auto rdPtr = buffer.getReadPointer(0);
        for (size_t index = 0; index < totalSamp_; ++index)
        {
            std::cout << "[" << index / numSamp << ", " << index % numSamp << "] = " << rdPtr[index] << " -- " ;

        }
        std::cout << std::endl;

    }
    void printFloatBuffer(const std::vector<float>& buffer);
    void printFloatBuffer(const std::vector<float>& buffer)
    {
        auto totalSamp = buffer.size();
        for (size_t index = 0; index < totalSamp; ++index)
        {
            std::cout << "[" << index << "] = " << buffer[index] << " -- ";
        }
        std::cout << std::endl;
    }

    /*!
     * @brief Interleaves a buffer into a vector of interleaved channels. Where each interleaved channel has 2 channels from buffer. If the number of channels is odd, the last channel is simply the last in buffer.
     * @param intChannels
     * @param buffer
     */
    void interleaveChannels (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer);
    void interleaveChannels (std::vector<std::vector<float>>& intChannels, juce::AudioBuffer<float>& buffer)
    {
        auto& is =  intChannels;
        auto& b =   buffer;

        //layout
        auto numChan = static_cast<size_t>(b.getNumChannels());
        auto numSamp = static_cast<size_t>(b.getNumSamples());
        is.resize(numChan >> 1, std::vector<float>(numSamp << 1, 0.0f));

        //interleave
        auto rdPtrs = b.getArrayOfReadPointers();
        auto channelIndexOffset = 0lu;
        for (auto &i : is)
        {
            std::iota(i.begin(), i.end(), 0.0f);
            std::transform(i.begin(), i.end(), i.begin(),
                [rdPtrs, channelIndexOffset](float&idx_)->float {
                    auto idx = static_cast<size_t>(idx_);
                    auto channelIndex = channelIndexOffset + (idx % 2);
                    auto rdOffset =  idx / 2;
                    return rdPtrs[channelIndex][rdOffset];
                });
            channelIndexOffset += 2;
        }

        //last channel (if odd)
        if (numChan % 2)
        {
            std::vector<float> lstChann = std::vector<float>(numSamp, 0.0f);
            std::copy(rdPtrs[numChan-1], rdPtrs[numChan-1] + numSamp, lstChann.begin());
            is.push_back(lstChann);
        }
    }
    void interleaveChannels (std::vector<float>& intBuffer, juce::AudioBuffer<float>& buffer);
    void interleaveChannels (std::vector<float>& intBuffer, juce::AudioBuffer<float>& buffer)
    {
        intBuffer.clear();

        auto& i =   intBuffer;
        auto& b =   buffer;
        auto numSamp =  static_cast<size_t>(b.getNumSamples());
        auto numChan =  static_cast<size_t>(b.getNumChannels()); numChan = numChan > 2 ? 2 : numChan;
        auto readPointersPerChannel =   buffer.getArrayOfReadPointers();

        if (numChan == 1) {
            std::copy(b.getReadPointer(0), b.getReadPointer(0) + numSamp, std::back_inserter(i));
            return;
        }
        //more than 1 channel
        i.resize((size_t)numSamp * (size_t)numChan, 0.0f);
        std::iota(i.begin(), i.end(), 0);
        std::transform(i.begin(), i.end(),
            i.begin(),
            [readPointersPerChannel, numChan](float&idx_)->float {
                auto idx = static_cast<size_t>(idx_);
                auto channelIndex = idx % numChan;
                auto rdOffset = idx / numChan;
                return readPointersPerChannel[channelIndex][rdOffset];
            });
        b.setNotClear();
    }
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<std::vector<float>>& channels);
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<std::vector<float>>& channels)
    {
        deintBuffer.clear();
        auto&d = deintBuffer;
        auto&cs = channels;
        auto wrPtrs = d.getArrayOfWritePointers();
        auto numChan = static_cast<size_t>(d.getNumChannels());
        auto numSamp = static_cast<size_t>(d.getNumSamples());

        for (auto cIdx = 0lu; cIdx < numChan; ++cIdx)
        {
            auto& c = cs[cIdx / 2];
            auto totalChannels = c.size() / numSamp;
            for (auto sIdx = 0lu; sIdx < numSamp; ++sIdx)
            {
                wrPtrs[cIdx][sIdx] = c[sIdx * totalChannels + cIdx % 2];
            }
        }
        deintBuffer.setNotClear();
    }
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<float>& buffer);
    void deinterleaveChannels (juce::AudioBuffer<float>& deintBuffer, std::vector<float>& buffer)
    {
        deintBuffer.clear();
        auto&d = deintBuffer;
        auto&b = buffer;
        auto numSamp = static_cast<size_t>(d.getNumSamples());
        auto numChan = static_cast<size_t>(d.getNumChannels()); numChan = numChan > 2 ? 2 : numChan;
        auto totalSamp = numChan * numSamp;
        jassert(totalSamp == b.size() && numChan > 0);

        auto writePointersPerChannel = d.getArrayOfWritePointers();
        for (auto sampleIndex = 0lu; sampleIndex < totalSamp; ++sampleIndex)
        {
            writePointersPerChannel[sampleIndex % numChan][sampleIndex / numChan] = b[sampleIndex];
        }
        deintBuffer.setNotClear();

    }

}

void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    updateProcessorHeader(buffer); //THIS UPDATES THE SAMPLE RATE AND THE BLOCK SIZE OF THE PROCESSOR!!!!. ALSO THE TONE GENERATOR when the sample rate changes or the block size changes.

    if (auto* playHead = getPlayHead())
    {
        auto optionalPositionInfo = playHead->getPosition();
        if (optionalPositionInfo)
        {
            juce::Optional<double> info = optionalPositionInfo->getPpqPosition();
            mScrubCurrentPosition.store(*info);
        }
    }

    juce::ScopedNoDenormals noDenormals;

    //Tone Generator
    auto totalNumInputChannels  = getTotalNumInputChannels(); /* not if 2 */ totalNumInputChannels = totalNumInputChannels > 2 ? 2 : totalNumInputChannels;
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    std::vector<std::vector<float>> channels{};
    Utilities::splitChannels(channels, buffer);

    //Lets encode.
    if (useOpus && streamOut)
    {
        for (auto channelIndex = 0lu; channelIndex < channels.size(); ++channelIndex)
        {
            /********** ENCODING STAGE *********************************************/
            auto& channel = channels[channelIndex];
            auto& cfg = *pCodecConfig;
            auto [result, encodedData, encodedDataSize] = pOpusCodec->encodeChannel(channel.data(), channelIndex);
            if (result != OpusImpl::Result::OK)
            {
                std::cout << "Failed to encode channel idx: " << channelIndex << std::endl;
                continue;
            }
            /***********************************************************************/
            /*********** DECODING STAGE ********************************************/

            auto [decResult, decodedData, decodedDataSize] = pOpusCodec->decodeChannel(encodedData.data(), encodedDataSize, channelIndex);
            if (decResult != OpusImpl::Result::OK)
            {
                std::cout << "Failed to decode channel idx: " << channelIndex << std::endl;
                continue;
            }

            if (decodedDataSize != channel.size())
            {
                std::cout << "Decoded data size mismatch! channel idx: " << channelIndex << std::endl;
                continue;
            }

            std::copy(decodedData.begin(), decodedData.end(), channel.begin());
        }
    }

    Utilities::joinChannels(buffer, channels);



    //Encode The Buffer.
    //Interleave the Channels.
    buffer.applyGain(muteTrack ? 0.0f : static_cast<float>(masterGain)); //fMuteTrack if enabled
    //TO AUDIO DEVICE
}

//==============================================================================
bool AudioStreamPluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioStreamPluginProcessor::createEditor()
{
    return new AudioStreamPluginEditor (*this);
}

//==============================================================================
void AudioStreamPluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioStreamPluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioStreamPluginProcessor();
}

std::once_flag noSessionIDflag;
std::once_flag noStreamIDflag;
void AudioStreamPluginProcessor::streamOutNaive (std::vector<std::byte> data)
{
    if (!streamSessionID)
    {
        std::call_once(noSessionIDflag, []() { std::cout << "CRITICAL ERROR: No Session Was Created." << std::endl; });
        return;
    }
    if (!streamIdOutput)
    {
        std::call_once(noStreamIDflag, []() { std::cout << "CRITICAL ERROR: No Stream Was Created." << std::endl; });
        return;
    }
    if (!pRTP->PushFrame(streamIdOutput, data))
    {
        std::cout << "Failed to send frame!" << std::endl;
        return;
    }


}
