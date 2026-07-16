#include "PluginEditor.h"

BandComponent::BandComponent(juce::AudioProcessorValueTreeState& apvts, ExpanderBand& bandRef,
    const juce::String& prefix, juce::Colour accent)
    : band(bandRef), accentColour(accent)
{
    for (auto* k : { &thrKnob, &upKnob, &downKnob })
    {
        k->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        addAndMakeVisible(*k);
    }
    thrKnob.setName(prefix + "_thr"); upKnob.setName(prefix + "_up"); downKnob.setName(prefix + "_down");

    thrAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, prefix + "_thr", thrKnob);
    upAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, prefix + "_up", upKnob);
    downAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, prefix + "_down", downKnob);

    addAndMakeVisible(muteBtn);
    addAndMakeVisible(soloBtn);
    muteBtn.setClickingTogglesState(true);
    soloBtn.setClickingTogglesState(true);
    muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe86b6b));
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff7ee787));

    muteBtn.onClick = [this] { band.manualMute.store(muteBtn.getToggleState()); };
    soloBtn.onClick = [this] { band.soloed.store(soloBtn.getToggleState()); };
}

void BandComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff252c38));
    g.fillRoundedRectangle(b, 10.0f);
    g.setColour(accentColour);
    g.fillRect(b.removeFromTop(3.0f));
}

void BandComponent::resized()
{
    auto b = getLocalBounds().reduced(8);
    auto knobRow = b.removeFromTop(80);
    auto w = knobRow.getWidth() / 3;
    thrKnob.setBounds(knobRow.removeFromLeft(w));
    upKnob.setBounds(knobRow.removeFromLeft(w));
    downKnob.setBounds(knobRow);

    auto btnRow = b.removeFromBottom(24);
    muteBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(2));
    soloBtn.setBounds(btnRow.reduced(2));
}

ExpndAudioProcessorEditor::ExpndAudioProcessorEditor(ExpndAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
    lowBand(p.apvts, p.bandLow, "low", juce::Colour(0xff7ee787)),
    midBand(p.apvts, p.bandMid, "mid", juce::Colour(0xff9fb4e8)),
    highBand(p.apvts, p.bandHigh, "high", juce::Colour(0xff7ee787))
{
    setLookAndFeel(&lnf);
    setSize(680, 560);

    for (auto* k : { &xlowKnob, &xhighKnob, &depthKnob, &timeKnob })
    {
        k->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        addAndMakeVisible(*k);
    }
    xlowKnob.setName("xlow"); xhighKnob.setName("xhigh"); depthKnob.setName("depth"); timeKnob.setName("time");

    xlowAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "xlow", xlowKnob);
    xhighAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "xhigh", xhighKnob);
    depthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "depth", depthKnob);
    timeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "time", timeKnob);

    addAndMakeVisible(lowBand);
    addAndMakeVisible(midBand);
    addAndMakeVisible(highBand);

    addAndMakeVisible(proc.oscBefore);
    addAndMakeVisible(proc.oscAfter);
    oscBeforeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9fb4e8));
    oscAfterLabel.setColour(juce::Label::textColourId, juce::Colour(0xff7ee787));
    oscBeforeLabel.setFont(juce::FontOptions(12.0f));
    oscAfterLabel.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(oscBeforeLabel);
    addAndMakeVisible(oscAfterLabel);

    startTimerHz(30);
}

ExpndAudioProcessorEditor::~ExpndAudioProcessorEditor() { setLookAndFeel(nullptr); }

void ExpndAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1b2028));
    g.setColour(juce::Colour(0xff7ee787));
    g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
    g.drawText("EXPND", getLocalBounds().removeFromTop(30).reduced(12, 0), juce::Justification::centredLeft);
}

void ExpndAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced(16);
    b.removeFromTop(24);

    // Oscilloscope row at the top - before (left) and after (right), side by side
    auto scopeRow = b.removeFromTop(120);
    auto scopeW = scopeRow.getWidth() / 2;
    auto beforeArea = scopeRow.removeFromLeft(scopeW).reduced(4);
    auto afterArea = scopeRow.reduced(4);
    oscBeforeLabel.setBounds(beforeArea.removeFromTop(16));
    oscAfterLabel.setBounds(afterArea.removeFromTop(16));
    proc.oscBefore.setBounds(beforeArea);
    proc.oscAfter.setBounds(afterArea);

    b.removeFromTop(12);

    auto topRow = b.removeFromTop(100);
    auto w = topRow.getWidth() / 4;
    xlowKnob.setBounds(topRow.removeFromLeft(w));
    xhighKnob.setBounds(topRow.removeFromLeft(w));
    depthKnob.setBounds(topRow.removeFromLeft(w));
    timeKnob.setBounds(topRow);

    b.removeFromTop(12);
    auto bw = b.getWidth() / 3;
    lowBand.setBounds(b.removeFromLeft(bw).reduced(4));
    midBand.setBounds(b.removeFromLeft(bw).reduced(4));
    highBand.setBounds(b.reduced(4));
}

void ExpndAudioProcessorEditor::timerCallback() { repaint(); }