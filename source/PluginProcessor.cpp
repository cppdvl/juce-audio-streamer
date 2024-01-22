#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "Utilities/Utilities.h"

#include "opusImpl.h"

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
}

void AudioStreamPluginProcessor::prepareToPlay (double , int )
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //Set 48k (More Suitable for Opus according to documentation)
    // I removed forcing the sample rate to 48k, because it was causing issues with the graphical interface and I had no certainty about the real size of the buffer and its duration.

    mAudioMixerBlocks   = std::vector<Mixer::AudioMixerBlock>(static_cast<size_t>(getTotalNumInputChannels()));

}

bool& AudioStreamPluginProcessor::getMonoFlagReference()
{
    return mMonoSplit;
}

const int& AudioStreamPluginProcessor::getSampleRateReference() const
{
    return mSampleRate;
}

const int& AudioStreamPluginProcessor::getBlockSizeReference() const
{
    return mBlockSize;
}

std::tuple<uint32_t, int64_t> AudioStreamPluginProcessor::getUpdatedTimePosition()
{
    auto [nTimeMS, nSamplePosition] = Utilities::Time::getPosInMSAndSamples(getPlayHead());
    jassert(nSamplePosition >= 0);
    return std::make_tuple(nTimeMS, nSamplePosition);
}



void AudioStreamPluginProcessor::beforeProcessBlock(juce::AudioBuffer<float>& buffer)
{
    mBlockSize = buffer.getNumSamples();
    mSampleRate = static_cast<int>(getSampleRate());
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    mChannels = totalNumOutputChannels - totalNumInputChannels;
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

void AudioStreamPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    beforeProcessBlock(buffer);
    auto [nTimeMS, timeStamp] = getUpdatedTimePosition();


    std::vector<Mixer::Block> splittedBlocks {};
    Utilities::Data::splitChannels(splittedBlocks, buffer, mMonoSplit);

    //AUDIOMIXER BLOCK
    size_t channelIndex = 0;
    jassert(mAudioMixerBlocks.size() == splittedBlocks.size());
    Mixer::AudioMixerBlock::mix(mAudioMixerBlocks, timeStamp, splittedBlocks);



    //PLAYBACK (Only two channels)
    if (listenedRenderedAudioChannel) Utilities::Data::joinChannels(buffer, {mAudioMixerBlocks[0].getBlock(timeStamp), mAudioMixerBlocks[1].getBlock(timeStamp)});
    //TODO: Create a button to listen to rendered AudioChannel.

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


AudioStreamPluginProcessor::~AudioStreamPluginProcessor()
{
    /*if (streamSessionID)
    {
        pRTP->DestroySession(streamSessionID);
    }*/
}

void AudioStreamPluginProcessor::releaseResources()
{

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
