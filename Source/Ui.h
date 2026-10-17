#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Theme.h"

juce::Font uiFont (float height, bool bold = false, float kerning = 0.0f);
inline const juce::Colour kInk (0xffececf0), kMute (0xffa5a5ad), kWidget (0xff1b1b1f);

// The 8 band colours follow the theme: a fan of hues around its accent
juce::Colour bandColour (const Theme&, int band);

class DLook : public juce::LookAndFeel_V4
{
public:
    DLook();
    void setTheme (const Theme&);
    const Theme& theme() const { return *current; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;

private:
    const Theme* current = nullptr;
};

const Theme& themeOf (const juce::Component&);
juce::Colour accentOf (const juce::Component&);   // the selected band's colour when there is one

//==============================================================================
// Knob with its name above and value below. The arc follows `arcColour`.
class Knob : public juce::Component
{
public:
    Knob();
    void resized() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    juce::Slider slider;
    juce::String label;
    juce::Colour arcColour { 0xffff73b2 };
    bool showLabel = true;
    std::function<float()> gainReduction;   // dB, for the duck ring
    std::function<juce::String()> valueText;
};

// Row of exclusive buttons bound to a choice parameter
class Segmented : public juce::Component, public juce::SettableTooltipClient
{
public:
    Segmented (juce::StringArray items, bool boldCaps);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void bind (juce::RangedAudioParameter*);
    std::function<void (int)> onChange;
    int index = 0;
    juce::Colour colour { 0xffff73b2 };
    float fontSize = 12.0f;
private:
    juce::StringArray items;
    bool caps;
    std::unique_ptr<juce::ParameterAttachment> attach;
};

//==============================================================================
class DellayriumEditor;

class Screen : public juce::Component, private juce::Timer
{
public:
    Screen (DellayriumProcessor&, DellayriumEditor&);
    ~Screen() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    juce::Rectangle<float> screenArea() const;
    float bottomReserve = 0.0f;           // space taken by the drawer
    bool analyzerOn = true;
    void renderOnce();                    // test helper: run the analyzer without waiting for the timer

private:
    DellayriumProcessor& proc;
    DellayriumEditor& editor;
    juce::Rectangle<float> plot;
    juce::Image glassCache;

    // analyzer
    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder, cols = 240, slices = 24;
    juce::dsp::FFT fft { fftOrder };
    std::vector<float> ring = std::vector<float> (fftSize, 0.0f), fftData = std::vector<float> ((size_t) fftSize * 2, 0.0f), window;
    int ringPos = 0, frameCounter = 0;
    std::vector<float> live = std::vector<float> (cols, 0.0f);
    std::deque<std::vector<float>> history;

    struct Spark { float x, y, vx, vy, life, size; juce::Colour c; };
    std::vector<Spark> sparks;
    juce::Random rnd;

    int hover = -1, dragging = -1;
    bool dragMoved = false;
    juce::Point<float> dragStart;

    void timerCallback() override;
    void pullAnalyzer();
    float xFor (float f) const;
    float fFor (float x) const;
    float zeroY() const;
    float bellH() const;
    float bandResponse (int band, float f) const;
    juce::Point<float> nodePos (int band) const;
    int nodeAt (juce::Point<float>) const;
    juce::Rectangle<float> tagRect (int band) const;
    juce::Rectangle<float> soloRect() const;
    juce::Rectangle<float> analyzerRect() const;
    bool bandOn (int b) const;
    float param (int b, const char* name) const;
    void setParam (int b, const char* name, float value);
    juce::Colour colourAt (float f, float& envelope) const;
    void showBandMenu (int band);
    void addPointAt (juce::Point<float>);
};

//==============================================================================
class Drawer : public juce::Component, private juce::Timer
{
public:
    Drawer (DellayriumProcessor&, DellayriumEditor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void bindBand (int band);
    void lookAndFeelChanged() override { updateColours(); }
    void parentHierarchyChanged() override { updateColours(); }
    void setOpen (bool);
    bool isOpen() const { return open; }
    float visibleHeight() const;
    static constexpr int fullHeight = 214, tabHeight = 46;
    juce::Path shape() const;
    bool hitTest (int x, int y) override;

private:
    DellayriumProcessor& proc;
    DellayriumEditor& editor;
    int band = 0;
    bool open = true;
    float openness = 1.0f;

    Knob time, timeMs, feedback, modeKnob, duck, drive, size, diffuse, mix;
    Segmented feel { { "1/1", ".", "T", "ms" }, false };
    Segmented engine { { "ANALOG", "FFT" }, true };
    juce::ComboBox modeBox, driveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aTime, aMs, aFb, aAmt, aDuck, aDrive, aSize, aDiff, aMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aMode, aDtype;
    std::unique_ptr<juce::ParameterAttachment> onAttach;
    bool bandEnabled = true;

    void timerCallback() override;
    void updateColours();
    void updateModeLabel();
    juce::Rectangle<float> chevronRect() const;
    juce::Rectangle<float> powerRect() const;
};

//==============================================================================
class MeterColumn : public juce::Component, private juce::Timer
{
public:
    MeterColumn (DellayriumProcessor&, bool isOutput);
    void paint (juce::Graphics&) override;
    void resized() override;
    Knob knob;
private:
    DellayriumProcessor& proc;
    bool output;
    float level[2] {}, peakHold[2] {}, peakDb = -100.0f;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    void timerCallback() override;
};

//==============================================================================
class PresetBar : public juce::Component, private juce::Timer
{
public:
    explicit PresetBar (DellayriumProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    DellayriumProcessor& proc;
    juce::Rectangle<float> prevR() const, nextR() const, nameR() const, saveR() const;
    void showMenu();
    void askForName (const juce::String& title, const juce::String& initial, std::function<void (const juce::String&)> onOk);
    void timerCallback() override { repaint(); }
};
