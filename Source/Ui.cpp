#include "PluginEditor.h"
#include "Logo.h"

using namespace juce;

namespace
{
Point<float> polar (float cx, float cy, float r, float a) { return { cx + std::sin (a) * r, cy - std::cos (a) * r }; }

String freqText (float f)
{
    return f >= 999.5f ? String (f / 1000.0f, f >= 9995.0f ? 1 : 2) + " kHz" : String (roundToInt (f)) + " Hz";
}
}

Font uiFont (float height, bool bold, float kerning)
{
    Font f (FontOptions (height, bold ? Font::bold : Font::plain));
    if (kerning != 0.0f)
        f.setExtraKerningFactor (kerning);
    return f;
}

Colour bandColour (const Theme& t, int band)
{
    const float h = t.accent.getHue(), s = t.accent.getSaturation(), v = t.accent.getBrightness();
    const float rel = (float) band - 3.0f;
    if (t.gloss)   // Luxe: one gold family, from deep amber to pale champagne
        return Colour::fromHSV (h - rel * 0.012f, jlimit (0.2f, 1.0f, s - rel * 0.06f), jlimit (0.5f, 1.0f, v + rel * 0.03f), 1.0f);
    float hue = h - rel * 0.075f;
    hue -= std::floor (hue);
    return Colour::fromHSV (hue, jlimit (0.45f, 0.9f, s * 0.95f), jmax (0.85f, v), 1.0f);
}

//==============================================================================
DLook::DLook() { setTheme (Theme::all().front()); }

void DLook::setTheme (const Theme& t)
{
    current = &t;
    setColour (ResizableWindow::backgroundColourId, t.bg);
    setColour (ComboBox::textColourId, kInk);
    setColour (ComboBox::backgroundColourId, kWidget);
    setColour (ComboBox::outlineColourId, Colours::white.withAlpha (0.10f));
    setColour (PopupMenu::backgroundColourId, kWidget);
    setColour (PopupMenu::textColourId, kInk);
    setColour (PopupMenu::headerTextColourId, t.accent);
    setColour (PopupMenu::highlightedBackgroundColourId, t.accent);
    setColour (PopupMenu::highlightedTextColourId, Colour (0xff111114));
    setColour (AlertWindow::backgroundColourId, kWidget);
    setColour (AlertWindow::textColourId, kInk);
    setColour (AlertWindow::outlineColourId, t.accent);
    setColour (TextEditor::backgroundColourId, Colour (0xff111114));
    setColour (TextEditor::textColourId, kInk);
    setColour (TextEditor::focusedOutlineColourId, t.accent);
    setColour (TextButton::buttonColourId, Colour (0xff26262b));
    setColour (TextButton::textColourOffId, kInk);
    setColour (TooltipWindow::textColourId, kInk);
    setColour (Label::textColourId, kInk);
}

const Theme& themeOf (const Component& c)
{
    if (auto* l = dynamic_cast<DLook*> (&c.getLookAndFeel()))
        return l->theme();
    return Theme::all().front();
}

