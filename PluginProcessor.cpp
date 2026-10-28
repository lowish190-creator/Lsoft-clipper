#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SoftClipDSP.h"

namespace IDs
{
    static const juce::String threshold  { "threshold" };
    static const juce::String postGain   { "postgain" };
    static const juce::String shape      { "shape" };
    static const juce::String oversample { "oversampling" };
    static const juce::String bypass     { "bypass" };
}

juce::AudioProcessorValueTreeState::ParameterLayout SoftClipperAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { IDs::threshold, 1 }, "Threshold",
        NormalisableRange<float> (-30.0f, 0.0f, 0.01f), -6.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { IDs::postGain, 1 }, "Post gain",
        NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { IDs::shape, 1 }, "Shape",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { IDs::oversample, 1 }, "Oversampling",
        StringArray { "Off", "2x", "4x", "8x" }, 2));

    params.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { IDs::bypass, 1 }, "Bypass", false));

    return { params.begin(), params.end() };
}

SoftClipperAudioProcessor::SoftClipperAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    thresholdParam    = apvts.getRawParameterValue (IDs::threshold);
    postGainParam     = apvts.getRawParameterValue (IDs::postGain);
    shapeParam        = apvts.getRawParameterValue (IDs::shape);
    oversamplingParam = apvts.getRawParameterValue (IDs::oversample);
    bypassParam       = apvts.getRawParameterValue (IDs::bypass);
}

bool SoftClipperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void SoftClipperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const auto numChannels = (size_t) juce::jmax (1, getTotalNumOutputChannels());

    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            numChannels, i + 1,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversamplers[i]->initProcessing ((size_t) samplesPerBlock);
    }

    thresholdSmoothed.reset (sampleRate, 0.02);
    postGainSmoothed.reset (sampleRate, 0.02);
    thresholdSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (thresholdParam->load()));
    postGainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (postGainParam->load()));

    currentOsChoice = -1; // fuerza la actualizacion de la latencia
}

void SoftClipperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // Oversampling 0 = off, 1..3 = 2x/4x/8x
    const int osChoice = (int) oversamplingParam->load();
    if (osChoice != currentOsChoice)
    {
        currentOsChoice = osChoice;
        setLatencySamples (osChoice == 0
                               ? 0
                               : (int) std::round (oversamplers[(size_t) osChoice - 1]->getLatencyInSamples()));
        if (osChoice > 0)
            oversamplers[(size_t) osChoice - 1]->reset();
    }

    if (bypassParam->load() > 0.5f)
        return;

    thresholdSmoothed.setTargetValue (juce::Decibels::decibelsToGain (thresholdParam->load()));
    postGainSmoothed.setTargetValue  (juce::Decibels::decibelsToGain (postGainParam->load()));
    const float shape = shapeParam->load();

    juce::dsp::AudioBlock<float> block (buffer);
    auto inBlock = block.getSubsetChannelBlock (0, (size_t) numChannels);

    // Ganancias por muestra (a la tasa original) para suavizar los parametros
    std::vector<float> thr ((size_t) numSamples), pg ((size_t) numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        thr[(size_t) i] = thresholdSmoothed.getNextValue();
        pg[(size_t) i]  = postGainSmoothed.getNextValue();
    }

    auto clipBlock = [&] (juce::dsp::AudioBlock<float>& b, int factor)
    {
        const auto n = b.getNumSamples();
        for (size_t ch = 0; ch < b.getNumChannels(); ++ch)
        {
            auto* d = b.getChannelPointer (ch);
            for (size_t i = 0; i < n; ++i)
            {
                const auto idx = juce::jmin ((size_t) numSamples - 1, i / (size_t) factor);
                const float t = thr[idx];
                d[i] = sc::clip (d[i] / t, shape) * t * pg[idx];
            }
        }
    };

    if (osChoice == 0)
    {
        clipBlock (inBlock, 1);
    }
    else
    {
        auto& os = *oversamplers[(size_t) osChoice - 1];
        auto upBlock = os.processSamplesUp (inBlock);
        clipBlock (upBlock, 1 << osChoice);
        os.processSamplesDown (inBlock);
    }

    float peak = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
    outputPeak.store (peak);
}

juce::AudioProcessorEditor* SoftClipperAudioProcessor::createEditor()
{
    return new SoftClipperAudioProcessorEditor (*this);
}

void SoftClipperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SoftClipperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoftClipperAudioProcessor();
}
