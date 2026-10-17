#include "PluginEditor.h"
#include "Logo.h"

using namespace juce;

namespace
{
    bool askedThisRun = false;
    PropertiesFile::Options settingsOptions()
    {
        PropertiesFile::Options o;
        o.applicationName = "Dellayrium";
        o.folderName = "Primal Rage Lab";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        o.millisecondsBeforeSaving = 0;
        return o;
    }
}

String DellayriumEditor::loadSetting (const String& key, const String& fallback)
{
    PropertiesFile f (settingsOptions());
    return f.getValue (key, fallback);
}

void DellayriumEditor::saveSetting (const String& key, const String& value)
{
    PropertiesFile f (settingsOptions());
    f.setValue (key, value);
    f.saveIfNeeded();
}

DellayriumEditor::DellayriumEditor (DellayriumProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    // theme, help and language are remembered for every new instance
    const auto themeName = loadSetting ("theme", proc.getTheme());
    proc.setTheme (themeName);
    look.setTheme (Theme::byName (themeName));
    helpOn = loadSetting ("help", "on") != "off";
    lang = loadSetting ("helpLang", "en") == "ru" ? Help::Lang::ru : Help::Lang::en;

    addAndMakeVisible (inCol);
    addAndMakeVisible (outCol);
    addAndMakeVisible (presetBar);
    addAndMakeVisible (screen);
    addAndMakeVisible (drawer);
    addChildComponent (tour);
    addChildComponent (welcome);
    setLookAndFeel (&look);

    tour.resolveTarget = [this] (const String& n) { return tourTarget (n); };
    tour.onStep = [this] (const String& n)
    {
        if (n == "drawer" || n == "engine" || n == "time" || n == "mode" || n == "duck" || n == "drive" || n == "space" || n == "mix")
            drawer.setOpen (true);
    };
    tour.onFinish = [] { saveSetting ("tour", "never"); };
    welcome.onLanguage = [this] (Help::Lang l)
    {
        lang = l;
        saveSetting ("helpLang", l == Help::Lang::ru ? "ru" : "en");
        applyHelp();
    };
    welcome.onChoice = [this] (int choice)
    {
        welcome.setVisible (false);
        if (choice == 0) tour.start (lang);
        else if (choice == 2) saveSetting ("tour", "never");
    };
    welcome.setLang (lang);
    applyHelp();

    // size: the project remembers its own, otherwise the last one used anywhere.
    // Read it before setResizeLimits, which resizes the editor (and would overwrite the saved size).
    int w = (int) proc.apvts.state.getProperty ("uiW", loadSetting ("w", String (baseW)).getIntValue());
    int h = (int) proc.apvts.state.getProperty ("uiH", loadSetting ("h", String (baseH)).getIntValue());
    setResizable (true, true);
    setResizeLimits (1100, 700, 3400, 2000);
    ready = true;
    setSize (jlimit (1100, 3400, w), jlimit (700, 2000, h));

    if (loadSetting ("tour", "") != "never" && ! askedThisRun)
    {
        askedThisRun = true;
        welcome.setVisible (true);
        welcome.toFront (false);
    }
}

void DellayriumEditor::applyHelp()
{
    if (helpOn && tooltipWindow == nullptr)
        tooltipWindow = std::make_unique<TooltipWindow> (this, 700);
    else if (! helpOn)
        tooltipWindow.reset();
    auto tip = [this] (const char* k) { return helpOn ? Help::tip (k, lang) : String(); };
    inCol.knob.slider.setTooltip (tip ("dry"));
    outCol.knob.slider.setTooltip (tip ("wet"));
    inCol.setTooltip (tip ("inMeter"));
    outCol.setTooltip (tip ("outMeter"));
    presetBar.setTooltip (tip ("presets"));
    screen.setTooltip (tip ("screen"));
    drawer.setHelp (helpOn, lang);
}

Rectangle<float> DellayriumEditor::tourTarget (const String& n)
{
    if (n.isEmpty()) return {};
    if (n == "inCol") return inCol.getBounds().toFloat();
    if (n == "outCol") return outCol.getBounds().toFloat();
    if (n == "presets") return presetBar.getBounds().toFloat();
    if (n == "gear") return gearArea.toFloat();
    if (n == "drawer") return drawer.getBounds().toFloat().withTrimmedTop ((float) drawer.getHeight() - drawer.visibleHeight()).withTrimmedBottom (8.0f);
    const auto sr = screen.tourRect (n);
    if (! sr.isEmpty()) return sr + screen.getPosition().toFloat();
    const auto dr = drawer.tourRect (n);
    if (! dr.isEmpty()) return (dr + drawer.getPosition()).toFloat();
    return {};
}

DellayriumEditor::~DellayriumEditor() { setLookAndFeel (nullptr); }

void DellayriumEditor::selectBand (int band, bool openDrawer)
{
    proc.setSelectedBand (band);
    drawer.bindBand (band);
    if (openDrawer) drawer.setOpen (true);
    if (proc.soloBand.load() >= 0 && proc.soloBand.load() != band)
        proc.soloBand = -1;
    screen.repaint();
}

