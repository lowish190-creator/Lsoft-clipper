#pragma once
#include "PluginProcessor.h"

// Muestra la curva de transferencia (entrada -> salida) en tiempo real
class TransferCurve : public juce::Component
{
public:
    explicit TransferCurve (SoftClipperAudioProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override;

private:
    SoftClipperAudioProcessor& proc;
};

class SoftClipperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit SoftClipperAudioProcessorEditor (SoftClipperAudioProcessor&);
    ~SoftClipperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    SoftClipperAudioProcessor& processor;
    TransferCurve curve;

    juce::Slider thresholdSlider, postGainSlider, shapeSlider;
    juce::Label thresholdLabel, postGainLabel, shapeLabel;
    juce::ComboBox oversamplingBox;
    juce::Label oversamplingLabel;
    juce::ToggleButton bypassButton { "Bypass" };

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment    = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> thresholdAtt, postGainAtt, shapeAtt;
    std::unique_ptr<ComboAttachment> oversamplingAtt;
    std::unique_ptr<ButtonAttachment> bypassAtt;

    float meterLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftClipperAudioProcessorEditor)
};
