#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>

class SoftClipperAudioProcessor : public juce::AudioProcessor
{
public:
    SoftClipperAudioProcessor();
    ~SoftClipperAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Nivel de pico (post-clip) para el medidor del editor
    std::atomic<float> outputPeak { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Oversampling: indice 0 = 2x, 1 = 4x, 2 = 8x (se usa 1x sin oversampler)
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    int currentOsChoice = -1;

    juce::SmoothedValue<float> thresholdSmoothed, postGainSmoothed;
    double currentSampleRate = 44100.0;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* postGainParam = nullptr;
    std::atomic<float>* shapeParam = nullptr;
    std::atomic<float>* oversamplingParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftClipperAudioProcessor)
};
