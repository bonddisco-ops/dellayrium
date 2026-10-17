// Offline test for Dellayrium: band selectivity, timing, ping-pong, FFT engine, stability, CPU,
// and screenshots of the editor in every theme.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr double SR = 48000.0;
constexpr int BS = 512;

void setP (DellayriumProcessor& p, const String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    jassert (prm != nullptr);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

void soloBandSetup (DellayriumProcessor& p, int band, float freq, int engine, int mode, float ms, float fb)
{
    for (int b = 0; b < Params::numBands; ++b) setP (p, Params::id (b, Params::on), 0);
    setP (p, Params::id (band, Params::on), 1);
    setP (p, Params::id (band, Params::freq), freq);
    setP (p, Params::id (band, Params::level), 1.0f);
    setP (p, Params::id (band, Params::q), 2.0f);
    setP (p, Params::id (band, Params::engine), (float) engine);
    setP (p, Params::id (band, Params::mode), (float) mode);
    setP (p, Params::id (band, Params::feel), 3);
    setP (p, Params::id (band, Params::ms), ms);
    setP (p, Params::id (band, Params::fb), fb);
    setP (p, Params::id (band, Params::amt), 0);
    setP (p, Params::dry, 0);
    setP (p, Params::wet, 100);
}

AudioBuffer<float> run (DellayriumProcessor& p, const AudioBuffer<float>& in)
{
    AudioBuffer<float> out (in);
    MidiBuffer midi;
    for (int pos = 0; pos < out.getNumSamples(); pos += BS)
    {
        const int n = jmin (BS, out.getNumSamples() - pos);
        AudioBuffer<float> blk (out.getArrayOfWritePointers(), 2, pos, n);
        p.processBlock (blk, midi);
    }
    return out;
}

AudioBuffer<float> sine (float f, float secs, float amp = 0.5f)
{
    AudioBuffer<float> b (2, (int) (SR * secs));
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float v = amp * std::sin (MathConstants<float>::twoPi * f * (float) i / (float) SR);
        b.setSample (0, i, v);
        b.setSample (1, i, v);
    }
    return b;
}

float rms (const AudioBuffer<float>& b, int ch, int start, int len)
{
    double s = 0;
    for (int i = start; i < start + len; ++i) s += (double) b.getSample (ch, i) * b.getSample (ch, i);
    return (float) std::sqrt (s / jmax (1, len));
}

int firstAbove (const AudioBuffer<float>& b, int ch, float thr)
{
    for (int i = 0; i < b.getNumSamples(); ++i)
        if (std::abs (b.getSample (ch, i)) > thr) return i;
    return -1;
}

int peakIndex (const AudioBuffer<float>& b, int ch, int from, int to)
{
    int best = from; float bv = 0;
    for (int i = from; i < jmin (to, b.getNumSamples()); ++i)
        if (std::abs (b.getSample (ch, i)) > bv) { bv = std::abs (b.getSample (ch, i)); best = i; }
    return best;
}

