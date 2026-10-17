#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

// Factory presets plus user presets (XML files in Documents/Primal Rage Lab/Dellayrium/Presets)
class PresetManager : private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState&);
    ~PresetManager() override;

    static juce::File getFolder();
    static constexpr const char* extension = ".dlypreset";

    juce::StringArray getPresetNames() const;          // user presets
    static juce::StringArray getFactoryNames();
    bool loadFactory (int index);
    bool isFactory (const juce::String& name) const { return getFactoryNames().contains (name); }
    juce::String getCurrentName() const { return currentName; }
    bool isModified() const { return modified.load(); }

    void loadInit();
    bool load (const juce::String& name);
    bool save (const juce::String& name);
    bool remove (const juce::String& name);
    bool rename (const juce::String& from, const juce::String& to);
    void step (int direction);   // previous / next preset (factory, then user)

    // The current preset name travels with the project
    void setCurrentNameFromState (const juce::String& name) { currentName = name.isEmpty() ? "Init" : name; }

private:
    void parameterChanged (const juce::String&, float) override;
    void markClean();

    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentName { "Init" };
    std::atomic<bool> modified { false };
    std::atomic<juce::uint32> ignoreUntil { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
