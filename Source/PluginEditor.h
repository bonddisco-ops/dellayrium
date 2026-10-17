#pragma once
#include "Ui.h"

class DellayriumEditor : public juce::AudioProcessorEditor
{
public:
    explicit DellayriumEditor (DellayriumProcessor&);
    ~DellayriumEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    void selectBand (int band, bool openDrawer = true);
    int selectedBand() const { return proc.getSelectedBand(); }
    void applyTheme (const juce::String& name);
    void drawerMoved();               // the drawer animates; the screen leaves room for it
    void showToast (const juce::String&);

    static constexpr int baseW = 1560, baseH = 860;

private:
    DellayriumProcessor& proc;
    DLook look;
    MeterColumn inCol { proc, false }, outCol { proc, true };
    PresetBar presetBar { proc };
    Screen screen { proc, *this };
    Drawer drawer { proc, *this };
    TourOverlay tour;
    WelcomePrompt welcome;
    juce::Image background;
    juce::Rectangle<int> logoArea, gearArea;
    juce::String toast;
    juce::uint32 toastUntil = 0;
    bool ready = false, helpOn = true;
    Help::Lang lang = Help::Lang::en;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    void buildBackground();
    void showGearMenu();
    void applyHelp();
    juce::Rectangle<float> tourTarget (const juce::String&);
    static juce::String loadSetting (const juce::String& key, const juce::String& fallback);
    static void saveSetting (const juce::String& key, const juce::String& value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DellayriumEditor)
};