void reset (DellayriumProcessor& p)
{
    p.prepareToPlay (SR, BS);
}
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI gui;
    const File outDir (argc > 1 ? argv[1] : "/tmp/dly");
    outDir.createDirectory();
    int failures = 0;
    auto check = [&] (bool ok, const String& what) { std::cout << (ok ? "  ok   " : "  FAIL ") << what << "\n"; if (! ok) ++failures; };

    DellayriumProcessor p;
    p.setPlayConfigDetails (2, 2, SR, BS);

    for (int engine = 0; engine < 2; ++engine)
    {
        const String en = engine == 0 ? "Analog" : "FFT";
        std::cout << "== " << en << " engine\n";
        // selectivity: band at 1 kHz
        soloBandSetup (p, 3, 1000.0f, engine, 0, 200.0f, 0.0f);
        reset (p);
        auto a = run (p, sine (1000.0f, 1.0f));
        reset (p);
        auto b = run (p, sine (100.0f, 1.0f));
        reset (p);
        auto c = run (p, sine (8000.0f, 1.0f));
        const float r1 = rms (a, 0, 24000, 24000), r2 = rms (b, 0, 24000, 24000), r3 = rms (c, 0, 24000, 24000);
        std::cout << "   1k in band: " << Decibels::gainToDecibels (r1 / 0.3536f) << " dB, 100 Hz: " << Decibels::gainToDecibels (r2 / 0.3536f)
                  << " dB, 8 kHz: " << Decibels::gainToDecibels (r3 / 0.3536f) << " dB\n";
        check (std::abs (Decibels::gainToDecibels (r1 / 0.3536f)) < 2.5f, en + ": centre of the band passes at ~0 dB");
        check (Decibels::gainToDecibels (r2 / r1) < -20.0f && Decibels::gainToDecibels (r3 / r1) < -20.0f, en + ": outside the bell is at least 20 dB down");

        // timing: impulse -> first echo at 200 ms
        soloBandSetup (p, 3, 1000.0f, engine, 0, 200.0f, 0.0f);
        setP (p, Params::id (3, Params::q), 0.3f);
        reset (p);
        AudioBuffer<float> imp (2, (int) SR);
        imp.clear();
        imp.setSample (0, 100, 1.0f); imp.setSample (1, 100, 1.0f);
        auto ir = run (p, imp);
        const int pk = peakIndex (ir, 0, 0, ir.getNumSamples());
        const float ms = (float) (pk - 100) / (float) SR * 1000.0f;
        std::cout << "   echo peak at " << ms << " ms\n";
        check (std::abs (ms - 200.0f) < 3.0f, en + ": echo lands at the set time");
    }

    // ping-pong: left first, then right
    {
        soloBandSetup (p, 3, 1000.0f, 0, 1, 150.0f, 70.0f);
        setP (p, Params::id (3, Params::q), 0.3f);
        reset (p);
        AudioBuffer<float> imp (2, (int) SR);
        imp.clear();
        imp.setSample (0, 100, 1.0f); imp.setSample (1, 100, 1.0f);
        auto ir = run (p, imp);
        const int l = peakIndex (ir, 0, 0, 12000), r = peakIndex (ir, 1, 0, 20000);
        std::cout << "== ping-pong: L first at " << (l - 100) / 48.0 << " ms, R first at " << (r - 100) / 48.0 << " ms\n";
        check (std::abs ((l - 100) / 48.0 - 150.0) < 3.0 && std::abs ((r - 100) / 48.0 - 300.0) < 4.0, "ping-pong alternates sides");
    }

    // stability + CPU: all 8 bands, max feedback, every processor in the loop
    {
        for (int b = 0; b < Params::numBands; ++b)
        {
            setP (p, Params::id (b, Params::on), 1);
            setP (p, Params::id (b, Params::level), 1);
            setP (p, Params::id (b, Params::fb), 100);
            setP (p, Params::id (b, Params::engine), (float) (b % 2));
            setP (p, Params::id (b, Params::mode), (float) (b % 4));
            setP (p, Params::id (b, Params::amt), 80);
            setP (p, Params::id (b, Params::drive), 70);
            setP (p, Params::id (b, Params::dtype), (float) (b % 4));
            setP (p, Params::id (b, Params::diff), 100);
            setP (p, Params::id (b, Params::duck), 50);
            setP (p, Params::id (b, Params::feel), 0);
            setP (p, Params::id (b, Params::sync), (float) (b % 5));
        }
        setP (p, Params::dry, 100);
        setP (p, Params::wet, 100);
        reset (p);
        AudioBuffer<float> noise (2, (int) (SR * 10));
        Random rnd (1);
        for (int i = 0; i < noise.getNumSamples(); ++i)
        {
            const float env = (i % 24000) < 4000 ? 0.5f : 0.0f;
            noise.setSample (0, i, (rnd.nextFloat() * 2 - 1) * env);
            noise.setSample (1, i, (rnd.nextFloat() * 2 - 1) * env);
        }
        const auto t0 = Time::getMillisecondCounterHiRes();
        auto out = run (p, noise);
        const auto t1 = Time::getMillisecondCounterHiRes();
        float peak = 0; bool nan = false;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < out.getNumSamples(); ++i)
            {
                const float v = out.getSample (ch, i);
                if (! std::isfinite (v)) nan = true;
                peak = jmax (peak, std::abs (v));
            }
        std::cout << "== stress: peak " << peak << ", 10 s rendered in " << (t1 - t0) << " ms ("
                  << String ((t1 - t0) / 100.0, 1) << " % of one core)\n";
        check (! nan, "no NaN / inf at max feedback");
        check (peak < 12.0f, "max feedback stays bounded");
        check ((t1 - t0) < 10000.0 * 0.35, "CPU under 35 % of a core for 8 bands at 48 kHz");
    }

    // drive must never make the feedback loop run away: 85 % feedback + 100 % drive has to die out
    {
        static const char* names[] = { "Tape", "Tube", "Gentle", "Transform" };
        for (int t = 0; t < 4; ++t)
        {
            soloBandSetup (p, 3, 1000.0f, 0, 0, 150.0f, 85.0f);
            setP (p, Params::id (3, Params::q), 0.6f);
            setP (p, Params::id (3, Params::drive), 100);
            setP (p, Params::id (3, Params::dtype), (float) t);
            setP (p, Params::id (3, Params::duck), 0);
            reset (p);
            AudioBuffer<float> in (2, (int) (SR * 10));
            in.clear();
            Random rnd (3);
            for (int i = 0; i < (int) (SR * 0.5); ++i)
            {
                const float v = (rnd.nextFloat() * 2 - 1) * 0.8f;
                in.setSample (0, i, v); in.setSample (1, i, v);
            }
            auto out = run (p, in);
            const float loud = rms (out, 0, (int) (SR * 0.2), (int) (SR * 0.5)), tail = rms (out, 0, (int) (SR * 9), (int) SR);
            const float db = Decibels::gainToDecibels (tail / jmax (1.0e-9f, loud), -200.0f);
            std::cout << "   drive " << names[t] << " 100 % at feedback 85 %: tail after 9 s " << db << " dB\n";
            check (db < -40.0f, String ("drive ") + names[t] + ": echoes die out instead of droning");
        }
    }

    // every factory preset has to ring out and stop
    {
        const auto names = PresetManager::getFactoryNames();
        for (int i = 0; i < names.size(); ++i)
        {
            reset (p);
            p.presets.loadFactory (i);
            AudioBuffer<float> in (2, (int) (SR * 14));
            in.clear();
            Random rnd (5);
            for (int s = 0; s < (int) (SR * 2); ++s)
            {
                const float tt = (float) (s % 12000) / 12000.0f;
                const float v = (rnd.nextFloat() * 2 - 1) * std::exp (-tt * 9.0f) * 0.5f;
                in.setSample (0, s, v); in.setSample (1, s, v);
            }
            auto out = run (p, in);
            const float loud = rms (out, 0, 0, (int) (SR * 2)), tail = rms (out, 0, (int) (SR * 13), (int) SR);
            bool finite = true;
            for (int s = 0; s < out.getNumSamples(); ++s) finite = finite && std::isfinite (out.getSample (0, s));
            const float db = Decibels::gainToDecibels (tail / jmax (1.0e-9f, loud), -200.0f);
            std::cout << "   preset " << names[i] << ": tail 11 s after the input stops " << db << " dB\n";
            check (finite && db < -40.0f, "preset " + names[i] + " rings out");
        }
    }

    // duck release: with a short release the echo is back quickly, with a long one it is still held down
    {
        float grFast = 0, grSlow = 0;
        for (int pass = 0; pass < 2; ++pass)
        {
            soloBandSetup (p, 3, 1000.0f, 0, 0, 50.0f, 0.0f);
            setP (p, Params::id (3, Params::duck), 100);
            setP (p, Params::id (3, Params::datk), 1);
            setP (p, Params::id (3, Params::drel), pass == 0 ? 50.0f : 1000.0f);
            reset (p);
            run (p, sine (1000.0f, 1.0f));
            AudioBuffer<float> sil (2, (int) (SR * 0.2));
            sil.clear();
            run (p, sil);
            (pass == 0 ? grFast : grSlow) = p.bandGr[3].load();
        }
        std::cout << "== duck 200 ms after the input stops: release 50 ms " << grFast << " dB, release 1 s " << grSlow << " dB\n";
        check (grFast > -1.0f && grSlow < -10.0f, "duck release time works");
        check (p.apvts.getParameter (Params::id (0, Params::freq))->getName (100) == "Band 1 Frequency"
                && p.apvts.getParameter (Params::id (0, Params::datk)) != nullptr && p.apvts.getParameter (Params::id (0, Params::drel)) != nullptr,
               "Frequency, Duck Attack and Duck Release are automatable parameters");
    }

    // ---- screenshots
    {
        DellayriumProcessor q;
        q.setPlayConfigDetails (2, 2, SR, BS);
        q.prepareToPlay (SR, BS);
        q.presets.loadFactory (0);
        for (const auto& theme : Theme::all())
        {
            q.setTheme (theme.name);
            std::unique_ptr<DellayriumEditor> ed (dynamic_cast<DellayriumEditor*> (q.createEditor()));
            ed->setSize (DellayriumEditor::baseW, DellayriumEditor::baseH);
            WelcomePrompt* welcome = nullptr;
            TourOverlay* tour = nullptr;
            Drawer* drawer = nullptr;
            for (auto* c : ed->getChildren())
            {
                if (auto* w = dynamic_cast<WelcomePrompt*> (c)) welcome = w;
                if (auto* t = dynamic_cast<TourOverlay*> (c)) tour = t;
                if (auto* d = dynamic_cast<Drawer*> (c)) drawer = d;
            }
            const bool welcomeShown = welcome != nullptr && welcome->isVisible();
            if (welcome != nullptr) welcome->setVisible (false);
            // feed some drums-ish noise so the analyzer, waterfall and sparks have something to show
            Random rnd (7);
            for (int frame = 0; frame < 70; ++frame)
            {
                AudioBuffer<float> blk (2, 1600);
                for (int i = 0; i < 1600; ++i)
                {
                    const float t = (float) ((frame * 1600 + i) % 12000) / 12000.0f;
                    const float env = std::exp (-t * 9.0f);
                    const float v = (rnd.nextFloat() * 2 - 1) * env * 0.4f + 0.2f * env * std::sin ((float) i * 0.013f);
                    blk.setSample (0, i, v);
                    blk.setSample (1, i, v);
                }
                MidiBuffer m;
                for (int pos = 0; pos < 1600; pos += 400)
                {
                    AudioBuffer<float> sub (blk.getArrayOfWritePointers(), 2, pos, 400);
                    q.processBlock (sub, m);
                }
                for (auto* c : ed->getChildren())
                    if (auto* s = dynamic_cast<Screen*> (c)) s->renderOnce();
                MessageManager::getInstance()->runDispatchLoopUntil (5);
            }
            MessageManager::getInstance()->runDispatchLoopUntil (300);
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            auto f = outDir.getChildFile ("ui-" + theme.name + ".png");
            f.deleteFile();
            if (auto os = f.createOutputStream()) PNGImageFormat().writeImageToStream (img, *os);
            std::cout << "wrote " << f.getFullPathName() << "\n";
            if (theme.name == "Asphalt")
            {
                ed->setSize (1200, 760);
                MessageManager::getInstance()->runDispatchLoopUntil (300);
                auto small = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
                auto f2 = outDir.getChildFile ("ui-small.png");
                f2.deleteFile();
                if (auto os = f2.createOutputStream()) PNGImageFormat().writeImageToStream (small, *os);

                // the size must survive closing and reopening the editor
                ed.reset();
                std::unique_ptr<AudioProcessorEditor> again (q.createEditor());
                check (again->getWidth() == 1200 && again->getHeight() == 760,
                       "window size remembered (" + String (again->getWidth()) + "x" + String (again->getHeight()) + ")");
                again.reset();
                ed.reset (dynamic_cast<DellayriumEditor*> (q.createEditor()));
                for (auto* c : ed->getChildren())
                {
                    if (auto* w = dynamic_cast<WelcomePrompt*> (c)) welcome = w;
                    if (auto* t = dynamic_cast<TourOverlay*> (c)) tour = t;
                    if (auto* d = dynamic_cast<Drawer*> (c)) drawer = d;
                }

                // switching bands in the drawer must not write one band's mode into another
                auto* m0 = q.apvts.getParameter (Params::id (0, "mode"));
                auto* m2 = q.apvts.getParameter (Params::id (2, "mode"));
                m0->setValueNotifyingHost (m0->convertTo0to1 (3.0f));
                m2->setValueNotifyingHost (m2->convertTo0to1 (1.0f));
                drawer->bindBand (0); drawer->bindBand (2); drawer->bindBand (0); drawer->bindBand (2);
                MessageManager::getInstance()->runDispatchLoopUntil (100);
                check (std::lround (m0->convertFrom0to1 (m0->getValue())) == 3 && std::lround (m2->convertFrom0to1 (m2->getValue())) == 1,
                       "band modes kept when switching bands");

                // after a preset change the drawer shows a band that is actually on
                q.setSelectedBand (1);
                q.presets.loadFactory (PresetManager::getFactoryNames().indexOf ("Ping-Pong Hats"));
                MessageManager::getInstance()->runDispatchLoopUntil (400);
                const int selAfter = q.getSelectedBand();
                check (q.apvts.getRawParameterValue (Params::id (selAfter, Params::on))->load() > 0.5f,
                       "drawer jumps to a live band after a preset change (band " + String (selAfter + 1) + ")");
                q.presets.loadFactory (PresetManager::getFactoryNames().indexOf ("Dub Darkness"));
                MessageManager::getInstance()->runDispatchLoopUntil (400);
                auto dimg = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
                auto fd = outDir.getChildFile ("ui-dub.png");
                fd.deleteFile();
                if (auto os = fd.createOutputStream()) PNGImageFormat().writeImageToStream (dimg, *os);

                check (welcomeShown, "welcome prompt shown on first open");
                welcome->setVisible (true);
                welcome->toFront (false);
                MessageManager::getInstance()->runDispatchLoopUntil (100);
                auto wimg = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
                auto f3 = outDir.getChildFile ("ui-welcome.png");
                f3.deleteFile();
                if (auto os = f3.createOutputStream()) PNGImageFormat().writeImageToStream (wimg, *os);
                welcome->setVisible (false);
                tour->start (Help::Lang::ru);
                for (int stepNo : { 2, 3, 7, 16 })
                {
                    tour->showStep (stepNo);
                    MessageManager::getInstance()->runDispatchLoopUntil (400);
                    auto timg = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
                    auto f4 = outDir.getChildFile ("ui-tour-" + String (stepNo) + ".png");
                    f4.deleteFile();
                    if (auto os = f4.createOutputStream()) PNGImageFormat().writeImageToStream (timg, *os);
                }
                tour->setVisible (false);
            }
        }
    }

    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + String (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
