#pragma once
#include "PluginProcessor.h"

class ExpndLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
        float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = startAngle + pos * (endAngle - startAngle);

        g.setColour(juce::Colour(0xff252c38));
        g.fillEllipse(bounds);

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius - 4, radius - 4, 0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colour(0xff3a4354));
        g.strokePath(track, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc(centre.x, centre.y, radius - 4, radius - 4, 0.0f, startAngle, angle, true);
        auto accent = (s.getName().contains("depth") || s.getName().contains("time") || s.getName().contains("out"))
            ? juce::Colour(0xff9fb4e8) : juce::Colour(0xff7ee787);
        g.setColour(accent);
        g.strokePath(value, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(juce::Colour(0xff1f2530));
        g.fillEllipse(centre.x - radius * 0.6f, centre.y - radius * 0.6f, radius * 1.2f, radius * 1.2f);
    }
};

class BandComponent : public juce::Component
{
public:
    BandComponent(juce::AudioProcessorValueTreeState& apvts, ExpanderBand& bandRef,
        const juce::String& prefix, juce::Colour accent);
    void resized() override;
    void paint(juce::Graphics&) override;

private:
    ExpanderBand& band;
    juce::Colour accentColour;
    juce::Slider thrKnob, upKnob, downKnob;
    juce::TextButton muteBtn{ "Mute" }, soloBtn{ "Solo" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thrAtt, upAtt, downAtt;
};

class ExpndAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ExpndAudioProcessorEditor(ExpndAudioProcessor&);
    ~ExpndAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    ExpndAudioProcessor& proc;
    ExpndLookAndFeel lnf;

    juce::Slider xlowKnob, xhighKnob, depthKnob, timeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xlowAtt, xhighAtt, depthAtt, timeAtt;

    BandComponent lowBand, midBand, highBand;
    juce::Label oscBeforeLabel{ {}, "Before" }, oscAfterLabel{ {}, "After" };
};