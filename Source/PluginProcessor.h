#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP.h"
#include "Params.h"
#include "PresetManager.h"

class DellayriumProcessor : public juce::AudioProcessor
{
public:
    DellayriumProcessor();
    ~DellayriumProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets { apvts };

    // ---- shared with the editor
    std::atomic<int> soloBand { -1 };
    std::atomic<float> inPeak[2] {}, outPeak[2] {};
    std::atomic<float> bandEnergy[Params::numBands] {};
    std::atomic<float> bandGr[Params::numBands] {};
    std::atomic<double> currentBpm { 120.0 };
    double getSampleRateSafe() const { return sr; }

    // output spectrum for the analyzer (mono)
    static constexpr int scopeSize = 16384;
    juce::AbstractFifo scopeFifo { scopeSize };
    std::vector<float> scopeBuf = std::vector<float> (scopeSize, 0.0f);

    // UI state that travels with the project
    juce::String getTheme() const;
    void setTheme (const juce::String&);
    int getSelectedBand() const;
    void setSelectedBand (int);

    float delaySeconds (int band) const;   // current delay time of a band, for the display

private:
    struct BandParams
    {
        std::atomic<float>* on; std::atomic<float>* freq; std::atomic<float>* level; std::atomic<float>* q;
        std::atomic<float>* engine; std::atomic<float>* sync; std::atomic<float>* feel; std::atomic<float>* ms;
        std::atomic<float>* fb; std::atomic<float>* mode; std::atomic<float>* amt; std::atomic<float>* duck;
        std::atomic<float>* drive; std::atomic<float>* dtype; std::atomic<float>* size; std::atomic<float>* diff;
        std::atomic<float>* mix;
    };
    std::array<BandParams, Params::numBands> bp {};
    std::atomic<float>* dryParam = nullptr;
    std::atomic<float>* wetParam = nullptr;

    std::array<dly::BandState, Params::numBands> bands;
    dly::SpectralSplitter splitter;
    std::array<float, Params::numBands> curFreq {}, curQ {}, curLevel {}, curMix {}, curFb {};
    float curDry = 1.0f, curWet = 0.7f;
    float inEnv = 0.0f;
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DellayriumProcessor)
};
