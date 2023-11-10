#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Utilities.h"

#include "opusImpl.h"

#include <chrono>
#include <random>
#include <thread>
#include <cstddef>
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

    streamSessionID = pRTP->CreateSession("127.0.0.1");
    if (!streamSessionID)
    {
        std::cout << "Failed to create the RTP Session" << std::endl;
    }
    else
    {

        std::vector<int32_t> ports{8888, 8889};

        bool searchingForListeningPort = true;
        for (auto& port : ports)
        {
            //wait
            inPort = port;
            outPort = 8888 + (port + 1) % 2;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 5);
            std::this_thread::sleep_for(std::chrono::seconds(dis(gen)));

            //check if port is in use or we are looking
            if (!searchingForListeningPort) break;
            if (RTPWrapUtils::udpPortIsInUse(port)) continue;

            //attempt to create an input stream
            streamIdInput = pRTP->CreateStream(streamSessionID, port, 1);
            if (!streamIdInput)
            {
                std::cout << "Failed to create an inbound RTP Stream" << std::endl;
            }
            else
            {
                std::cout << "Inbound Stream ID: " << streamIdInput << std::endl;

                searchingForListeningPort = false;

                break;
            }
        }


        //Let's create a streamSessionID and a stream
        streamIdOutput = pRTP->CreateStream(streamSessionID, outPort, 0);
        if (!streamIdOutput)
        {
            std::cout << "Failed to create an outbound RTP Stream" << std::endl;
        }
        else
        {
            std::cout << "Outbound Stream ID: " << streamIdOutput << std::endl;
        }
    }



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

    pCodecConfig = std::make_unique<OpusImpl::CODECConfig>();

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
        streamIdOutput = pRTP->CreateStream(streamSessionID, static_cast<uint16_t>(outPort), 0);
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


void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    beforeProcessBlock(buffer, midiMessages); //THIS UPDATES THE SAMPLE RATE AND THE BLOCK SIZE OF THE PROCESSOR!!!!. ALSO THE TONE GENERATOR when the sample rate changes or the block size changes.

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
    Utilities::Data::splitChannels(channels, buffer);

    //Lets encode.
    if (useOpus && streamOut)
    {
        for (auto channelIndex = 0lu; channelIndex < channels.size(); ++channelIndex)
        {
            /********** ENCODING STAGE *********************************************/
            auto& channel = channels[channelIndex];
            auto [result, encodedData, encodedDataSize] = pOpusCodec->encodeChannel(channel.data(), channelIndex);
            if (result != OpusImpl::Result::OK)
            {
                std::cout << "Failed to encode channel idx: " << channelIndex << std::endl;
                continue;
            }
            //Grab uvgRTP
            pRTP->PushFrame(streamIdOutput, encodedData);
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

    Utilities::Data::joinChannels(buffer, channels);



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
