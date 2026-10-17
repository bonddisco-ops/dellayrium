#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

DellayriumProcessor::DellayriumProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DELLAYRIUM", Params::createLayout())
{
    using namespace Params;
    for (int b = 0; b < numBands; ++b)
    {
        auto get = [this, b] (const char* n) { return apvts.getRawParameterValue (id (b, n)); };
        bp[(size_t) b] = { get (Params::on), get (Params::freq), get (Params::level), get (Params::q), get (Params::engine),
                           get (Params::sync), get (Params::feel), get (Params::ms), get (Params::fb), get (Params::mode),
                           get (Params::amt), get (Params::duck), get (Params::drive), get (Params::dtype), get (Params::size),
                           get (Params::diff), get (Params::mix), get (Params::datk), get (Params::drel) };
    }
    dryParam = apvts.getRawParameterValue (dry);
    wetParam = apvts.getRawParameterValue (wet);
    if (! apvts.state.hasProperty ("theme")) apvts.state.setProperty ("theme", "Asphalt", nullptr);
    if (! apvts.state.hasProperty ("selected")) apvts.state.setProperty ("selected", 3, nullptr);
}

bool DellayriumProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    const auto in = l.getMainInputChannelSet();
    return out == AudioChannelSet::stereo() && (in == AudioChannelSet::stereo() || in == AudioChannelSet::mono());
}

void DellayriumProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& b : bands) b.prepare (sampleRate);
    splitter.prepare (sampleRate);
    for (int b = 0; b < Params::numBands; ++b)
    {
        curFreq[(size_t) b] = bp[(size_t) b].freq->load();
        curQ[(size_t) b] = bp[(size_t) b].q->load();
        curLevel[(size_t) b] = 0.0f;
        curMix[(size_t) b] = bp[(size_t) b].mix->load() * 0.01f;
        curFb[(size_t) b] = bp[(size_t) b].fb->load() * 0.01f;
    }
    curDry = dryParam->load() * 0.01f;
    curWet = wetParam->load() * 0.01f;
}

float DellayriumProcessor::delaySeconds (int b) const
{
    const auto& p = bp[(size_t) jlimit (0, Params::numBands - 1, b)];
    const int feel = roundToInt (p.feel->load());
    if (feel == 3)
        return p.ms->load() * 0.001f;
    double beats = Params::syncBeats (roundToInt (p.sync->load()));
    if (feel == 1) beats *= 1.5;
    if (feel == 2) beats *= 2.0 / 3.0;
    return (float) jmin (4.0, beats * 60.0 / jmax (20.0, currentBpm.load()));
}

void DellayriumProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    if (n == 0)
        return;
    if (numIn == 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                currentBpm = *bpm;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (jmin (1, buffer.getNumChannels() - 1));
    const float fsr = (float) sr;

    // ---- per-block setup
    struct Block
    {
        bool active = false, fftEngine = false;
        int mode = 0, dtype = 0;
        float targetLevel = 0, targetMix = 1, targetFb = 0, amt = 0, duck = 0, drive = 0, diffG = 0, makeup = 1;
        float duckAtt = 1, duckRel = 1;
        float timeL = 1, timeR = 1;
    };
    std::array<Block, Params::numBands> blk;
    const int solo = soloBand.load();
    const int lat = splitter.latency();

    for (int b = 0; b < Params::numBands; ++b)
    {
        auto& p = bp[(size_t) b];
        auto& s = bands[(size_t) b];
        auto& k = blk[(size_t) b];
        const bool on = p.on->load() > 0.5f;
        k.targetLevel = on ? Params::levelGain (p.level->load()) : 0.0f;
        k.active = on || curLevel[(size_t) b] > 1.0e-5f || s.energy > 1.0e-5f;
        if (! k.active)
        {
            s.energy = 0.0f;
            s.grDb = 0.0f;
            continue;
        }
        k.fftEngine = p.engine->load() > 0.5f;
        k.mode = roundToInt (p.mode->load());
        k.dtype = roundToInt (p.dtype->load());
        k.targetMix = p.mix->load() * 0.01f;
        k.targetFb = jmin (0.97f, p.fb->load() * 0.0098f);
        k.amt = p.amt->load() * 0.01f;
        k.duck = p.duck->load() * 0.01f;
        k.drive = p.drive->load() * 0.01f;
        // the shaper keeps unity gain for quiet signals so feedback can never run away;
        // the extra loudness of a driven echo is added after the loop
        k.makeup = 1.0f + k.drive * 0.8f;
        k.duckAtt = 1.0f - std::exp (-1.0f / (jmax (0.1f, p.datk->load()) * 0.001f * fsr));
        k.duckRel = 1.0f - std::exp (-1.0f / (jmax (10.0f, p.drel->load()) * 0.001f * fsr));
        k.diffG = p.diff->load() * 0.0072f;

        // filter coefficients glide towards the target a little each block
        auto& f = curFreq[(size_t) b];
        auto& q = curQ[(size_t) b];
        const float tf = p.freq->load(), tq = p.q->load();
        f = f * std::pow (tf / jmax (1.0f, f), 0.35f);
        q = q + (tq - q) * 0.35f;
        for (auto& svf : s.svfL) svf.set (f, q, fsr);
        for (auto& svf : s.svfR) svf.set (f, q, fsr);

        float t = delaySeconds (b) * fsr;
        if (k.fftEngine)
            t = jmax (1.0f, t - (float) lat);
        t = jlimit (1.0f, fsr * 4.2f, t);
        k.timeL = t;
        k.timeR = t;
        if (k.mode == 1) k.timeR = t * (1.0f + k.amt * 0.5f);          // ping-pong swing
        if (k.mode == 2) k.timeR = t + k.amt * 0.012f * fsr;           // wide: haas offset
        if (k.mode == 3) k.timeR = t + k.amt * 0.004f * fsr;
        if (s.timeL <= 0.0f) { s.timeL = k.timeL; s.timeR = k.timeR; }

        // diffusion network sized by Size
        const float sizeMs = p.size->load();
        static const float ratios[] = { 1.0f, 0.71f, 0.53f, 0.37f };
        for (int i = 0; i < 4; ++i)
        {
            s.diffL[(size_t) i].setLength ((int) (sizeMs * ratios[i] * 0.001f * fsr));
            s.diffR[(size_t) i].setLength ((int) (sizeMs * ratios[i] * 1.07f * 0.001f * fsr));
        }
        // darken: each repeat closes a little more from the top (Mono)
        const float darkHz = k.mode == 0 ? 20000.0f * std::pow (1.0f - k.amt, 2.2f) + 500.0f : 20000.0f;
        s.darkL.set (darkHz, fsr);
        s.darkR.set (darkHz, fsr);
        s.splitL.set (f, fsr);
        s.splitR.set (f, fsr);

        auto& su = splitter.setup[(size_t) b];
        su.active = k.fftEngine;
        su.freq = f;
        su.q = q;
        su.spectral = k.mode == 3;
        su.scatter = k.amt;
    }
    for (int b = 0; b < Params::numBands; ++b)
        if (! blk[(size_t) b].active)
            splitter.setup[(size_t) b].active = false;
    const bool useFft = splitter.anyActive();

    const float targetDry = dryParam->load() * 0.01f, targetWet = wetParam->load() * 0.01f;
    const float smooth = 1.0f - std::exp (-1.0f / (0.02f * fsr));     // ~20 ms parameter glide
    const float timeSmooth = 1.0f - std::exp (-1.0f / (0.08f * fsr));
    const float energyDecay = std::exp (-1.0f / (0.25f * fsr));

    std::array<std::array<float, 2>, dly::SpectralSplitter::maxBands> fftOut {};
    float inPk[2] {}, outPk[2] {};
    int scopeStart1, scopeSize1, scopeStart2, scopeSize2;
    scopeFifo.prepareToWrite (n, scopeStart1, scopeSize1, scopeStart2, scopeSize2);

    auto softLimit = [] (float v) { const float a = std::abs (v); return a < 1.0f ? v : std::copysign (1.0f + std::tanh (a - 1.0f), v); };

    for (int i = 0; i < n; ++i)
    {
        const float xL = L[i], xR = R[i];
        inPk[0] = jmax (inPk[0], std::abs (xL));
        inPk[1] = jmax (inPk[1], std::abs (xR));
        const float a = jmax (std::abs (xL), std::abs (xR));

        if (useFft)
            splitter.process (xL, xR, fftOut);

        curDry += smooth * (targetDry - curDry);
        curWet += smooth * (targetWet - curWet);

        float wetL = 0, wetR = 0, soloL = 0, soloR = 0;
        for (int b = 0; b < Params::numBands; ++b)
        {
            const auto& k = blk[(size_t) b];
            if (! k.active)
                continue;
            auto& s = bands[(size_t) b];
            auto& lvl = curLevel[(size_t) b];
            auto& mx = curMix[(size_t) b];
            auto& fb = curFb[(size_t) b];
            lvl += smooth * (k.targetLevel - lvl);
            mx += smooth * (k.targetMix - mx);
            fb += smooth * (k.targetFb - fb);
            s.timeL += timeSmooth * (k.timeL - s.timeL);
            s.timeR += timeSmooth * (k.timeR - s.timeR);

            // the slice of the input under this bell
            float sL, sR;
            if (k.fftEngine)
            {
                sL = fftOut[(size_t) b][0];
                sR = fftOut[(size_t) b][1];
            }
            else
            {
                sL = s.svfL[1].process (s.svfL[0].process (xL));
                sR = s.svfR[1].process (s.svfR[0].process (xR));
            }
            sL *= lvl;
            sR *= lvl;

            auto chain = [&] (float v, bool left)
            {
                v = dly::driveSample (v, k.dtype, k.drive);
                if (k.mode == 0 && k.amt > 0.001f)
                    v = left ? s.darkL.process (v) : s.darkR.process (v);
                if (k.diffG > 0.001f)
                {
                    auto& net = left ? s.diffL : s.diffR;
                    for (auto& ap : net) v = ap.process (v, k.diffG);
                }
                v = left ? s.dcL.process (v) : s.dcR.process (v);
                return softLimit (v);
            };

            float oL, oR;
            switch (k.mode)
            {
                case 0: // mono
                {
                    const float m = 0.5f * (sL + sR);
                    s.dl.push (m + s.fbL * fb);
                    const float rep = chain (s.dl.read (s.timeL), true);
                    s.fbL = rep;
                    oL = oR = rep;
                    break;
                }
                case 1: // ping-pong: in on the left, every repeat jumps sides
                {
                    const float m = 0.5f * (sL + sR);
                    s.dl.push (m + s.fbR * fb);
                    s.dr.push (s.fbL * fb);
                    const float repL = chain (s.dl.read (s.timeL), true);
                    const float repR = chain (s.dr.read (s.timeR), false);
                    s.fbL = repL;
                    s.fbR = repR;
                    oL = repL;
                    oR = repR;
                    break;
                }
                default: // wide and spectral wide: true stereo
                {
                    s.dl.push (sL + s.fbL * fb);
                    s.dr.push (sR + s.fbR * fb);
                    float repL = chain (s.dl.read (s.timeL), true);
                    float repR = chain (s.dr.read (s.timeR), false);
                    s.fbL = repL;
                    s.fbR = repR;
                    if (k.mode == 2)
                    {
                        const float mid = 0.5f * (repL + repR), side = 0.5f * (repL - repR) * (0.6f + k.amt * 1.6f);
                        oL = mid + side;
                        oR = mid - side;
                    }
                    else
                    {
                        // scatter: lows of the band to one side, highs to the other, then decorrelate
                        const float sc = k.amt;
                        const float loL = s.splitL.process (repL), hiL = repL - loL;
                        const float loR = s.splitR.process (repR), hiR = repR - loR;
                        float l = (loL * (1.0f + sc) + hiL * (1.0f - sc)) / (1.0f + sc * 0.5f);
                        float r = (loR * (1.0f - sc) + hiR * (1.0f + sc)) / (1.0f + sc * 0.5f);
                        if (! k.fftEngine && sc > 0.001f)
                        {
                            float dl = l, dr = r;
                            for (auto& ap : s.decoL) dl = ap.process (dl, 0.6f);
                            for (auto& ap : s.decoR) dr = ap.process (dr, 0.6f);
                            l += (dl - l) * sc;
                            r += (dr - r) * sc;
                        }
                        oL = l;
                        oR = r;
                    }
                    break;
                }
            }
            // duck the echo while the dry signal plays
            // each band has its own envelope, with its own attack and release
            s.env += (a > s.env ? k.duckAtt : k.duckRel) * (a - s.env);
            float g = 1.0f;
            if (k.duck > 0.001f)
            {
                const float gr = k.duck * jlimit (0.0f, 1.0f, s.env * 3.0f);
                g = 1.0f - 0.94f * gr;
            }
            s.grDb = Decibels::gainToDecibels (g);
            oL *= g * mx * k.makeup;
            oR *= g * mx * k.makeup;
            s.energy = jmax (s.energy * energyDecay, jmax (std::abs (oL), std::abs (oR)));

            wetL += oL;
            wetR += oR;
            if (b == solo)
            {
                soloL = sL + oL;
                soloR = sR + oR;
            }
        }

        float outL, outR;
        if (solo >= 0 && solo < Params::numBands)
        {
            outL = soloL;
            outR = soloR;
        }
        else
        {
            outL = xL * curDry + wetL * curWet;
            outR = xR * curDry + wetR * curWet;
        }
        L[i] = outL;
        R[i] = outR;
        outPk[0] = jmax (outPk[0], std::abs (outL));
        outPk[1] = jmax (outPk[1], std::abs (outR));
        const float mono = 0.5f * (outL + outR);
        if (i < scopeSize1) scopeBuf[(size_t) (scopeStart1 + i)] = mono;
        else if (i - scopeSize1 < scopeSize2) scopeBuf[(size_t) (scopeStart2 + i - scopeSize1)] = mono;
    }
    scopeFifo.finishedWrite (scopeSize1 + scopeSize2);

    for (int c = 0; c < 2; ++c)
    {
        inPeak[c] = jmax (inPeak[c].load(), inPk[c]);
        outPeak[c] = jmax (outPeak[c].load(), outPk[c]);
    }
    for (int b = 0; b < Params::numBands; ++b)
    {
        bandEnergy[b] = bands[(size_t) b].energy;
        bandGr[b] = bands[(size_t) b].grDb;
    }
}

//==============================================================================
String DellayriumProcessor::getTheme() const { return apvts.state.getProperty ("theme", "Asphalt").toString(); }
void DellayriumProcessor::setTheme (const String& t) { apvts.state.setProperty ("theme", t, nullptr); }
int DellayriumProcessor::getSelectedBand() const { return jlimit (0, Params::numBands - 1, (int) apvts.state.getProperty ("selected", 3)); }
void DellayriumProcessor::setSelectedBand (int b) { apvts.state.setProperty ("selected", b, nullptr); }

void DellayriumProcessor::getStateInformation (MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void DellayriumProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            presets.setCurrentNameFromState (tree.getProperty ("presetName", "Init").toString());
            apvts.replaceState (tree);
        }
}

AudioProcessorEditor* DellayriumProcessor::createEditor() { return new DellayriumEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DellayriumProcessor(); }
