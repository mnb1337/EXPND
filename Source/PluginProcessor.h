#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

struct ExpanderBand
{
    float envelope = 0.0f;
    float thresholdDb = -24.0f, upAmt = 0.5f, downAmt = 0.5f;
    float attackCoef = 0.99f, releaseCoef = 0.999f;
    float depth = 0.7f;
    std::atomic<float> meterDb{ 0.0f };

    std::atomic<bool> manualMute{ false };
    std::atomic<bool> soloed{ false };
    bool effectiveMute = false; // solo ya mute wala boton

    float processSample(float x)
    {
        float rect = std::abs(x);
        float coef = rect > envelope ? attackCoef : releaseCoef;
        envelope = coef * envelope + (1.0f - coef) * rect;
        float envDb = 20.0f * std::log10(envelope + 1.0e-8f);
        float gainDb;
        if (envDb < thresholdDb) gainDb = (envDb - thresholdDb) * downAmt * 1.5f;
        else                     gainDb = (envDb - thresholdDb) * upAmt * 0.6f;
        gainDb *= depth;
        meterDb.store(gainDb);
        return effectiveMute ? 0.0f : x * juce::Decibels::decibelsToGain(gainDb);
    }

    void setTimeMs(float ms, double sampleRate)
    {
        float atkSec = juce::jmax(0.001f, ms / 4000.0f);
        float relSec = juce::jmax(0.01f, ms / 1000.0f);
        attackCoef = std::exp(-1.0f / (atkSec * (float)sampleRate));
        releaseCoef = std::exp(-1.0f / (relSec * (float)sampleRate));
    }
};

class ExpndAudioProcessor : public juce::AudioProcessor
{
public:
    ExpndAudioProcessor();
    ~ExpndAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EXPND"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;
    ExpanderBand bandLow, bandMid, bandHigh;
    float depthAll = 0.7f, timeMs = 40.0f;

    // osci hai yaha se vro
    juce::AudioVisualiserComponent oscBefore{ 1 };
    juce::AudioVisualiserComponent oscAfter{ 1 };

private:
    juce::dsp::LinkwitzRileyFilter<float> lowLP, midHP, midLP, highHP;
    double currentSampleRate = 44100.0;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
};