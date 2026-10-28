#include "PluginEditor.h"
#include "SoftClipDSP.h"

void TransferCurve::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff1b1e24));
    g.fillRoundedRectangle (area, 6.0f);

    // rejilla
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (area.getX(), area.getCentreY(), area.getRight(), area.getCentreY());
    g.drawLine (area.getCentreX(), area.getY(), area.getCentreX(), area.getBottom());

    const float shape = *proc.apvts.getRawParameterValue ("shape");
    const float thrDb = *proc.apvts.getRawParameterValue ("threshold");
    const float pgDb  = *proc.apvts.getRawParameterValue ("postgain");
    const float thr = juce::Decibels::decibelsToGain (thrDb);
    const float pg  = juce::Decibels::decibelsToGain (pgDb);

    // eje: -1..+1 de entrada; salida limitada al mismo rango visual
    juce::Path p;
    const int n = 200;
    for (int i = 0; i <= n; ++i)
    {
        const float x = juce::jmap ((float) i, 0.0f, (float) n, -1.0f, 1.0f);
        const float y = juce::jlimit (-1.0f, 1.0f, sc::clip (x / thr, shape) * thr * pg);
        const float px = juce::jmap (x, -1.0f, 1.0f, area.getX(), area.getRight());
        const float py = juce::jmap (y, -1.0f, 1.0f, area.getBottom(), area.getY());
        if (i == 0) p.startNewSubPath (px, py);
        else        p.lineTo (px, py);
    }
    g.setColour (juce::Colour (0xffff9f1c));
    g.strokePath (p, juce::PathStrokeType (2.0f));
}

SoftClipperAudioProcessorEditor::SoftClipperAudioProcessorEditor (SoftClipperAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), curve (p)
{
    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
        s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff9f1c));
        addAndMakeVisible (s);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupKnob (thresholdSlider, thresholdLabel, "Threshold");
    setupKnob (postGainSlider, postGainLabel, "Post gain");
    setupKnob (shapeSlider, shapeLabel, "Shape");
    thresholdSlider.setTextValueSuffix (" dB");
    postGainSlider.setTextValueSuffix (" dB");

    oversamplingBox.addItemList ({ "Off", "2x", "4x", "8x" }, 1);
    addAndMakeVisible (oversamplingBox);
    oversamplingLabel.setText ("Oversampling", juce::dontSendNotification);
    oversamplingLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (oversamplingLabel);
    addAndMakeVisible (bypassButton);
    addAndMakeVisible (curve);

    thresholdAtt    = std::make_unique<SliderAttachment> (p.apvts, "threshold", thresholdSlider);
    postGainAtt     = std::make_unique<SliderAttachment> (p.apvts, "postgain", postGainSlider);
    shapeAtt        = std::make_unique<SliderAttachment> (p.apvts, "shape", shapeSlider);
    oversamplingAtt = std::make_unique<ComboAttachment>  (p.apvts, "oversampling", oversamplingBox);
    bypassAtt       = std::make_unique<ButtonAttachment> (p.apvts, "bypass", bypassButton);

    setSize (560, 360);
    startTimerHz (30);
}

SoftClipperAudioProcessorEditor::~SoftClipperAudioProcessorEditor()
{
    stopTimer();
}

void SoftClipperAudioProcessorEditor::timerCallback()
{
    const float peak = processor.outputPeak.exchange (0.0f);
    meterLevel = juce::jmax (peak, meterLevel * 0.85f);
    curve.repaint();
    repaint (getLocalBounds().removeFromRight (20));
}

void SoftClipperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff23272e));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("SOFT CLIPPER", 16, 8, 250, 28, juce::Justification::centredLeft);

    // medidor de pico de salida
    auto m = getLocalBounds().removeFromRight (20).reduced (4, 40).withTrimmedBottom (10).toFloat();
    g.setColour (juce::Colour (0xff1b1e24));
    g.fillRoundedRectangle (m, 3.0f);
    const float db = juce::Decibels::gainToDecibels (meterLevel, -60.0f);
    const float h = juce::jmap (juce::jlimit (-60.0f, 3.0f, db), -60.0f, 3.0f, 0.0f, m.getHeight());
    g.setColour (db > 0.0f ? juce::Colours::red : juce::Colour (0xff4cc38a));
    g.fillRoundedRectangle (m.withTop (m.getBottom() - h), 3.0f);
}

void SoftClipperAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    r.removeFromRight (20);
    auto top = r.removeFromTop (30);
    bypassButton.setBounds (top.removeFromRight (80));
    oversamplingBox.setBounds (top.removeFromRight (80));
    oversamplingLabel.setBounds (top.removeFromRight (110));

    r.removeFromTop (6);
    curve.setBounds (r.removeFromLeft (r.getHeight()).reduced (2));
    r.removeFromLeft (10);

    auto knobArea = r;
    const int colW = knobArea.getWidth() / 3;
    auto place = [&] (juce::Slider& s, juce::Label& l, juce::Rectangle<int> a)
    {
        l.setBounds (a.removeFromTop (20));
        s.setBounds (a);
    };
    // 3 potenciometros: se acomodan en columna doble si falta ancho
    auto k1 = knobArea.removeFromTop (knobArea.getHeight() / 2);
    place (thresholdSlider, thresholdLabel, k1.removeFromLeft (k1.getWidth() / 2));
    place (postGainSlider,  postGainLabel,  k1);
    place (shapeSlider, shapeLabel, knobArea.withSizeKeepingCentre (juce::jmax (colW, 100), knobArea.getHeight()));
}