void DLook::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float pos, float startAngle, float endAngle, Slider& slider)
{
    const auto& t = theme();
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat();
    const float R = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 9.0f;
    if (R < 6.0f)
        return;
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const bool compact = R < 22.0f;
    const auto arcCol = slider.findColour (Slider::rotarySliderFillColourId);

    Path track;
    track.addCentredArc (cx, cy, R + 6.0f, R + 6.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.strokePath (track, PathStrokeType (compact ? 2.5f : 3.0f, PathStrokeType::curved, PathStrokeType::rounded));

    const float valueAngle = startAngle + (endAngle - startAngle) * pos;
    if (pos > 0.002f)
    {
        Path value;
        value.addCentredArc (cx, cy, R + 6.0f, R + 6.0f, 0.0f, startAngle, valueAngle, true);
        const auto lit = arcCol.interpolatedWith (t.accentDim, 0.35f * (1.0f - pos));
        g.setColour (lit.withAlpha (0.35f));
        g.strokePath (value, PathStrokeType (compact ? 5.0f : 6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (lit);
        g.strokePath (value, PathStrokeType (compact ? 2.5f : 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    {
        const float sr = R * 1.35f;
        ColourGradient shadow (Colours::black.withAlpha (0.65f), cx + 3.0f, cy + 7.0f, Colours::transparentBlack, cx + 3.0f + sr, cy + 7.0f, true);
        g.setGradientFill (shadow);
        g.fillEllipse (cx + 3.0f - sr, cy + 7.0f - sr, sr * 2.0f, sr * 2.0f);
    }
    if (t.gloss)
    {
        ColourGradient rim (Colour (0xfff6dc9a), cx, cy - R - 2.0f, Colour (0xffd9b96a), cx, cy + R + 2.0f, false);
        rim.addColour (0.5, Colour (0xff8a6a2a));
        g.setGradientFill (rim);
        g.fillEllipse (cx - R - 2.0f, cy - R - 2.0f, (R + 2.0f) * 2.0f, (R + 2.0f) * 2.0f);
        ColourGradient body (Colour (0xff3a3a42), cx, cy - R * 0.5f, Colour (0xff050506), cx, cy + R, true);
        g.setGradientFill (body);
        g.fillEllipse (cx - R + 0.5f, cy - R + 0.5f, R * 2.0f - 1.0f, R * 2.0f - 1.0f);
        Graphics::ScopedSaveState s (g);
        Path clip;
        clip.addEllipse (cx - R + 1.0f, cy - R + 1.0f, R * 2.0f - 2.0f, R * 2.0f - 2.0f);
        g.reduceClipRegion (clip);
        ColourGradient spec (Colours::white.withAlpha (0.55f), cx, cy - R, Colours::white.withAlpha (0.0f), cx, cy, false);
        g.setGradientFill (spec);
        g.fillEllipse (cx - R * 0.75f, cy - R * 0.87f, R * 1.5f, R * 0.84f);
    }
    else
    {
        const int teeth = compact ? 36 : 44;
        for (int i = 0; i < teeth; ++i)
        {
            const float a = (float) i / (float) teeth * MathConstants<float>::twoPi;
            g.setColour (i % 2 == 0 ? Colour (0xff55555b) : Colour (0xff2b2b2e));
            g.drawLine (Line<float> (polar (cx, cy, R - 1.0f, a), polar (cx, cy, R + 2.0f, a)), compact ? 1.6f : 2.0f);
        }
        ColourGradient body (Colour (0xff6a6a70), cx, cy - R, Colour (0xff1c1c20), cx, cy + R, false);
        body.addColour (0.5, Colour (0xff38383d));
        g.setGradientFill (body);
        g.fillEllipse (cx - R + 1.0f, cy - R + 1.0f, R * 2.0f - 2.0f, R * 2.0f - 2.0f);
        const float capR = R * 0.74f;
        ColourGradient cap (Colour (0xff8c8c93), cx - R * 0.3f, cy - R * 0.4f, Colour (0xff26262a), cx - R * 0.3f + R * 0.95f, cy - R * 0.4f, true);
        cap.addColour (0.45, Colour (0xff4a4a50));
        g.setGradientFill (cap);
        g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
        Path rimHi;
        rimHi.addCentredArc (cx, cy, capR, capR, 0.0f, -1.1f, 1.1f, true);
        g.setColour (Colours::white.withAlpha (0.18f));
        g.strokePath (rimHi, PathStrokeType (1.0f));
    }
    Path ptr;
    ptr.startNewSubPath (polar (cx, cy, R * 0.2f, valueAngle));
    ptr.lineTo (polar (cx, cy, R * 0.66f, valueAngle));
    g.setColour (t.gloss ? Colour (0xfff6dc9a) : Colour (0xfff0f0f4));
    g.strokePath (ptr, PathStrokeType (compact ? 2.4f : 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

void DLook::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (kWidget);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Colours::white.withAlpha (box.isMouseOver (true) ? 0.18f : 0.10f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    Path arrow;
    const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
    arrow.startNewSubPath (ax - 4.5f, ay - 2.5f);
    arrow.lineTo (ax, ay + 2.5f);
    arrow.lineTo (ax + 4.5f, ay - 2.5f);
    g.setColour (box.findColour (ComboBox::arrowColourId));
    g.strokePath (arrow, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

void DLook::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (4, 0, box.getWidth() - 24, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

Font DLook::getComboBoxFont (ComboBox& box) { return uiFont (box.getHeight() < 26 ? 11.5f : 13.0f, true, 0.06f); }
Font DLook::getPopupMenuFont() { return uiFont (14.0f); }

Rectangle<int> DLook::getTooltipBounds (const String& tip, Point<int> pos, Rectangle<int> parent)
{
    AttributedString s;
    s.append (tip, uiFont (13.0f), kInk);
    TextLayout tl;
    tl.createLayout (s, 300.0f);
    const int w = (int) std::ceil (tl.getWidth()) + 22, h = (int) std::ceil (tl.getHeight()) + 16;
    const int x = pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 18;
    const int y = pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18;
    return Rectangle<int> (x, y, w, h).constrainedWithin (parent);
}

void DLook::drawTooltip (Graphics& g, const String& text, int w, int h)
{
    const auto r = Rectangle<int> (w, h).toFloat();
    g.setColour (Colour (0xf2141417));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (theme().accent.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
    AttributedString s;
    s.append (text, uiFont (13.0f), kInk);
    TextLayout tl;
    tl.createLayout (s, (float) w - 22.0f);
    tl.draw (g, r.reduced (11.0f, 8.0f));
}

void DLook::drawButtonBackground (Graphics& g, Button& b, const Colour& base, bool hi, bool)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (hi ? base.brighter (0.12f) : base);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Colours::white.withAlpha (0.1f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void DLook::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setColour (b.findColour (TextButton::textColourOffId));
    g.setFont (uiFont (14.0f));
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
}

//==============================================================================
Knob::Knob()
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    slider.setDoubleClickReturnValue (false, 0.0);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    if (showLabel) r.removeFromTop (18);
    r.removeFromBottom (22);
    const int s = jmin (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (s, s));
    slider.setColour (Slider::rotarySliderFillColourId, arcColour);
}

void Knob::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    const bool onDark = (bool) getProperties().getWithDefault ("onDark", true);
    const auto ink = onDark || ! t.lightBg ? kInk : t.ink;
    if (showLabel)
    {
        g.setColour (ink.withAlpha (0.85f));
        g.setFont (uiFont (13.0f));
        g.drawText (label, getLocalBounds().removeFromTop (16), Justification::centred);
    }
    g.setColour (ink);
    g.setFont (uiFont (14.0f));
    const auto txt = valueText ? valueText() : slider.getTextFromValue (slider.getValue());
    g.drawText (txt, getLocalBounds().removeFromBottom (20), Justification::centred);
}

void Knob::paintOverChildren (Graphics& g)
{
    if (! gainReduction)
        return;
    const float db = gainReduction();
    const float amt = jlimit (0.0f, 1.0f, -db / 24.0f);
    if (amt < 0.01f)
        return;
    const auto b = slider.getBounds().toFloat();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f - 9.0f + 11.0f;
    Path gr;
    gr.addCentredArc (b.getCentreX(), b.getCentreY(), R, R, 0.0f, 0.0f, -amt * 2.4f, true);
    const Colour orange (0xffffb35a);
    g.setColour (orange.withAlpha (0.3f));
    g.strokePath (gr, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (orange);
    g.strokePath (gr, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

//==============================================================================
Segmented::Segmented (StringArray i, bool boldCaps) : items (std::move (i)), caps (boldCaps)
{
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void Segmented::bind (RangedAudioParameter* p)
{
    attach.reset();
    if (p == nullptr)
        return;
    attach = std::make_unique<ParameterAttachment> (*p, [this] (float v) { index = roundToInt (v); repaint(); });
    attach->sendInitialUpdate();
}

void Segmented::paint (Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0e0e10));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
    const float w = r.getWidth() / (float) items.size();
    for (int i = 0; i < items.size(); ++i)
    {
        auto cell = Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight()).reduced (2.0f);
        if (i == index)
        {
            g.setColour (colour);
            g.fillRoundedRectangle (cell, 5.0f);
        }
        g.setColour (i == index ? Colour (0xff141016) : kInk.withAlpha (0.85f));
        g.setFont (uiFont (fontSize, caps || i == index, caps ? 0.12f : 0.0f));
        g.drawText (items[i], cell, Justification::centred);
    }
}

void Segmented::mouseDown (const MouseEvent& e)
{
    const int i = jlimit (0, items.size() - 1, (int) ((float) e.x / (float) getWidth() * (float) items.size()));
    index = i;
    if (attach) attach->setValueAsCompleteGesture ((float) i);
    if (onChange) onChange (i);
    repaint();
}

//==============================================================================
// Screen
//==============================================================================
Screen::Screen (DellayriumProcessor& p, DellayriumEditor& e) : proc (p), editor (e)
{
    window.resize ((size_t) fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * (float) i / (float) (fftSize - 1));
    setOpaque (false);
    startTimerHz (30);
}

Screen::~Screen() { stopTimer(); }

Rectangle<float> Screen::screenArea() const { return getLocalBounds().toFloat().reduced (12.0f); }

void Screen::resized()
{
    const auto s = screenArea();
    plot = { s.getX() + 62.0f, s.getY() + 74.0f, s.getWidth() - 124.0f, s.getHeight() - 110.0f };
    glassCache = {};
}

float Screen::xFor (float f) const { return plot.getX() + (std::log10 (jmax (1.0f, f)) - std::log10 (20.0f)) / 3.0f * plot.getWidth(); }
float Screen::fFor (float x) const { return std::pow (10.0f, std::log10 (20.0f) + (x - plot.getX()) / plot.getWidth() * 3.0f); }
float Screen::zeroY() const { return plot.getY() + jmin (plot.getHeight() * 0.48f, 330.0f); }
float Screen::bellH() const { return zeroY() - plot.getY() - 34.0f; }

float Screen::param (int b, const char* name) const
{
    if (auto* p = proc.apvts.getRawParameterValue (Params::id (b, name)))
        return p->load();
    return 0.0f;
}

void Screen::setParam (int b, const char* name, float v)
{
    if (auto* p = proc.apvts.getParameter (Params::id (b, name)))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

bool Screen::bandOn (int b) const { return param (b, Params::on) > 0.5f; }

float Screen::bandResponse (int b, float f) const
{
    if (! bandOn (b))
        return 0.0f;
    const float fc = param (b, Params::freq), q = param (b, Params::q);
    const float shape = param (b, Params::engine) > 0.5f ? dly::fftResponse (f, fc, q) : dly::analogResponse (f, fc, q);
    return param (b, Params::level) * shape;
}

Point<float> Screen::nodePos (int b) const
{
    return { xFor (param (b, Params::freq)), zeroY() - param (b, Params::level) * bellH() };
}

int Screen::nodeAt (Point<float> p) const
{
    int best = -1;
    float bestD = 16.0f;
    for (int b = 0; b < Params::numBands; ++b)
        if (bandOn (b))
        {
            const float d = nodePos (b).getDistanceFrom (p);
            if (d < bestD) { bestD = d; best = b; }
        }
    return best;
}

Colour Screen::colourAt (float f, float& env) const
{
    const auto& t = themeOf (*this);
    float sumP = 0.0f, wsum = 0.0f, r = 0.0f, gg = 0.0f, bb = 0.0f;
    for (int b = 0; b < Params::numBands; ++b)
    {
        const float v = bandResponse (b, f);
        if (v <= 0.0f) continue;
        sumP += v * v * v * v;
        const float w = v * v * v + 1.0e-7f;
        const auto c = bandColour (t, b);
        wsum += w;
        r += c.getFloatRed() * w; gg += c.getFloatGreen() * w; bb += c.getFloatBlue() * w;
    }
    env = std::pow (sumP, 0.25f);
    if (wsum <= 1.0e-6f)
        return t.accentDim;
    const auto c = Colour::fromFloatRGBA (r / wsum, gg / wsum, bb / wsum, 1.0f);
    return c.interpolatedWith (t.accentDim, jlimit (0.0f, 1.0f, 1.0f - env * 8.0f));
}

String timeLabel (DellayriumProcessor& proc, int b, bool full)
{
    const int feel = roundToInt (proc.apvts.getRawParameterValue (Params::id (b, Params::feel))->load());
    if (feel == 3)
        return String (roundToInt (proc.apvts.getRawParameterValue (Params::id (b, Params::ms))->load())) + " ms";
    auto t = Params::syncNames()[roundToInt (proc.apvts.getRawParameterValue (Params::id (b, Params::sync))->load())];
    if (feel == 1) t += ".";
    if (feel == 2) t += "T";
    ignoreUnused (full);
    return t;
}

Rectangle<float> Screen::tagRect (int b) const
{
    const auto p = nodePos (b);
    const bool sel = b == editor.selectedBand();
    String text = timeLabel (proc, b, sel);
    if (sel)
        text = freqText (param (b, Params::freq)) + "   " + text + "   " + Params::modeNames()[roundToInt (param (b, Params::mode))].toUpperCase();
    const float tw = (float) GlyphArrangement::getStringWidthInt (uiFont (sel ? 13.0f : 11.0f), text) + (sel ? 64.0f : 20.0f);
    auto r = Rectangle<float> (tw, sel ? 30.0f : 22.0f).withCentre ({ p.x, p.y - (sel ? 44.0f : 32.0f) });
    if (r.getY() < plot.getY() - 52.0f) r.setY (plot.getY() - 52.0f);
    const auto s = screenArea();
    if (r.getX() < s.getX() + 10) r.setX (s.getX() + 10);
    if (r.getRight() > s.getRight() - 10) r.setX (s.getRight() - 10 - r.getWidth());
    return r;
}

Rectangle<float> Screen::soloRect() const
{
    const auto r = tagRect (editor.selectedBand());
    return { r.getX() + 5, r.getY() + 4, 26, 22 };
}

Rectangle<float> Screen::analyzerRect() const
{
    const auto s = screenArea();
    return { s.getX() + 22, s.getY() + 16, 220, 30 };
}

void Screen::pullAnalyzer()
{
    auto& fifo = proc.scopeFifo;
    int s1, n1, s2, n2;
    const int ready = fifo.getNumReady();
    fifo.prepareToRead (ready, s1, n1, s2, n2);
    auto take = [this] (int start, int num)
    {
        for (int i = 0; i < num; ++i)
        {
            ring[(size_t) ringPos] = proc.scopeBuf[(size_t) (start + i)];
            ringPos = (ringPos + 1) % fftSize;
        }
    };
    take (s1, n1);
    take (s2, n2);
    fifo.finishedRead (n1 + n2);

    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = ring[(size_t) ((ringPos + i) % fftSize)] * window[(size_t) i];
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    const float sr = (float) proc.getSampleRateSafe();
    const float binHz = sr / (float) fftSize;
    for (int c = 0; c < cols; ++c)
    {
        const float f0 = 20.0f * std::pow (1000.0f, (float) c / (float) cols);
        const float f1 = 20.0f * std::pow (1000.0f, (float) (c + 1) / (float) cols);
        int k0 = (int) (f0 / binHz), k1 = jmax (k0 + 1, (int) (f1 / binHz));
        k1 = jmin (k1, fftSize / 2);
        float m = 0.0f;
        for (int k = k0; k < k1; ++k) m = jmax (m, fftData[(size_t) k]);
        // tilt +4.5 dB/oct so a mix looks flat-ish, like analyzers usually do
        const float db = Decibels::gainToDecibels (m / (float) fftSize * 4.0f, -120.0f) + 4.5f * std::log2 (jmax (20.0f, f0) / 1000.0f);
        const float v = jlimit (0.0f, 1.0f, (db + 78.0f) / 78.0f);
        live[(size_t) c] = jmax (v, live[(size_t) c] * 0.82f);
    }
    if (++frameCounter % 2 == 0)
    {
        history.push_front (live);
        while ((int) history.size() > slices) history.pop_back();
    }
}

void Screen::renderOnce()
{
    pullAnalyzer();
    timerCallback();
}

void Screen::timerCallback()
{
    if (analyzerOn)
        pullAnalyzer();
    else if (! history.empty())
        history.clear();

    // sparks rise where a band is ringing
    const auto& t = themeOf (*this);
    const float H = bellH(), zy = zeroY();
    for (int b = 0; b < Params::numBands; ++b)
    {
        if (! bandOn (b)) continue;
        const float e = proc.bandEnergy[b].load();
        float expect = jlimit (0.0f, 4.0f, e * 10.0f);
        while (expect > 0.0f)
        {
            if (rnd.nextFloat() < jmin (1.0f, expect))
            {
                const float fc = param (b, Params::freq), q = param (b, Params::q);
                const float spread = (rnd.nextFloat() + rnd.nextFloat() - 1.0f) * 0.9f / jmax (0.3f, q);
                const float f = fc * std::pow (2.0f, spread);
                float env;
                colourAt (f, env);
                const float x = xFor (f), y = zy - env * H;
                sparks.push_back ({ x, y, (rnd.nextFloat() - 0.5f) * 0.8f, -0.4f - rnd.nextFloat() * 1.2f, 1.0f,
                                    1.2f + rnd.nextFloat() * 2.2f, bandColour (t, b) });
            }
            expect -= 1.0f;
        }
    }
    for (auto& s : sparks)
    {
        s.x += s.vx;
        s.y += s.vy;
        s.vy *= 0.97f;
        s.life -= 0.035f;
    }
    sparks.erase (std::remove_if (sparks.begin(), sparks.end(), [] (const Spark& s) { return s.life <= 0.0f; }), sparks.end());
    if (sparks.size() > 400) sparks.erase (sparks.begin(), sparks.begin() + (long) (sparks.size() - 400));
    repaint();
}

void Screen::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto s = screenArea();
    const float zy = zeroY(), H = bellH();

    // ---- static glass and grid, cached
    if (! glassCache.isValid() || glassCache.getWidth() != getWidth())
    {
        glassCache = Image (Image::ARGB, getWidth(), getHeight(), true);
        Graphics gc (glassCache);
        const auto outer = getLocalBounds().toFloat();
        if (t.gloss)
        {
            ColourGradient bez (Colour (0xfff3d892), 0.0f, outer.getY(), Colour (0xffe2c47a), 0.0f, outer.getBottom(), false);
            bez.addColour (0.12, Colour (0xff9a7a35));
            bez.addColour (0.5, Colour (0xff2a2214));
            bez.addColour (0.9, Colour (0xff7a5f28));
            gc.setGradientFill (bez);
            gc.fillRoundedRectangle (outer, 18.0f);
            gc.setColour (Colour (0xff050506));
            gc.fillRoundedRectangle (outer.reduced (3.0f), 16.0f);
        }
        else
        {
            gc.setColour (Colour (0xff09090b));
            gc.fillRoundedRectangle (outer, 18.0f);
            gc.setColour (Colours::white.withAlpha (0.08f));
            gc.drawRoundedRectangle (outer.reduced (0.5f), 18.0f, 1.0f);
        }
        Path clip;
        clip.addRoundedRectangle (s, 26.0f);
        gc.reduceClipRegion (clip);
        ColourGradient glass (Colour (0xff1d1c22), s.getCentreX(), s.getCentreY(), Colour (0xff060608), s.getCentreX() + s.getWidth() * 0.6f, s.getCentreY(), true);
        gc.setGradientFill (glass);
        gc.fillRect (s);
        gc.setFont (uiFont (11.0f));
        for (auto [f, l] : std::initializer_list<std::pair<float, const char*>> { { 50, "50" }, { 100, "100" }, { 200, "200" }, { 500, "500" }, { 1000, "1k" }, { 2000, "2k" }, { 5000, "5k" }, { 10000, "10k" } })
        {
            const float x = xFor (f);
            gc.setColour (Colours::white.withAlpha (0.045f));
            gc.drawVerticalLine (roundToInt (x), plot.getY() - 6, zy);
            gc.setColour (Colours::white.withAlpha (0.3f));
            gc.drawText (l, Rectangle<float> (x - 20, plot.getY() - 24, 40, 14), Justification::centred);
        }
        gc.setColour (Colours::white.withAlpha (0.12f));
        gc.drawHorizontalLine (roundToInt (zy), plot.getX(), plot.getRight());
        for (auto [v, l] : std::initializer_list<std::pair<float, const char*>> { { 1.0f, "0" }, { 0.707f, "-6" }, { 0.5f, "-12" }, { 0.25f, "-24" } })
        {
            const float y = zy - H * v;
            gc.setColour (Colours::white.withAlpha (0.04f));
            gc.drawHorizontalLine (roundToInt (y), plot.getX(), plot.getRight());
            gc.setColour (Colours::white.withAlpha (0.3f));
            gc.drawText (l, Rectangle<float> (plot.getX() - 44, y - 7, 34, 14), Justification::centredRight);
            gc.drawText (l, Rectangle<float> (plot.getRight() + 10, y - 7, 34, 14), Justification::centredLeft);
        }
        gc.setColour (Colours::white.withAlpha (0.3f));
        gc.drawText ("SEND", Rectangle<float> (plot.getRight() + 6, zy - H - 28, 50, 14), Justification::centredLeft);
    }
    g.drawImageAt (glassCache, 0, 0);

    Graphics::ScopedSaveState ss (g);
    {
        Path clip;
        clip.addRoundedRectangle (s, 26.0f);
        g.reduceClipRegion (clip);
    }

    // ---- per-x colour of the bands (shared by the waterfall and the curve)
    const int steps = jmax (60, (int) (plot.getWidth() / 4.0f));
    std::vector<float> envs ((size_t) steps + 1);
    ColourGradient bandGrad (t.accentDim, plot.getX(), 0, t.accentDim, plot.getRight(), 0, false);
    bandGrad.clearColours();
    for (int i = 0; i <= steps; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) steps;
        float env;
        const auto c = colourAt (fFor (x), env);
        envs[(size_t) i] = env;
        if (i % 3 == 0 || i == steps)
            bandGrad.addColour ((double) i / steps, c);
    }

    // ---- waterfall: spectrum slices falling down and fading, newest on top
    if (analyzerOn && ! history.empty())
    {
        const float floor = s.getBottom() - bottomReserve - 6.0f;
        const float spacing = jmax (7.0f, (floor - zy - 14.0f) / (float) slices);
        const float amp = H * 0.85f;
        for (int k = (int) history.size() - 1; k >= 0; --k)
        {
            const auto& sl = history[(size_t) k];
            const float age = (float) k / (float) slices;
            const float yOff = zy + 4.0f + (float) k * spacing;
            if (yOff > floor) continue;
            const float squeeze = 1.0f - age * 0.14f;
            const float cx = plot.getCentreX();
            Path line;
            for (int c = 0; c < cols; ++c)
            {
                const float x = plot.getX() + plot.getWidth() * (float) c / (float) (cols - 1);
                const float px = cx + (x - cx) * squeeze;
                const float py = yOff - sl[(size_t) c] * amp * (k == 0 ? 1.0f : 0.42f * (1.0f - age * 0.5f));
                if (c == 0) line.startNewSubPath (px, py); else line.lineTo (px, py);
            }
            Path area (line);
            area.lineTo (cx + (plot.getRight() - cx) * squeeze, yOff + 3);
            area.lineTo (cx + (plot.getX() - cx) * squeeze, yOff + 3);
            area.closeSubPath();
            g.setColour (Colour (0xff0d0d10).withAlpha (k == 0 ? 0.45f : 0.6f));
            g.fillPath (area);
            auto cg = bandGrad;
            cg.point1 = { cx + (plot.getX() - cx) * squeeze, 0 };
            cg.point2 = { cx + (plot.getRight() - cx) * squeeze, 0 };
            cg.multiplyOpacity (0.95f * std::pow (0.91f, (float) k));
            g.setGradientFill (cg);
            if (k == 0)
            {
                auto glow = cg;
                glow.multiplyOpacity (0.3f);
                g.setGradientFill (glow);
                g.strokePath (line, PathStrokeType (5.0f));
                g.setGradientFill (cg);
            }
            g.strokePath (line, PathStrokeType (k == 0 ? 1.8f : 1.2f));
        }
    }

    // ---- the combined curve of all bands
    {
        Path curve;
        for (int i = 0; i <= steps; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) steps;
            const Point<float> p (x, zy - envs[(size_t) i] * H);
            if (i == 0) curve.startNewSubPath (p); else curve.lineTo (p);
        }
        ColourGradient cg (bandGrad);
        cg.clearColours();
        for (int i = 0; i <= steps; i += 3)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) steps;
            float env;
            auto c = colourAt (fFor (x), env);
            cg.addColour ((double) i / steps, c.withAlpha (jlimit (0.25f, 1.0f, env * 6.0f)));
        }
        Path wash (curve);
        wash.lineTo (plot.getRight(), zy);
        wash.lineTo (plot.getX(), zy);
        wash.closeSubPath();
        {
            Graphics::ScopedSaveState ws (g);
            g.reduceClipRegion (wash);
            auto fillG = cg;
            fillG.multiplyOpacity (0.22f);
            g.setGradientFill (fillG);
            g.fillRect (Rectangle<float> (plot.getX(), zy - H - 10, plot.getWidth(), H + 10));
            ColourGradient fade (Colours::transparentBlack, 0, zy - H, Colour (0xff0d0d10).withAlpha (0.85f), 0, zy, false);
            g.setGradientFill (fade);
            g.fillRect (Rectangle<float> (plot.getX(), zy - H - 10, plot.getWidth(), H + 10));
        }
        auto glow = cg;
        glow.multiplyOpacity (0.28f);
        g.setGradientFill (glow);
        g.strokePath (curve, PathStrokeType (8.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill (cg);
        g.strokePath (curve, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));

        const int sel = editor.selectedBand();
        if (bandOn (sel))
        {
            Path own;
            for (int i = 0; i <= steps; ++i)
            {
                const float x = plot.getX() + plot.getWidth() * (float) i / (float) steps;
                const Point<float> p (x, zy - bandResponse (sel, fFor (x)) * H);
                if (i == 0) own.startNewSubPath (p); else own.lineTo (p);
            }
            const float dashes[] = { 4.0f, 5.0f };
            Path dashed;
            PathStrokeType (1.2f).createDashedStroke (dashed, own, dashes, 2);
            g.setColour (bandColour (t, sel).withAlpha (0.5f));
            g.fillPath (dashed);
        }
    }

    // ---- sparks
    for (const auto& sp : sparks)
    {
        const float a = jlimit (0.0f, 1.0f, sp.life);
        g.setColour (sp.c.withAlpha (a * 0.28f));
        g.fillEllipse (Rectangle<float> (sp.size * 6, sp.size * 6).withCentre ({ sp.x, sp.y }));
        g.setColour (sp.c.interpolatedWith (Colours::white, 0.7f).withAlpha (a));
        g.fillEllipse (Rectangle<float> (sp.size * 1.4f, sp.size * 1.4f).withCentre ({ sp.x, sp.y }));
        if (sp.size > 2.6f)
        {
            // a little four-point twinkle on the bigger ones
            g.setColour (Colours::white.withAlpha (a * 0.7f));
            g.drawLine (sp.x - sp.size * 2.2f, sp.y, sp.x + sp.size * 2.2f, sp.y, 0.8f);
            g.drawLine (sp.x, sp.y - sp.size * 2.2f, sp.x, sp.y + sp.size * 2.2f, 0.8f);
        }
    }

    // ---- nodes and tags
    const int sel = editor.selectedBand();
    const int solo = proc.soloBand.load();
    for (int pass = 0; pass < 2; ++pass)
        for (int b = 0; b < Params::numBands; ++b)
        {
            if (! bandOn (b) || (pass == 0) == (b == sel)) continue;   // selected drawn last
            const auto c = bandColour (t, b);
            const bool isSel = b == sel;
            const auto p = nodePos (b);
            const float r = isSel ? 11.0f : (b == hover ? 10.0f : 9.0f);
            g.setColour (c.withAlpha (0.25f));
            g.fillEllipse (Rectangle<float> (r * 2 + 10, r * 2 + 10).withCentre (p));
            g.setColour (Colour (0xff111114));
            g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (p));
            g.setColour (c);
            g.drawEllipse (Rectangle<float> (r * 2 - 3, r * 2 - 3).withCentre (p), 3.0f);
            g.setFont (uiFont (11.0f, true));
            g.drawText (String (b + 1), Rectangle<float> (r * 2, r * 2).withCentre (p), Justification::centred);

            auto box = tagRect (b);
            g.setColour (Colour (0xff111114).withAlpha (0.92f));
            g.fillRoundedRectangle (box, 6.0f);
            g.setColour (c.withAlpha (isSel ? 0.9f : 0.55f));
            g.drawRoundedRectangle (box.reduced (0.5f), 6.0f, 1.2f);
            String text = timeLabel (proc, b, isSel);
            if (isSel)
            {
                text = freqText (param (b, Params::freq)) + "   " + text + "   " + Params::modeNames()[roundToInt (param (b, Params::mode))].toUpperCase();
                const auto hb = soloRect();
                const bool soloOn = solo == b;
                g.setColour (soloOn ? c : c.withAlpha (0.18f));
                g.fillRoundedRectangle (hb, 5.0f);
                if (! soloOn) { g.setColour (c); g.drawRoundedRectangle (hb.reduced (0.5f), 5.0f, 1.0f); }
                Path hp;
                const float cx = hb.getCentreX(), cy = hb.getCentreY() + 1;
                hp.addCentredArc (cx, cy, 6.0f, 6.0f, 0.0f, -1.6f, 1.6f, true);
                g.setColour (soloOn ? Colour (0xff111114) : c);
                g.strokePath (hp, PathStrokeType (1.8f));
                g.fillRoundedRectangle (cx - 7.5f, cy, 3.5f, 6.0f, 1.0f);
                g.fillRoundedRectangle (cx + 4.0f, cy, 3.5f, 6.0f, 1.0f);
                box.removeFromLeft (34.0f);
            }
            g.setColour (c);
            g.setFont (uiFont (isSel ? 13.0f : 11.0f));
            g.drawText (text, box, Justification::centred);
        }

    // ---- glass: scanlines, reflection, vignette
    for (float y = s.getY(); y < s.getBottom(); y += 3.0f)
    {
        g.setColour (Colours::black.withAlpha (0.16f));
        g.fillRect (s.getX(), y, s.getWidth(), 1.0f);
    }
    ColourGradient refl (Colours::white.withAlpha (t.gloss ? 0.14f : 0.08f), 0.0f, s.getY(), Colours::white.withAlpha (0.0f), 0.0f, s.getY() + s.getHeight() * 0.4f, false);
    g.setGradientFill (refl);
    g.fillEllipse (s.getX() + s.getWidth() * 0.32f - s.getWidth() * 0.36f, s.getY() + s.getHeight() * 0.10f - s.getHeight() * 0.24f, s.getWidth() * 0.72f, s.getHeight() * 0.48f);
    ColourGradient vg (Colours::transparentBlack, s.getCentreX(), s.getCentreY(), Colours::black.withAlpha (0.7f), s.getCentreX() + s.getWidth() * 0.62f, s.getCentreY(), true);
    vg.addColour (0.5, Colours::transparentBlack);
    g.setGradientFill (vg);
    g.fillRect (s);

    // ---- top controls and hints
    const auto ar = analyzerRect();
    g.setColour (Colour ((uint8) 10, 10, 12, 0.8f));
    g.fillRoundedRectangle (ar.expanded (5, 4), 8.0f);
    g.setColour (Colour (0xff0e0e10));
    g.fillRoundedRectangle (ar, 7.0f);
    const float hw = ar.getWidth() * 0.5f;
    auto onCell = Rectangle<float> (ar.getX(), ar.getY(), hw, ar.getHeight()).reduced (2.0f);
    auto offCell = Rectangle<float> (ar.getX() + hw, ar.getY(), hw, ar.getHeight()).reduced (2.0f);
    g.setColour (t.accent);
    g.fillRoundedRectangle (analyzerOn ? onCell : offCell, 5.0f);
    g.setFont (uiFont (12.0f, true, 0.12f));
    g.setColour (analyzerOn ? Colour (0xff141016) : kInk);
    g.drawText ("ANALYZER", onCell, Justification::centred);
    g.setColour (analyzerOn ? kInk : Colour (0xff141016));
    g.drawText ("OFF", offCell, Justification::centred);

    g.setColour (Colours::white.withAlpha (0.35f));
    g.setFont (uiFont (12.0f));
    g.drawText ("double-click: add / remove point     wheel on a point: width     right-click: options",
                Rectangle<float> (ar.getRight() + 20, ar.getY(), s.getRight() - ar.getRight() - 42, ar.getHeight()), Justification::centredRight);
}

void Screen::mouseMove (const MouseEvent& e)
{
    const int h = nodeAt (e.position);
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
}

void Screen::mouseDown (const MouseEvent& e)
{
    dragging = -1;
    dragMoved = false;
    if (analyzerRect().contains (e.position))
    {
        analyzerOn = e.position.x < analyzerRect().getCentreX();
        repaint();
        return;
    }
    const int sel = editor.selectedBand();
    if (bandOn (sel) && soloRect().contains (e.position))
    {
        // press the headphones: solo now; drag from here and the point follows while you listen
        soloWasOn = proc.soloBand.load() == sel;
        proc.soloBand = sel;
        soloDrag = true;
        dragging = sel;
        dragStart = e.position;
        grabOffset = nodePos (sel) - e.position;
        for (auto* id : { Params::freq, Params::level })
            if (auto* p = proc.apvts.getParameter (Params::id (sel, id))) p->beginChangeGesture();
        repaint();
        return;
    }
    const int n = nodeAt (e.position);
    if (n < 0)
    {
        if (e.mods.isPopupMenu())
        {
            PopupMenu m;
            m.addItem (1, "Add a point here");
            const auto pos = e.position;
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this, pos] (int r)
            {
                if (r == 1) addPointAt (pos);
            });
        }
        return;
    }
    editor.selectBand (n);
    if (e.mods.isAltDown())
    {
        proc.soloBand = proc.soloBand.load() == n ? -1 : n;
        return;
    }
    if (e.mods.isPopupMenu())
    {
        showBandMenu (n);
        return;
    }
    dragging = n;
    dragStart = e.position;
    grabOffset = nodePos (n) - e.position;
    for (auto* id : { Params::freq, Params::level })
        if (auto* p = proc.apvts.getParameter (Params::id (n, id))) p->beginChangeGesture();
}

void Screen::mouseDrag (const MouseEvent& e)
{
    if (dragging < 0)
        return;
    if (e.getDistanceFromDragStart() > 2)
        dragMoved = true;
    if (! dragMoved)
        return;
    auto pos = e.position;
    if (e.mods.isShiftDown())
        pos = dragStart + (e.position - dragStart) * 0.25f;
    pos += grabOffset;
    const float f = jlimit (20.0f, 20000.0f, fFor (jlimit (plot.getX(), plot.getRight(), pos.x)));
    const float lvl = jlimit (0.0f, 1.0f, (zeroY() - pos.y) / bellH());
    setParam (dragging, Params::freq, f);
    setParam (dragging, Params::level, lvl);
    repaint();
}

void Screen::mouseUp (const MouseEvent&)
{
    if (dragging >= 0)
        for (auto* id : { Params::freq, Params::level })
            if (auto* p = proc.apvts.getParameter (Params::id (dragging, id))) p->endChangeGesture();
    if (soloDrag)
    {
        // a plain click toggles solo; a drag from the headphones only listens while held
        if (dragMoved) proc.soloBand = soloWasOn ? dragging : -1;
        else proc.soloBand = soloWasOn ? -1 : dragging;
        soloDrag = false;
    }
    dragging = -1;
    repaint();
}

Rectangle<float> Screen::tourRect (const String& name) const
{
    const int sel = editor.selectedBand();
    if (name == "screen") return screenArea();
    if (name == "node") return Rectangle<float> (60, 60).withCentre (nodePos (sel));
    if (name == "solo") return tagRect (sel).expanded (4);
    if (name == "analyzer") return { plot.getX(), zeroY() + 4, plot.getWidth(), jmax (40.0f, screenArea().getBottom() - bottomReserve - zeroY() - 10) };
    return {};
}

void Screen::mouseDoubleClick (const MouseEvent& e)
{
    const int n = nodeAt (e.position);
    if (n >= 0)
    {
        setParam (n, Params::on, 0.0f);
        if (proc.soloBand.load() == n) proc.soloBand = -1;
        repaint();
        return;
    }
    if (plot.expanded (0, 40).contains (e.position))
        addPointAt (e.position);
}

void Screen::addPointAt (Point<float> pos)
{
    for (int b = 0; b < Params::numBands; ++b)
        if (! bandOn (b))
        {
            setParam (b, Params::freq, jlimit (20.0f, 20000.0f, fFor (pos.x)));
            setParam (b, Params::level, jlimit (0.05f, 1.0f, (zeroY() - pos.y) / bellH()));
            setParam (b, Params::on, 1.0f);
            editor.selectBand (b);
            return;
        }
    editor.showToast ("All 8 points are in use. Double-click a point to remove it.");
}

void Screen::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    int n = nodeAt (e.position);
    if (n < 0)
    {
        // near the selected bell also works
        const int sel = editor.selectedBand();
        if (bandOn (sel) && std::abs (e.position.x - nodePos (sel).x) < 60.0f && e.position.y < zeroY() + 20)
            n = sel;
    }
    if (n < 0)
        return;
    const float q = param (n, Params::q);
    const float dir = w.isReversed ? -w.deltaY : w.deltaY;
    setParam (n, Params::q, jlimit (0.3f, 12.0f, q * std::pow (1.25f, dir * 4.0f)));
    repaint();
}

void Screen::showBandMenu (int band)
{
    PopupMenu m;
    m.addSectionHeader ("Band " + String (band + 1));
    m.addItem (1, "Solo (listen to this band)", true, proc.soloBand.load() == band);
    m.addItem (2, "Engine: Analog", true, param (band, Params::engine) < 0.5f);
    m.addItem (3, "Engine: FFT", true, param (band, Params::engine) > 0.5f);
    m.addItem (4, "Reset width (Q)");
    m.addSeparator();
    m.addItem (5, "Remove point");
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this, band] (int r)
    {
        if (r == 1) proc.soloBand = proc.soloBand.load() == band ? -1 : band;
        if (r == 2) setParam (band, Params::engine, 0.0f);
        if (r == 3) setParam (band, Params::engine, 1.0f);
        if (r == 4) setParam (band, Params::q, 2.0f);
        if (r == 5) { setParam (band, Params::on, 0.0f); if (proc.soloBand.load() == band) proc.soloBand = -1; }
        repaint();
    });
}

//==============================================================================
// Drawer
//==============================================================================
Drawer::Drawer (DellayriumProcessor& p, DellayriumEditor& e) : proc (p), editor (e)
{
    time.label = "Time";
    timeMs.label = "Time";
    feedback.label = "Feedback";
    duck.label = "Duck";
    drive.label = "Drive";
    size.label = "Size";
    diffuse.label = "Diffuse";
    mix.label = "Mix";
    modeKnob.showLabel = false;
    drive.showLabel = false;
    for (auto* k : { &time, &timeMs, &feedback, &modeKnob, &duck, &drive, &size, &diffuse, &mix })
        addAndMakeVisible (*k);
    duck.gainReduction = [this] { return proc.bandGr[band].load(); };

    modeBox.addItemList ({ "MONO", "PING-PONG", "WIDE", "SPECTRAL WIDE" }, 1);
    driveBox.addItemList ({ "DRIVE: TAPE", "DRIVE: TUBE", "DRIVE: GENTLE", "DRIVE: TRANSFORM" }, 1);
    modeBox.onChange = [this] { updateModeLabel(); };
    addAndMakeVisible (modeBox);
    addAndMakeVisible (driveBox);
    feel.fontSize = 11.0f;
    feel.onChange = [this] (int i) { time.setVisible (i != 3); timeMs.setVisible (i == 3); };
    addAndMakeVisible (feel);
    addAndMakeVisible (engine);
    setInterceptsMouseClicks (true, true);
    bindBand (proc.getSelectedBand());
    startTimerHz (30);
}

void Drawer::bindBand (int b)
{
    band = jlimit (0, Params::numBands - 1, b);
    // Release the previous band's attachments first. Otherwise, while the controls jump to the
    // new band's values, the old attachments would write those values into the old band.
    aTime.reset(); aMs.reset(); aFb.reset(); aAmt.reset(); aDuck.reset(); aDrive.reset();
    aSize.reset(); aDiff.reset(); aMix.reset(); aMode.reset(); aDtype.reset(); onAttach.reset();
    using SA = AudioProcessorValueTreeState::SliderAttachment;
    using CA = AudioProcessorValueTreeState::ComboBoxAttachment;
    auto& st = proc.apvts;
    aTime = std::make_unique<SA> (st, Params::id (band, Params::sync), time.slider);
    aMs = std::make_unique<SA> (st, Params::id (band, Params::ms), timeMs.slider);
    aFb = std::make_unique<SA> (st, Params::id (band, Params::fb), feedback.slider);
    aAmt = std::make_unique<SA> (st, Params::id (band, Params::amt), modeKnob.slider);
    aDuck = std::make_unique<SA> (st, Params::id (band, Params::duck), duck.slider);
    aDrive = std::make_unique<SA> (st, Params::id (band, Params::drive), drive.slider);
    aSize = std::make_unique<SA> (st, Params::id (band, Params::size), size.slider);
    aDiff = std::make_unique<SA> (st, Params::id (band, Params::diff), diffuse.slider);
    aMix = std::make_unique<SA> (st, Params::id (band, Params::mix), mix.slider);
    aMode = std::make_unique<CA> (st, Params::id (band, Params::mode), modeBox);
    aDtype = std::make_unique<CA> (st, Params::id (band, Params::dtype), driveBox);
    feel.bind (st.getParameter (Params::id (band, Params::feel)));
    engine.bind (st.getParameter (Params::id (band, Params::engine)));
    if (auto* p = st.getParameter (Params::id (band, Params::on)))
    {
        onAttach = std::make_unique<ParameterAttachment> (*p, [this] (float v) { bandEnabled = v > 0.5f; updateColours(); repaint(); });
        onAttach->sendInitialUpdate();
    }
    const bool ms = feel.index == 3;
    time.setVisible (! ms);
    timeMs.setVisible (ms);
    updateModeLabel();
    updateColours();
    repaint();
}

void Drawer::updateModeLabel()
{
    const int m = jlimit (0, 3, modeBox.getSelectedItemIndex());
    const auto name = Params::modeKnob()[m];
    modeKnob.valueText = [this, name] { return name + " " + String (roundToInt (modeKnob.slider.getValue())) + " %"; };
    static const char* keys[] = { "darken", "swing", "width", "scatter" };
    modeKnob.slider.setTooltip (helpOn ? Help::tip (keys[m], lang) : String());
    modeKnob.repaint();
}

void Drawer::setHelp (bool on, Help::Lang l)
{
    helpOn = on;
    lang = l;
    auto tip = [this] (const char* k) { return helpOn ? Help::tip (k, lang) : String(); };
    engine.setTooltip (tip ("engine"));
    time.slider.setTooltip (tip ("time"));
    timeMs.slider.setTooltip (tip ("timeMs"));
    feel.setTooltip (tip ("feel"));
    feedback.slider.setTooltip (tip ("feedback"));
    modeBox.setTooltip (tip ("mode"));
    duck.slider.setTooltip (tip ("duck"));
    drive.slider.setTooltip (tip ("drive"));
    driveBox.setTooltip (tip ("driveType"));
    size.slider.setTooltip (tip ("size"));
    diffuse.slider.setTooltip (tip ("diffuse"));
    mix.slider.setTooltip (tip ("mix"));
    updateModeLabel();
}

Rectangle<int> Drawer::tourRect (const String& name) const
{
    auto u = [] (const Component& a, const Component& b) { return a.getBounds().getUnion (b.getBounds()); };
    if (name == "engine") return engine.getBounds();
    if (name == "time") return u (time, feel).getUnion (feedback.getBounds());
    if (name == "mode") return u (modeBox, modeKnob);
    if (name == "duck") return duck.getBounds();
    if (name == "drive") return u (driveBox, drive);
    if (name == "space") return u (size, diffuse);
    if (name == "mix") return mix.getBounds();
    return {};
}

void Drawer::updateColours()
{
    const auto c = bandColour (themeOf (*this), band);
    for (auto* k : { &time, &timeMs, &feedback, &modeKnob, &duck, &drive, &size, &diffuse, &mix })
    {
        k->arcColour = c;
        k->slider.setColour (Slider::rotarySliderFillColourId, c);
        k->repaint();
    }
    feel.colour = c;
    engine.colour = c;
    modeBox.setColour (ComboBox::arrowColourId, c);
    driveBox.setColour (ComboBox::arrowColourId, c);
    for (auto* comp : std::initializer_list<Component*> { &time, &timeMs, &feedback, &modeKnob, &duck, &drive, &size, &diffuse, &mix, &feel, &modeBox, &driveBox })
        comp->setAlpha (bandEnabled ? 1.0f : 0.35f);
    repaint();
}

Path Drawer::shape() const
{
    const auto r = getLocalBounds().toFloat();
    Path p;
    const int steps = 120;
    p.startNewSubPath (r.getX(), r.getBottom() + 2);
    for (int i = 0; i <= steps; ++i)
    {
        const float u = (float) i / (float) steps;
        const float d = (u - 0.5f) * 2.0f;
        const float sh = 1.0f / (1.0f + std::pow (std::abs (d) / 0.90f, 14.0f));
        p.lineTo (r.getX() + r.getWidth() * u, r.getBottom() - sh * (float) fullHeight);
    }
    p.lineTo (r.getRight(), r.getBottom() + 2);
    p.closeSubPath();
    return p;
}

bool Drawer::hitTest (int x, int y) { return shape().contains ((float) x, (float) y); }

Rectangle<float> Drawer::chevronRect() const
{
    const float cx = (float) getWidth() * 0.5f, top = (float) (getHeight() - fullHeight);
    return { cx + 170, top + 10, 32, 28 };
}

Rectangle<float> Drawer::powerRect() const
{
    const float cx = (float) getWidth() * 0.5f, top = (float) (getHeight() - fullHeight);
    return { cx - 154, top + 12, 24, 24 };
}

void Drawer::resized()
{
    const auto r = getLocalBounds().toFloat();
    const float top = r.getBottom() - (float) fullHeight;
    const float cx = r.getCentreX();
    engine.setBounds (Rectangle<float> (cx + 6, top + 10, 150, 28).toNearestInt());

    const float y0 = top + 56;
    const float ks = 64.0f;
    const float margin = r.getWidth() > 1300.0f ? 0.13f : 0.10f;
    const float left = r.getX() + r.getWidth() * margin, right = r.getRight() - r.getWidth() * margin;
    auto slot = [&] (int i) { return left + (right - left) * ((float) i + 0.5f) / 8.0f; };
    auto knobBox = [&] (Knob& k, int i, bool label)
    {
        const float h = (label ? 18.0f : 0.0f) + ks + 4.0f + 22.0f;
        k.setBounds (Rectangle<float> (slot (i) - 56, label ? y0 : y0 + 18, 112, h).toNearestInt());
    };
    knobBox (time, 0, true);
    knobBox (timeMs, 0, true);
    feel.setBounds (Rectangle<float> (slot (0) - 56, y0 + 112, 112, 22).toNearestInt());
    knobBox (feedback, 1, true);
    const float slotW = (right - left) / 8.0f;
    const float boxW = jmin (124.0f, slotW - 8.0f);
    modeBox.setBounds (Rectangle<float> (slot (2) - boxW * 0.5f, y0 - 3, boxW, 23).toNearestInt());
    knobBox (modeKnob, 2, false);
    knobBox (duck, 3, true);
    driveBox.setBounds (Rectangle<float> (slot (4) - boxW * 0.5f, y0 - 3, boxW, 23).toNearestInt());
    knobBox (drive, 4, false);
    knobBox (size, 5, true);
    knobBox (diffuse, 6, true);
    knobBox (mix, 7, true);
}

void Drawer::paint (Graphics& g)
{
    const auto c = bandColour (themeOf (*this), band);
    const auto path = shape();
    DropShadow (Colours::black.withAlpha (0.7f), 28, { 0, -4 }).drawForPath (g, path);
    const auto r = getLocalBounds().toFloat();
    const float top = r.getBottom() - (float) fullHeight;
    ColourGradient fill (Colour (0xf0141417), 0, top, Colour (0xfa0b0b0d), 0, r.getBottom(), false);
    g.setGradientFill (fill);
    g.fillPath (path);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (path);
        ColourGradient hi (Colours::white.withAlpha (0.07f), 0, top, Colours::white.withAlpha (0.0f), 0, top + 70, false);
        g.setGradientFill (hi);
        g.fillRect (r);
    }
    Path topLine;
    {
        const int steps = 120;
        for (int i = 0; i <= steps; ++i)
        {
            const float u = (float) i / (float) steps;
            const float d = (u - 0.5f) * 2.0f;
            const float sh = 1.0f / (1.0f + std::pow (std::abs (d) / 0.90f, 14.0f));
            const Point<float> p (r.getX() + r.getWidth() * u, r.getBottom() - sh * (float) fullHeight);
            if (i == 0) topLine.startNewSubPath (p); else topLine.lineTo (p);
        }
    }
    g.setColour (c.withAlpha (0.25f));
    g.strokePath (topLine, PathStrokeType (6.0f));
    g.setColour (c.withAlpha (0.85f));
    g.strokePath (topLine, PathStrokeType (1.6f));

    // header
    const float cx = r.getCentreX();
    const auto pr = powerRect();
    {
        Path pw;
        pw.addCentredArc (pr.getCentreX(), pr.getCentreY() + 1, 7.0f, 7.0f, 0.0f, 0.6f, MathConstants<float>::twoPi - 0.6f, true);
        pw.startNewSubPath (pr.getCentreX(), pr.getCentreY() - 8);
        pw.lineTo (pr.getCentreX(), pr.getCentreY() - 1);
        g.setColour (bandEnabled ? c : kMute);
        g.strokePath (pw, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (c);
    g.fillEllipse (cx - 120, top + 18, 12, 12);
    g.setColour (kInk);
    g.setFont (uiFont (13.0f, true, 0.3f));
    g.drawText ("BAND " + String (band + 1) + (bandEnabled ? "" : "  OFF"), Rectangle<float> (cx - 100, top + 14, 150, 20), Justification::centredLeft);
    const auto ch = chevronRect();
    Path chev;
    if (open) { chev.startNewSubPath (ch.getX() + 8, ch.getY() + 10); chev.lineTo (ch.getCentreX(), ch.getY() + 18); chev.lineTo (ch.getRight() - 8, ch.getY() + 10); }
    else      { chev.startNewSubPath (ch.getX() + 8, ch.getY() + 18); chev.lineTo (ch.getCentreX(), ch.getY() + 10); chev.lineTo (ch.getRight() - 8, ch.getY() + 18); }
    g.setColour (kMute);
    g.strokePath (chev, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

void Drawer::mouseDown (const MouseEvent& e)
{
    if (chevronRect().expanded (6).contains (e.position))
    {
        setOpen (! open);
        return;
    }
    if (powerRect().expanded (4).contains (e.position) && onAttach)
    {
        onAttach->setValueAsCompleteGesture (bandEnabled ? 0.0f : 1.0f);
        return;
    }
    if (! open && e.position.y < (float) (getHeight() - fullHeight) + 46.0f)
        setOpen (true);
}

void Drawer::setOpen (bool o) { open = o; repaint(); }

float Drawer::visibleHeight() const { return (float) tabHeight + ((float) fullHeight - (float) tabHeight) * openness; }

void Drawer::timerCallback()
{
    const float target = open ? 1.0f : 0.0f;
    if (std::abs (openness - target) > 0.001f)
    {
        openness += (target - openness) * 0.3f;
        if (std::abs (openness - target) < 0.01f) openness = target;
        editor.drawerMoved();
    }
    if (duck.isVisible()) duck.repaint();
}

//==============================================================================
MeterColumn::MeterColumn (DellayriumProcessor& p, bool isOut) : proc (p), output (isOut)
{
    knob.label = output ? "Wet" : "Dry";
    knob.getProperties().set ("onDark", false);
    addAndMakeVisible (knob);
    attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, output ? Params::wet : Params::dry, knob.slider);
    knob.slider.setTooltip (output ? "Wet: level of all the echoes together. Put Dry at 0 and Wet at 100 to use Dellayrium on a send."
                                   : "Dry: level of the untouched signal.");
    startTimerHz (30);
}

void MeterColumn::resized()
{
    knob.setBounds (getLocalBounds().removeFromBottom (138).withTrimmedBottom (14));
    knob.arcColour = output ? themeOf (*this).accent : Colour (0xffececf0);
    knob.slider.setColour (Slider::rotarySliderFillColourId, knob.arcColour);
}

void MeterColumn::timerCallback()
{
    for (int c = 0; c < 2; ++c)
    {
        auto& src = output ? proc.outPeak[c] : proc.inPeak[c];
        const float v = src.exchange (0.0f);
        const float db = Decibels::gainToDecibels (v, -60.0f);
        const float norm = jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);
        level[c] = jmax (norm, level[c] - 0.025f);
        peakHold[c] = jmax (norm, peakHold[c] - 0.004f);
        if (v > 0.0f) peakDb = jmax (peakDb - 0.15f, db);
        else peakDb = jmax (-100.0f, peakDb - 0.15f);
    }
    repaint();
}

void MeterColumn::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto c = getLocalBounds().toFloat();
    g.setColour (t.panelFill);
    g.fillRoundedRectangle (c, 10.0f);
    g.setColour (t.panelStroke);
    g.drawRoundedRectangle (c.reduced (0.5f), 10.0f, 1.0f);
    g.setColour (t.ink);
    g.setFont (uiFont (13.0f, true, 0.35f));
    g.drawText (output ? "OUT" : "IN", Rectangle<float> (c.getX(), c.getY() + 14, c.getWidth(), 18), Justification::centred);
    auto pb = Rectangle<float> (c.getX() + 8, c.getY() + 40, c.getWidth() - 16, 24);
    g.setColour (Colour (0xff111114));
    g.fillRoundedRectangle (pb, 5.0f);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (pb.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (peakDb > -0.1f ? Colour (0xffff3c46) : kInk);
    g.setFont (uiFont (12.0f));
    g.drawText (peakDb < -59.0f ? String ("-inf") : String (peakDb, 1), pb, Justification::centred);

    const float mTop = c.getY() + 90, mBot = c.getBottom() - 150;
    for (int i = 0; i < 2; ++i)
    {
        auto bar = Rectangle<float> (c.getCentreX() - 11 + (float) i * 14, mTop, 8, mBot - mTop);
        g.setColour (Colour (0xff0e0e10));
        g.fillRoundedRectangle (bar, 3.0f);
        auto lit = bar.withTop (bar.getBottom() - bar.getHeight() * level[i]);
        ColourGradient mg (t.accent, 0, bar.getY(), t.accentDim, 0, bar.getBottom(), false);
        g.setGradientFill (mg);
        g.fillRoundedRectangle (lit, 3.0f);
        const float py = bar.getBottom() - bar.getHeight() * peakHold[i];
        g.setColour (t.accentHi);
        g.fillRect (bar.getX(), py - 1.0f, bar.getWidth(), 2.0f);
    }
    for (int i = 0; i < 6; ++i)
    {
        const float y = mTop + (mBot - mTop) * (float) i / 5.0f;
        g.setColour (t.ink.withAlpha (0.15f));
        g.drawHorizontalLine (roundToInt (y), c.getX() + 6, c.getX() + 14);
    }
}

//==============================================================================
PresetBar::PresetBar (DellayriumProcessor& p) : proc (p)
{
    startTimerHz (4);
    setMouseCursor (MouseCursor::PointingHandCursor);
}

Rectangle<float> PresetBar::prevR() const { return { 0, 0, 26, (float) getHeight() }; }
Rectangle<float> PresetBar::nameR() const { return { 34, 0, (float) getWidth() - 34 - 76, (float) getHeight() }; }
Rectangle<float> PresetBar::nextR() const { return { nameR().getRight() + 8, 0, 26, (float) getHeight() }; }
Rectangle<float> PresetBar::saveR() const { return { nextR().getRight() + 8, 2, (float) getHeight() - 4, (float) getHeight() - 4 }; }

void PresetBar::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto ink = t.lightBg ? t.ink : Colour (0xffbdb5ba);
    auto nr = nameR();
    g.setColour (kWidget);
    g.fillRoundedRectangle (nr, 7.0f);
    g.setColour (Colours::white.withAlpha (0.09f));
    g.drawRoundedRectangle (nr.reduced (0.5f), 7.0f, 1.0f);
    g.setColour (kInk);
    g.setFont (uiFont (15.0f));
    g.drawText (proc.presets.getCurrentName() + (proc.presets.isModified() ? " *" : ""), nr.withTrimmedLeft (16).withTrimmedRight (40), Justification::centredLeft, true);
    Path a; a.startNewSubPath (nr.getRight() - 30, nr.getCentreY() - 3); a.lineTo (nr.getRight() - 23, nr.getCentreY() + 4); a.lineTo (nr.getRight() - 16, nr.getCentreY() - 3);
    g.setColour (t.accent);
    g.strokePath (a, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
    auto arrow = [&] (Rectangle<float> r, bool left)
    {
        Path p;
        const float cx = r.getCentreX(), cy = r.getCentreY();
        p.startNewSubPath (cx + (left ? 4.0f : -4.0f), cy - 8);
        p.lineTo (cx + (left ? -4.0f : 4.0f), cy);
        p.lineTo (cx + (left ? 4.0f : -4.0f), cy + 8);
        g.setColour (ink);
        g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    };
    arrow (prevR(), true);
    arrow (nextR(), false);
    const auto sv = saveR().reduced (4);
    g.setColour (ink);
    g.drawRoundedRectangle (sv, 3.0f, 1.8f);
    g.drawRect (sv.reduced (5).withHeight (6), 1.6f);
    g.drawRect (Rectangle<float> (sv.getX() + 5, sv.getBottom() - 9, sv.getWidth() - 10, 7), 1.4f);
}

void PresetBar::mouseDown (const MouseEvent& e)
{
    if (prevR().contains (e.position)) proc.presets.step (-1);
    else if (nextR().contains (e.position)) proc.presets.step (1);
    else if (saveR().contains (e.position))
    {
        const auto cur = proc.presets.getCurrentName();
        askForName ("Save preset", proc.presets.isFactory (cur) || cur == "Init" ? String() : cur,
                    [this] (const String& n) { proc.presets.save (n); repaint(); });
    }
    else showMenu();
    repaint();
}

void PresetBar::showMenu()
{
    PopupMenu menu;
    const auto names = proc.presets.getPresetNames();
    const auto factory = PresetManager::getFactoryNames();
    const auto current = proc.presets.getCurrentName();
    menu.addItem (1, "Init", true, current == "Init");
    menu.addSectionHeader ("Factory presets");
    for (int i = 0; i < factory.size(); ++i)
        menu.addItem (500 + i, factory[i], true, factory[i] == current);
    menu.addSectionHeader ("User presets");
    if (names.isEmpty())
        menu.addItem (2, "(none yet - press the disk icon to save)", false);
    for (int i = 0; i < names.size(); ++i)
        menu.addItem (100 + i, names[i], true, names[i] == current);
    menu.addSeparator();
    const bool isUser = names.contains (current);
    menu.addItem (3, "Rename current...", isUser);
    menu.addItem (4, "Delete current...", isUser);
    menu.addItem (5, "Open presets folder");
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMinimumWidth ((int) nameR().getWidth()),
        [this, names, current, factory] (int r)
        {
            if (r == 1) proc.presets.loadInit();
            else if (r >= 500 && r - 500 < factory.size()) proc.presets.loadFactory (r - 500);
            else if (r >= 100 && r - 100 < names.size()) proc.presets.load (names[r - 100]);
            else if (r == 3) askForName ("Rename preset", current, [this, current] (const String& n) { proc.presets.rename (current, n); repaint(); });
            else if (r == 4)
            {
                auto opts = MessageBoxOptions().withIconType (MessageBoxIconType::WarningIcon)
                                .withTitle ("Delete preset?").withMessage ("Delete \"" + current + "\"? This cannot be undone.")
                                .withButton ("Delete").withButton ("Cancel").withAssociatedComponent (this);
                AlertWindow::showAsync (opts, [this, current] (int result) { if (result == 1) proc.presets.remove (current); repaint(); });
            }
            else if (r == 5) PresetManager::getFolder().startAsProcess();
            repaint();
        });
}

void PresetBar::askForName (const String& title, const String& initial, std::function<void (const String&)> onOk)
{
    auto* w = new AlertWindow (title, "Preset name:", MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", initial);
    w->addButton ("OK", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([w, onOk] (int result)
    {
        const auto text = w->getTextEditorContents ("name").trim();
        if (result == 1 && text.isNotEmpty())
            onOk (text);
    }), true);
}

//==============================================================================
TourOverlay::TourOverlay()
{
    for (auto* b : { &back, &next, &skip })
    {
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (*b);
    }
    back.onClick = [this] { showStep (step - 1); };
    next.onClick = [this]
    {
        if (step + 1 >= Help::tourLength)
        {
            setVisible (false);
            if (onFinish) onFinish();
        }
        else
        {
            showStep (step + 1);
        }
    };
    skip.onClick = [this] { setVisible (false); if (onFinish) onFinish(); };
}

void TourOverlay::start (Help::Lang l)
{
    lang = l;
    setVisible (true);
    toFront (false);
    showStep (0);
}

void TourOverlay::showStep (int s)
{
    step = jlimit (0, Help::tourLength - 1, s);
    const auto& t = themeOf (*this);
    back.setButtonText (Help::ui ("Back", "Назад", lang));
    next.setButtonText (step + 1 >= Help::tourLength ? Help::ui ("Done", "Готово", lang) : Help::ui ("Next", "Дальше", lang));
    skip.setButtonText (Help::ui ("Skip", "Пропустить", lang));
    next.setColour (TextButton::buttonColourId, t.accent);
    next.setColour (TextButton::textColourOffId, Colour (0xff111114));
    back.setVisible (step > 0);
    skip.setVisible (step + 1 < Help::tourLength);
    if (onStep)
        onStep (Help::tour[step].target);
    resized();
    repaint();
}

Rectangle<float> TourOverlay::calloutArea() const
{
    const auto& st = Help::tour[step];
    const auto target = resolveTarget ? resolveTarget (st.target) : Rectangle<float>();
    const float w = 380.0f;
    AttributedString s;
    s.append (Help::ui (st.textEn, st.textRu, lang), uiFont (14.0f), Colours::white);
    TextLayout tl;
    tl.createLayout (s, w - 36.0f);
    const float h = 54.0f + tl.getHeight() + 70.0f;
    const auto bounds = getLocalBounds().toFloat().reduced (16.0f);
    if (target.isEmpty())
        return bounds.withSizeKeepingCentre (w, h);
    float x = jlimit (bounds.getX(), bounds.getRight() - w, target.getCentreX() - w * 0.5f);
    float y = target.getBottom() + 22.0f;
    if (y + h > bounds.getBottom())
        y = target.getY() - 22.0f - h;
    if (y < bounds.getY())
    {
        // Not enough room above or below: put it to the side
        y = jlimit (bounds.getY(), bounds.getBottom() - h, target.getCentreY() - h * 0.5f);
        x = target.getRight() + 22.0f + w < bounds.getRight() ? target.getRight() + 22.0f : target.getX() - 22.0f - w;
    }
    return { x, y, w, h };
}

void TourOverlay::resized()
{
    const auto c = calloutArea().toNearestInt();
    next.setBounds (c.getRight() - 16 - 96, c.getBottom() - 16 - 30, 96, 30);
    back.setBounds (next.getX() - 8 - 86, next.getY(), 86, 30);
    skip.setBounds (c.getX() + 14, next.getY(), 104, 30);
}

void TourOverlay::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    const auto& st = Help::tour[step];
    const auto target = resolveTarget ? resolveTarget (st.target) : Rectangle<float>();

    Path dim;
    dim.addRectangle (getLocalBounds().toFloat());
    if (! target.isEmpty())
    {
        dim.addRoundedRectangle (target.expanded (6.0f), 12.0f);
        dim.setUsingNonZeroWinding (false);
    }
    g.setColour (Colours::black.withAlpha (0.68f));
    g.fillPath (dim);

    const auto c = calloutArea();
    if (! target.isEmpty())
    {
        g.setColour (t.accent.withAlpha (0.35f));
        g.drawRoundedRectangle (target.expanded (9.0f), 14.0f, 5.0f);
        g.setColour (t.accent);
        g.drawRoundedRectangle (target.expanded (6.0f), 12.0f, 2.0f);

        // Arrow from the card to the highlighted part
        Point<float> from, to;
        if (c.getBottom() <= target.getY())       { from = { c.getCentreX(), c.getBottom() }; to = { jlimit (target.getX() + 10.0f, target.getRight() - 10.0f, c.getCentreX()), target.getY() - 8.0f }; }
        else if (c.getY() >= target.getBottom())  { from = { c.getCentreX(), c.getY() };      to = { jlimit (target.getX() + 10.0f, target.getRight() - 10.0f, c.getCentreX()), target.getBottom() + 8.0f }; }
        else if (c.getX() >= target.getRight())   { from = { c.getX(), c.getCentreY() };      to = { target.getRight() + 8.0f, jlimit (target.getY() + 10.0f, target.getBottom() - 10.0f, c.getCentreY()) }; }
        else                                      { from = { c.getRight(), c.getCentreY() };  to = { target.getX() - 8.0f, jlimit (target.getY() + 10.0f, target.getBottom() - 10.0f, c.getCentreY()) }; }
        Path arrow;
        arrow.addArrow (Line<float> (from, to), 2.5f, 12.0f, 10.0f);
        g.setColour (t.accent);
        g.fillPath (arrow);
    }

    g.setColour (Colour (0xff141417));
    g.fillRoundedRectangle (c, 12.0f);
    g.setColour (t.accent.withAlpha (0.7f));
    g.drawRoundedRectangle (c.reduced (0.5f), 12.0f, 1.2f);

    g.setColour (t.accentHi);
    g.setFont (uiFont (17.0f, true));
    g.drawText (Help::ui (st.titleEn, st.titleRu, lang), c.reduced (18.0f, 0.0f).withY (c.getY() + 14.0f).withHeight (24.0f), Justification::centredLeft);
    g.setColour (Colour (0xff8a8a94));
    g.setFont (uiFont (12.0f));
    g.drawText (String (step + 1) + " / " + String (Help::tourLength), c.reduced (18.0f, 0.0f).withY (c.getY() + 14.0f).withHeight (24.0f), Justification::centredRight);

    AttributedString s;
    s.append (Help::ui (st.textEn, st.textRu, lang), uiFont (14.0f), Colour (0xffe2e2e8));
    s.setLineSpacing (3.0f);
    TextLayout tl;
    tl.createLayout (s, c.getWidth() - 36.0f);
    tl.draw (g, Rectangle<float> (c.getX() + 18.0f, c.getY() + 46.0f, c.getWidth() - 36.0f, tl.getHeight()));
}

//==============================================================================
WelcomePrompt::WelcomePrompt()
{
    for (auto* b : { &yes, &notNow, &never, &en, &ru })
    {
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (*b);
    }
    ru.setButtonText (String::fromUTF8 ("Русский"));
    yes.onClick = [this] { if (onChoice) onChoice (0); };
    notNow.onClick = [this] { if (onChoice) onChoice (1); };
    never.onClick = [this] { if (onChoice) onChoice (2); };
    en.onClick = [this] { setLang (Help::Lang::en); if (onLanguage) onLanguage (lang); };
    ru.onClick = [this] { setLang (Help::Lang::ru); if (onLanguage) onLanguage (lang); };
    setLang (Help::Lang::en);
}

void WelcomePrompt::setLang (Help::Lang l)
{
    lang = l;
    const auto& t = themeOf (*this);
    yes.setButtonText (Help::ui ("Yes, show me", "Да, покажи", lang));
    notNow.setButtonText (Help::ui ("Not now", "Не сейчас", lang));
    never.setButtonText (Help::ui ("Don't ask again", "Больше не спрашивать", lang));
    yes.setColour (TextButton::buttonColourId, t.accent);
    yes.setColour (TextButton::textColourOffId, Colour (0xff111114));
    for (auto* b : { &en, &ru })
    {
        const bool sel = (b == &ru) == (lang == Help::Lang::ru);
        b->setColour (TextButton::buttonColourId, sel ? t.accent : Colour (0xff26262b));
        b->setColour (TextButton::textColourOffId, sel ? Colour (0xff111114) : Colour (0xffececf0));
    }
    repaint();
}

Rectangle<float> WelcomePrompt::card() const
{
    return getLocalBounds().toFloat().withSizeKeepingCentre (500.0f, 270.0f);
}

void WelcomePrompt::resized()
{
    const auto c = card().toNearestInt();
    en.setBounds (c.getX() + 170, c.getY() + 128, 100, 28);
    ru.setBounds (en.getRight() + 8, en.getY(), 100, 28);
    yes.setBounds (c.getRight() - 24 - 150, c.getBottom() - 24 - 34, 150, 34);
    notNow.setBounds (yes.getX() - 10 - 120, yes.getY(), 120, 34);
    never.setBounds (c.getX() + 24, yes.getY(), 170, 34);
}

void WelcomePrompt::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    g.fillAll (Colours::black.withAlpha (0.62f));
    const auto c = card();
    g.setColour (Colour (0xff141417));
    g.fillRoundedRectangle (c, 14.0f);
    g.setColour (t.accent.withAlpha (0.7f));
    g.drawRoundedRectangle (c.reduced (0.5f), 14.0f, 1.2f);
    Logo::draw (g, Rectangle<float> (c.getX() + 24.0f, c.getY() + 22.0f, 54.0f, 56.0f), FillType (t.lightBg ? t.accent : t.logo));
    g.setColour (Colour (0xffececf0));
    g.setFont (uiFont (19.0f, true));
    g.drawText (Help::ui ("Hi! Want a quick tour of Dellayrium?", "Привет! Показать, как работает Dellayrium?", lang),
                c.withTrimmedLeft (94.0f).withTrimmedRight (20.0f).withY (c.getY() + 28.0f).withHeight (26.0f), Justification::centredLeft);
    g.setColour (Colour (0xffa5a5ad));
    g.setFont (uiFont (13.0f));
    g.drawText (Help::ui ("It takes a minute and shows what every knob does.", "Займёт пару минут: покажу, что делает каждая ручка.", lang),
                c.withTrimmedLeft (94.0f).withTrimmedRight (20.0f).withY (c.getY() + 56.0f).withHeight (20.0f), Justification::centredLeft);
    g.setColour (Colour (0xffececf0));
    g.setFont (uiFont (13.0f, true));
    g.drawText (Help::ui ("Help language", "Язык подсказок", lang), Rectangle<float> (c.getX() + 24.0f, c.getY() + 128.0f, 140.0f, 28.0f), Justification::centredLeft);
}
