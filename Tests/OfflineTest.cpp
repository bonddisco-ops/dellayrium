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
            }
        }
    }

    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + String (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