void DellayriumEditor::applyTheme (const String& name)
{
    proc.setTheme (name);
    saveSetting ("theme", name);
    look.setTheme (Theme::byName (name));
    background = {};
    sendLookAndFeelChange();
    resized();
    drawer.bindBand (proc.getSelectedBand());
    repaint();
}

void DellayriumEditor::showToast (const String& t)
{
    toast = t;
    toastUntil = Time::getMillisecondCounter() + 3500;
    repaint();
    Timer::callAfterDelay (3600, [safe = SafePointer<DellayriumEditor> (this)] { if (safe) safe->repaint(); });
}

void DellayriumEditor::buildBackground()
{
    const auto& t = look.theme();
    const int W = getWidth(), H = getHeight();
    background = Image (Image::RGB, W, H, false);
    Graphics g (background);
    if (t.gloss)
    {
        ColourGradient bg (Colour (0xff1c1c21), 0.0f, 0.0f, Colour (0xff141418), 0.0f, (float) H, false);
        bg.addColour (0.45, Colour (0xff0b0b0d));
        g.setGradientFill (bg);
        g.fillAll();
        struct Sheen { float x, w, a; };
        for (const auto& sh : { Sheen { 0.18f, 0.22f, 0.07f }, Sheen { 0.62f, 0.08f, 0.05f }, Sheen { 0.80f, 0.30f, 0.04f } })
        {
            const float x0 = (float) W * sh.x, w0 = (float) W * sh.w;
            ColourGradient sg (Colours::white.withAlpha (0.0f), x0 - w0, 0.0f, Colours::white.withAlpha (0.0f), x0 + w0, (float) H * 0.3f, false);
            sg.addColour (0.5, Colour ((uint8) 255, 248, 230, sh.a));
            g.setGradientFill (sg);
            Path p;
            p.startNewSubPath (x0 - w0, 0.0f); p.lineTo (x0 + w0, 0.0f);
            p.lineTo (x0 + w0 - (float) W * 0.25f, (float) H); p.lineTo (x0 - w0 - (float) W * 0.25f, (float) H);
            p.closeSubPath();
            g.fillPath (p);
        }
        return;
    }
    // wet asphalt: coarse blotches, fine grain and a few bright specks
    Random rnd (1234);
    constexpr int gw = 30, gh = 22;
    float coarse[gw * gh];
    for (auto& v : coarse) v = rnd.nextFloat() * 14.0f - 7.0f;
    {
        Image::BitmapData bd (background, Image::BitmapData::writeOnly);
        const float br = t.bg.getFloatRed() * 255.0f, bgc = t.bg.getFloatGreen() * 255.0f, bb = t.bg.getFloatBlue() * 255.0f;
        auto ch = [] (float f) { return (uint8) jlimit (0, 255, roundToInt (f)); };
        for (int y = 0; y < H; ++y)
        {
            const float fy = (float) y / (float) (H - 1) * (gh - 1);
            const int y0 = jmin (gh - 2, (int) fy);
            const float ty = fy - (float) y0;
            for (int x = 0; x < W; ++x)
            {
                const float fx = (float) x / (float) (W - 1) * (gw - 1);
                const int x0 = jmin (gw - 2, (int) fx);
                const float tx = fx - (float) x0;
                const float c = coarse[y0 * gw + x0] * (1 - tx) * (1 - ty) + coarse[y0 * gw + x0 + 1] * tx * (1 - ty)
                              + coarse[(y0 + 1) * gw + x0] * (1 - tx) * ty + coarse[(y0 + 1) * gw + x0 + 1] * tx * ty;
                const float nz = c + (rnd.nextFloat() - 0.5f) * 30.0f;
                if (rnd.nextFloat() < 0.004f) bd.setPixelColour (x, y, t.bg.interpolatedWith (t.speck, 0.4f + rnd.nextFloat() * 0.6f));
                else bd.setPixelColour (x, y, Colour (ch (br + nz), ch (bgc + nz), ch (bb + nz * 1.1f)));
            }
        }
    }
    ColourGradient sheen (Colour (0xffb4c4dc).withAlpha (t.lightBg ? 0.10f : 0.06f), (float) W * 0.7f, (float) H * 0.12f,
                          Colours::transparentWhite, (float) W * 1.05f, (float) H * 0.12f, true);
    g.setGradientFill (sheen);
    g.fillEllipse ((float) W * 0.35f, 0.0f, (float) W * 0.7f, (float) H * 0.3f);
    ColourGradient vig (Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.45f, t.vignette, 0.0f, 0.0f, true);
    g.setGradientFill (vig);
    g.fillRect (0, 0, W, H);
}

