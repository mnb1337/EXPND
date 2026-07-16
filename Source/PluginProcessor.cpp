#include "PluginProcessor.h"
#include "PluginEditor.h"

ExpndAudioProcessor::ExpndAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createLayout())
{
    lowLP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    midHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    midLP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    highHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

    oscBefore.setBufferSize(512);
    oscBefore.setSamplesPerBlock(64);
    oscBefore.setColours(juce::Colour(0xff1f2530), juce::Colour(0xff9fb4e8));

    oscAfter.setBufferSize(512);
    oscAfter.setSamplesPerBlock(64);
    oscAfter.setColours(juce::Colour(0xff1f2530), juce::Colour(0xff7ee787));
}

juce::AudioProcessorValueTreeState::ParameterLayout ExpndAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>("xlow", "Low/Mid Xover", 60.0f, 1000.0f, 220.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("xhigh", "Mid/High Xover", 1000.0f, 12000.0f, 4200.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("depth", "Depth", 0.0f, 1.0f, 0.7f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("time", "Time", 1.0f, 250.0f, 40.0f));
    for (auto* band : { "low", "mid", "high" })
    {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::String(band) + "_thr", juce::String(band) + " Threshold", -60.0f, 0.0f, -24.0f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::String(band) + "_up", juce::String(band) + " Up", 0.0f, 1.0f, 0.5f));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::String(band) + "_down", juce::String(band) + " Down", 0.0f, 1.0f, 0.5f));
    }
    return { p.begin(), p.end() };
}

void ExpndAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32)samplesPerBlock, 2 };
    lowLP.prepare(spec); midHP.prepare(spec); midLP.prepare(spec); highHP.prepare(spec);
}

void ExpndAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // "Before" scope sees the raw dry input, untouched
    oscBefore.pushBuffer(buffer);

    lowLP.setCutoffFrequency(*apvts.getRawParameterValue("xlow"));
    midHP.setCutoffFrequency(*apvts.getRawParameterValue("xlow"));
    midLP.setCutoffFrequency(*apvts.getRawParameterValue("xhigh"));
    highHP.setCutoffFrequency(*apvts.getRawParameterValue("xhigh"));

    depthAll = *apvts.getRawParameterValue("depth");
    timeMs = *apvts.getRawParameterValue("time");
    bandLow.thresholdDb = *apvts.getRawParameterValue("low_thr");
    bandLow.upAmt = *apvts.getRawParameterValue("low_up");
    bandLow.downAmt = *apvts.getRawParameterValue("low_down");
    bandMid.thresholdDb = *apvts.getRawParameterValue("mid_thr");
    bandMid.upAmt = *apvts.getRawParameterValue("mid_up");
    bandMid.downAmt = *apvts.getRawParameterValue("mid_down");
    bandHigh.thresholdDb = *apvts.getRawParameterValue("high_thr");
    bandHigh.upAmt = *apvts.getRawParameterValue("high_up");
    bandHigh.downAmt = *apvts.getRawParameterValue("high_down");

    for (auto* b : { &bandLow, &bandMid, &bandHigh })
    {
        b->depth = depthAll;
        b->setTimeMs(timeMs, currentSampleRate);
    }

    // Solo overrides: if anything is soloed, mute everything not soloed. Otherwise respect manual mute.
    bool anySolo = bandLow.soloed.load() || bandMid.soloed.load() || bandHigh.soloed.load();
    bandLow.effectiveMute = anySolo ? !bandLow.soloed.load() : bandLow.manualMute.load();
    bandMid.effectiveMute = anySolo ? !bandMid.soloed.load() : bandMid.manualMute.load();
    bandHigh.effectiveMute = anySolo ? !bandHigh.soloed.load() : bandHigh.manualMute.load();

    auto numCh = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    juce::AudioBuffer<float> low(numCh, numSamples), mid(numCh, numSamples), high(numCh, numSamples);
    low.makeCopyOf(buffer); mid.makeCopyOf(buffer); high.makeCopyOf(buffer);

    juce::dsp::AudioBlock<float> lowBlock(low), midBlock(mid), highBlock(high);
    lowLP.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
    midHP.process(juce::dsp::ProcessContextReplacing<float>(midBlock));
    midLP.process(juce::dsp::ProcessContextReplacing<float>(midBlock));
    highHP.process(juce::dsp::ProcessContextReplacing<float>(highBlock));

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* dry = buffer.getWritePointer(ch);
        auto* l = low.getWritePointer(ch);
        auto* m = mid.getWritePointer(ch);
        auto* h = high.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float lp = bandLow.processSample(l[i]);
            float mp = bandMid.processSample(m[i]);
            float hp = bandHigh.processSample(h[i]);
            float wet = lp + mp + hp;
            dry[i] = dry[i] * (1.0f - depthAll * 0.9f) + wet;
        }
    }

    // "After" scope sees the final processed output
    oscAfter.pushBuffer(buffer);
}

juce::AudioProcessorEditor* ExpndAudioProcessor::createEditor()
{
    return new ExpndAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ExpndAudioProcessor();
}