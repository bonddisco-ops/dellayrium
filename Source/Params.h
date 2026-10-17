#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs and layout of Dellayrium: 8 delay bands drawn as EQ bells, plus global Dry / Wet.
namespace Params
{
    inline constexpr int numBands = 8;

    inline constexpr const char* dry = "dry";
    inline constexpr const char* wet = "wet";

    // per band: "b<index>_<name>"
    inline juce::String id (int band, const char* name) { return "b" + juce::String (band) + "_" + name; }

    inline constexpr const char* on     = "on";
    inline constexpr const char* freq   = "freq";
    inline constexpr const char* level  = "level";   // bell height: how much of the band goes into the delay
    inline constexpr const char* q      = "q";
    inline constexpr const char* engine = "engine";  // Analog / FFT
    inline constexpr const char* sync   = "sync";    // note value
    inline constexpr const char* feel   = "feel";    // straight / dotted / triplet / ms
    inline constexpr const char* ms     = "ms";
    inline constexpr const char* fb     = "fb";
    inline constexpr const char* mode   = "mode";    // Mono / Ping-Pong / Wide / Spectral Wide
    inline constexpr const char* amt    = "amt";     // the knob under the mode: Darken / Swing / Width / Scatter
    inline constexpr const char* duck   = "duck";
    inline constexpr const char* datk   = "datk";    // duck attack, ms
    inline constexpr const char* drel   = "drel";    // duck release, ms
    inline constexpr const char* drive  = "drive";
    inline constexpr const char* dtype  = "dtype";
    inline constexpr const char* size   = "size";
    inline constexpr const char* diff   = "diff";
    inline constexpr const char* mix    = "mix";     // band output level

    inline const juce::StringArray& syncNames()   { static const juce::StringArray s { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1" }; return s; }
    inline const juce::StringArray& feelNames()   { static const juce::StringArray s { "Straight", "Dotted", "Triplet", "ms" }; return s; }
    inline const juce::StringArray& modeNames()   { static const juce::StringArray s { "Mono", "Ping-Pong", "Wide", "Spectral Wide" }; return s; }
    inline const juce::StringArray& modeKnob()    { static const juce::StringArray s { "Darken", "Swing", "Width", "Scatter" }; return s; }
    inline const juce::StringArray& driveNames()  { static const juce::StringArray s { "Tape", "Tube", "Gentle", "Transform" }; return s; }
    inline const juce::StringArray& engineNames() { static const juce::StringArray s { "Analog", "FFT" }; return s; }

    inline double syncBeats (int idx)
    {
        static const double b[] = { 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 2.0, 4.0, 8.0 };
        return b[juce::jlimit (0, 7, idx)];
    }

    struct BandDefaults { float freq; bool on; int sync; int feel; int mode; float q; float level; };
    inline const BandDefaults& defaults (int band)
    {
        static const BandDefaults d[numBands] = {
            { 60.0f,    false, 4, 0, 0, 2.0f, 0.6f },
            { 170.0f,   true,  3, 1, 0, 2.0f, 0.65f },
            { 420.0f,   false, 3, 0, 2, 2.4f, 0.6f },
            { 1000.0f,  true,  2, 0, 1, 2.2f, 0.85f },
            { 2200.0f,  false, 2, 0, 2, 2.6f, 0.6f },
            { 4600.0f,  true,  1, 0, 3, 2.8f, 0.7f },
            { 8500.0f,  false, 1, 0, 3, 2.8f, 0.6f },
            { 14000.0f, false, 0, 0, 1, 2.4f, 0.5f },
        };
        return d[juce::jlimit (0, numBands - 1, band)];
    }

    // bell height 0..1 -> linear gain (0 = silent, 0.5 = -12 dB, 1 = 0 dB)
    inline float levelGain (float level) { return level * level; }
    inline juce::String levelText (float level)
    {
        const float g = levelGain (level);
        return g < 0.0001f ? juce::String ("-inf dB") : juce::String (juce::Decibels::gainToDecibels (g), 1) + " dB";
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;
        auto pct = [] (float v, int) { return String (roundToInt (v)) + " %"; };

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { dry, 1 }, "Dry", NormalisableRange<float> (0, 100), 100.0f,
                                                           AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { wet, 1 }, "Wet", NormalisableRange<float> (0, 100), 70.0f,
                                                           AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

        for (int b = 0; b < numBands; ++b)
        {
            const auto& d = defaults (b);
            const String n = "Band " + String (b + 1) + " ";
            auto group = std::make_unique<AudioProcessorParameterGroup> ("band" + String (b), "Band " + String (b + 1), "|");

            group->addChild (std::make_unique<AudioParameterBool> (ParameterID { id (b, on), 1 }, n + "On", d.on));
            NormalisableRange<float> fr (20.0f, 20000.0f);
            fr.setSkewForCentre (1000.0f);
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, freq), 1 }, n + "Frequency", fr, d.freq,
                AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (roundToInt (v)) + " Hz"; })));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, level), 1 }, n + "Level", NormalisableRange<float> (0, 1), d.level,
                AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return levelText (v); })));
            NormalisableRange<float> qr (0.3f, 12.0f);
            qr.setSkewForCentre (2.0f);
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, q), 1 }, n + "Q", qr, d.q,
                AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 2); })));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id (b, engine), 1 }, n + "Engine", engineNames(), d.mode == 3 ? 1 : 0));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id (b, sync), 1 }, n + "Time", syncNames(), d.sync));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id (b, feel), 1 }, n + "Feel", feelNames(), d.feel));
            NormalisableRange<float> msr (1.0f, 2000.0f);
            msr.setSkewForCentre (250.0f);
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, ms), 1 }, n + "Time ms", msr, 250.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, fb), 1 }, n + "Feedback", NormalisableRange<float> (0, 100), 45.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id (b, mode), 1 }, n + "Mode", modeNames(), d.mode));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, amt), 1 }, n + "Mode Amount", NormalisableRange<float> (0, 100), 30.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, duck), 1 }, n + "Duck", NormalisableRange<float> (0, 100), 0.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
            NormalisableRange<float> ar (0.1f, 200.0f);
            ar.setSkewForCentre (10.0f);
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, datk), 1 }, n + "Duck Attack", ar, 5.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v < 10.0f ? String (v, 1) + " ms" : String (roundToInt (v)) + " ms"; })));
            NormalisableRange<float> rr (10.0f, 3000.0f);
            rr.setSkewForCentre (250.0f);
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, drel), 1 }, n + "Duck Release", rr, 250.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : String (roundToInt (v)) + " ms"; })));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, drive), 1 }, n + "Drive", NormalisableRange<float> (0, 100), 0.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id (b, dtype), 1 }, n + "Drive Type", driveNames(), 0));
            NormalisableRange<float> sr (2.0f, 50.0f);
            sr.setSkewForCentre (12.0f);
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, size), 1 }, n + "Size", sr, 12.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, diff), 1 }, n + "Diffuse", NormalisableRange<float> (0, 100), 0.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id (b, mix), 1 }, n + "Mix", NormalisableRange<float> (0, 100), 100.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
            layout.add (std::move (group));
        }
        return layout;
    }
}