void DellayriumEditor::paint (Graphics& g)
{
    const auto& t = look.theme();
    if (! background.isValid() || background.getWidth() != getWidth() || background.getHeight() != getHeight())
        buildBackground();
    g.drawImageAt (background, 0, 0);

    const auto la = logoArea.toFloat();
    if (t.gloss)
    {
        ColourGradient gold (Colour (0xfff6dc9a), la.getX(), la.getY(), Colour (0xffa47a2c), la.getX(), la.getBottom(), false);
        gold.addColour (0.55, Colour (0xffe8c46a));
        Logo::draw (g, la, FillType (gold));
    }
    else
        Logo::draw (g, la, FillType (t.logo));
    g.setColour (t.lightBg ? t.ink : Colours::white);
    g.setFont (uiFont (36.0f, true, 0.08f));
    g.drawText ("DELLAYRIUM", logoArea.getRight() + 10, 14, 420, 40, Justification::centredLeft);
    g.setColour (t.mute);
    g.setFont (uiFont (12.0f, true, 0.45f));
    g.drawText ("PRIMAL RAGE LAB", logoArea.getRight() + 13, 54, 320, 16, Justification::centredLeft);

    // gear
    const auto gr = gearArea.toFloat();
    g.setColour (kWidget);
    g.fillRoundedRectangle (gr, 7.0f);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (gr.reduced (0.5f), 7.0f, 1.0f);
    g.setColour (kInk);
    g.drawEllipse (Rectangle<float> (12, 12).withCentre (gr.getCentre()), 2.0f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = (float) i * MathConstants<float>::pi / 4.0f;
        const auto c = gr.getCentre();
        g.drawLine (c.x + std::sin (a) * 8, c.y - std::cos (a) * 8, c.x + std::sin (a) * 11, c.y - std::cos (a) * 11, 2.0f);
    }
}

void DellayriumEditor::paintOverChildren (Graphics& g)
{
    if (toast.isEmpty() || Time::getMillisecondCounter() > toastUntil)
        return;
    const auto f = uiFont (13.0f);
    const int w = GlyphArrangement::getStringWidthInt (f, toast) + 40;
    const auto r = Rectangle<int> (w, 40).withCentre ({ getWidth() / 2, 110 }).toFloat();
    g.setColour (Colour (0xf2141417));
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (look.theme().accent);
    g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.2f);
    g.setColour (kInk);
    g.setFont (f);
    g.drawText (toast, r, Justification::centred);
}

void DellayriumEditor::resized()
{
    const int W = getWidth(), H = getHeight();
    if (ready)
    {
        proc.apvts.state.setProperty ("uiW", W, nullptr);
        proc.apvts.state.setProperty ("uiH", H, nullptr);
        saveSetting ("w", String (W));
        saveSetting ("h", String (H));
    }
    inCol.setBounds (10, 10, 84, H - 20);
    outCol.setBounds (W - 94, 10, 84, H - 20);
    logoArea = { 104, 10, 62, 66 };
    gearArea = { W - 94 - 12 - 46, 20, 46, 38 };
    // the preset bar sits between the title and the gear, centred on the window when there is room
    const int left = logoArea.getRight() + 270, right = gearArea.getX() - 16;
    const int pw = jmin (380, right - left);
    const int px = jlimit (left, right - pw, W / 2 - pw / 2);
    presetBar.setBounds (px, 22, pw, 34);
    screen.setBounds (106, 84, W - 212, H - 96);
    tour.setBounds (getLocalBounds());
    welcome.setBounds (getLocalBounds());
    drawerMoved();
    if (tour.isVisible()) tour.resized();
}

void DellayriumEditor::drawerMoved()
{
    const int W = getWidth(), H = getHeight();
    const float vis = drawer.visibleHeight();
    drawer.setBounds (104, H - (int) vis - 8, W - 208, Drawer::fullHeight + 8);
    screen.bottomReserve = vis - 4.0f;
    screen.repaint();
}

void DellayriumEditor::mouseDown (const MouseEvent& e)
{
    if (gearArea.contains (e.getPosition()))
        showGearMenu();
}

void DellayriumEditor::showGearMenu()
{
    PopupMenu themes, help, langs, m;
    for (const auto& t : Theme::all())
        themes.addItem (t.name, true, t.name == look.theme().name, [this, n = t.name] { applyTheme (n); });
    help.addItem ("On", true, helpOn, [this] { helpOn = true; saveSetting ("help", "on"); applyHelp(); });
    help.addItem ("Off", true, ! helpOn, [this] { helpOn = false; saveSetting ("help", "off"); applyHelp(); });
    langs.addItem ("English", true, lang == Help::Lang::en, [this] { lang = Help::Lang::en; saveSetting ("helpLang", "en"); welcome.setLang (lang); applyHelp(); });
    langs.addItem (String::fromUTF8 ("Русский"), true, lang == Help::Lang::ru, [this] { lang = Help::Lang::ru; saveSetting ("helpLang", "ru"); welcome.setLang (lang); applyHelp(); });
    m.addSubMenu ("Theme", themes);
    m.addSubMenu ("Help", help);
    m.addSubMenu ("Help language", langs);
    m.addSeparator();
    m.addItem ("Start tour", [this] { tour.start (lang); });
    m.addItem ("Reset window size", [this] { setSize (baseW, baseH); });
    m.addSeparator();
    m.addItem (String ("Dellayrium ") + JucePlugin_VersionString + " by Primal Rage Lab", false, false, nullptr);
    m.showMenuAsync (PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (gearArea)));
}
