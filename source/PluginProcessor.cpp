#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    //Create a stream session
    pRTP->Initialize();

    //Create a stream session : this should change, the intention of this code is to purely test.
    streamSessionID = pRTP->CreateSession("127.0.0.1");
    if (!streamSessionID) {
        std::cout << "Failed to create session" << std::endl;
    }
    else
    {
        //try to create a listening stream (an input only stream).
        std::vector<int> ports {8888, 8889};

        bool searchPort = true;
        for (auto port : ports)
        {
            if (!searchPort) break;

            bool portIsNotFreeKeepLooking = !RTPWrapUtils::udpPortIsFree(port);
            if (portIsNotFreeKeepLooking || searchPort == false) continue;
            searchPort = false;
            inPort = port;
            outPort = inPort == 8888 ? 8889 : 8888;

            streamIdInput = pRTP->CreateStream(streamSessionID, inPort, 1);
            if (streamIdInput)
            {
                auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamIdInput);
                if (!pStream)
                {
                    std::cout << "Failed to get stream: " << streamIdInput << std::endl;
                }
                else if (pStream->install_receive_hook(this, +[](void*p, uvgrtp::frame::rtp_frame* pFrame) -> void
                             {
                                 auto pThis = reinterpret_cast<AudioStreamPluginProcessor*>(p);
                                 std::lock_guard<std::mutex> lock (pThis->mMutexInput);
                                 for (auto sz = 0ul; sz < pFrame->payload_len; ++sz)
                                 {
                                     pThis->mInputBuffer.push_back(*reinterpret_cast<std::byte*>(pFrame->payload + sz));
                                 }
                                 uvgrtp::frame::dealloc_frame(pFrame);
                             }) != RTP_OK)
                {
                    std::cout << "Failed to install RTP receive hook!" << std::endl;
                }
                std::cout << "Stream ID Input: " << streamIdInput << std::endl;
            }
        }
        if (searchPort)
        {
            std::cout << "Failed to find a free port." << std::endl;
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
void AudioStreamPluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    toneGenerator.prepareToPlay(samplesPerBlock, sampleRate);
    channelInfo.buffer = nullptr;
    channelInfo.startSample = 0;


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

    auto naiveUnPackErrorControl = [this, &inStreamBuffer]() -> std::tuple<int, int, int> {

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
        int nECtrl  = 0;
        if (!naiveUnPack (&nChan, sizeof (int)))
            return std::make_tuple (0, 0, 0);
        if (!naiveUnPack (&nSampl, sizeof (int)))
            return std::make_tuple (0, 0, 0);
        if (!naiveUnPack (&nECtrl, sizeof (int)))
            return std::make_tuple (0, 0, 0);

        int nECtrl_ = nChan + nSampl;
        int minBufferSizeExpected = nSampl * nChan;
        int totalBufferSize = (int)mInputBuffer.size();

        if (nECtrl_ != nECtrl || minBufferSizeExpected > totalBufferSize)
            return std::make_tuple (0, 0, 0);

        int bufferSize = nSampl * nChan;

        std::vector<float*> channelPtrs((size_t)nChan, nullptr);
        for (auto index = 0; index < nChan; ++index) channelPtrs[(size_t)index] = inStreamBuffer.getWritePointer (index);
        for (auto bufferIndex = 0; nChan > 0 && bufferIndex < bufferSize; ++bufferIndex)
        {
            int channelIndex = bufferIndex / nSampl;
            jassert(channelIndex < nChan);
            int sampleIndex = bufferIndex % nSampl;

            float fValue = 0.0f;
            if (!naiveUnPack (&fValue, sizeof (float)))
                return std::make_tuple (0, 0, 0);
            channelPtrs[(size_t)channelIndex][sampleIndex] = fValue;
        }
        return std::make_tuple (nChan, nSampl, nECtrl);
    };
    auto [nOChan, nSampl, nECtrl] = naiveUnPackErrorControl();

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
        RTPStreamConfig cfg;
        cfg.mSampRate = static_cast<int32_t>(getSampleRate());
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
void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    //OUTSTREAM CHAIN: buffer->fToneGenerator -> fCopy ----> outBuffer -> fGainOut -> streamOut
    //INSTREAM CHAIN:  buffer->fToneGenerator --+
    //                                          |
    //                                          v
    //                 streamIn -> fGainIn -> fAdd --> fMasterGain -> AudioDevice

    // buffer is the juce AudioBuffer in the parameters.
    // streamOut is another AudioBuffer that is sent to the network.
    // streamIn is another AudioBuffer that is received from the network.

    channelInfo.buffer = &buffer;
    channelInfo.numSamples = buffer.getNumSamples();
    toneGenerator.getNextAudioBlock(channelInfo); //buffer->fToneGenerator

    //Let's stream Out.
    juce::AudioBuffer<float> outStreamBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    outStreamBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples()); //fCopy
    outStreamBuffer.applyGain(static_cast<float> (streamOutGain)); //fGainOut
    processBlockStreamOutNaive(outStreamBuffer, midiMessages); //streamOut

    //Let's capture stream In.
    juce::AudioBuffer<float> inStreamBuffer = processBlockStreamInNaive(buffer, midiMessages); //streamIn
    inStreamBuffer.applyGain (static_cast<float>(streamInGain)); //fGainIn

    jassert(inStreamBuffer.getNumSamples() == buffer.getNumSamples());
    jassert(inStreamBuffer.getNumChannels() == buffer.getNumChannels());

    buffer.addFrom(0, 0, inStreamBuffer, 0, 0, buffer.getNumSamples());//fAdd
    buffer.applyGain(static_cast<float> (masterGain)); //fMasterGain
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
    /*auto pmedia = reinterpret_cast<uint8_t *>(data.data());
    auto pStream = UVGRTPWrap::GetSP(pRTP)->GetStream(streamIdOutput);
    if (pStream->push_frame(pmedia, data.size(), RTP_NO_FLAGS) != RTP_OK)
    {
        std::cerr << "Failed to send frame!" << std::endl;
        return;
    }*/

}
