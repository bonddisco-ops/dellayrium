#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

// Building blocks of Dellayrium's sound.
namespace dly
{
constexpr float pi = juce::MathConstants<float>::pi;

//==============================================================================
// Band-pass (Cytomic TPT state variable filter), normalised to 0 dB at the centre
struct SvfBand
{
    float a1 = 0, a2 = 0, a3 = 0, k = 1, ic1 = 0, ic2 = 0;
    void set (float fc, float q, float sr)
    {
        const float g = std::tan (pi * juce::jlimit (10.0f, sr * 0.49f, fc) / sr);
        k = 1.0f / juce::jmax (0.05f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    float process (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return v1 * k;
    }
    void reset() { ic1 = ic2 = 0; }
};

// Magnitude of the two cascaded band-passes at frequency f (used by the display too)
inline float analogResponse (float f, float fc, float q)
{
    const float r = f / fc, k = 1.0f / juce::jmax (0.05f, q);
    const float m = k * r / std::sqrt ((1.0f - r * r) * (1.0f - r * r) + (k * r) * (k * r));
    return m * m;
}

// The FFT engine's band shape: flat top, steep walls
inline float fftResponse (float f, float fc, float q)
{
    const float halfWidthOct = 0.75f / juce::jmax (0.05f, q);
    const float d = std::log2 (juce::jmax (1.0f, f) / fc) / halfWidthOct;
    return 1.0f / (1.0f + std::pow (d * d, 4.0f));
}

//==============================================================================
struct OnePoleLP
{
    float a = 1, z = 0;
    void set (float fc, float sr) { a = 1.0f - std::exp (-2.0f * pi * juce::jlimit (10.0f, sr * 0.49f, fc) / sr); }
    float process (float x) { z += a * (x - z); return z; }
    void reset() { z = 0; }
};

struct DcBlock
{
    float x1 = 0, y1 = 0;
    float process (float x) { const float y = x - x1 + 0.995f * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0; }
};

//==============================================================================
// Fractional delay line with cubic interpolation
struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;
    void prepare (int maxSamples)
    {
        int n = 1;
        while (n < maxSamples + 8) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float delay) const
    {
        delay = juce::jlimit (1.0f, (float) (mask - 4), delay);
        const float pos = (float) w - delay;
        const int i = (int) std::floor (pos);
        const float t = pos - (float) i;
        auto at = [this] (int idx) { return buf[(size_t) (idx & mask)]; };
        const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }
};

// Schroeder all-pass: smears an echo without colouring it much
struct Allpass
{
    std::vector<float> buf;
    int len = 1, pos = 0;
    void prepare (int maxLen) { buf.assign ((size_t) juce::jmax (2, maxLen), 0.0f); pos = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void setLength (int n) { len = juce::jlimit (1, (int) buf.size() - 1, n); if (pos >= len) pos = 0; }
    float process (float x, float g)
    {
        const float d = buf[(size_t) pos];
        const float y = -g * x + d;
        buf[(size_t) pos] = x + g * y;
        if (++pos >= len) pos = 0;
        return y;
    }
};

//==============================================================================
inline float driveSample (float x, int type, float amount)
{
    if (amount <= 0.0005f)
        return x;
    const float pre = 1.0f + amount * 14.0f;
    const float v = x * pre;
    float y;
    switch (type)
    {
        case 0:  y = std::tanh (v); break;                                                        // tape: soft, round
        case 1:  y = v >= 0 ? std::tanh (v * 1.2f) : std::tanh (v * 0.7f) * 1.15f; break;         // tube: asymmetric, adds even harmonics
        case 2:  y = v / (1.0f + std::abs (v)); break;                                            // gentle
        default: y = std::sin (v * 1.4f) * 0.8f; break;                                           // transform: wave folding
    }
    const float wetAmt = juce::jmin (1.0f, amount * 3.0f);
    return x + (y / std::sqrt (pre) - x) * wetAmt;
}

//==============================================================================
// One delay band (Analog engine input filter + the whole echo path)
struct BandState
{
    std::array<SvfBand, 2> svfL, svfR;
    DelayLine dl, dr;
    std::array<Allpass, 4> diffL, diffR;
    std::array<Allpass, 3> decoL, decoR;      // decorrelation for Spectral Wide (Analog)
    OnePoleLP darkL, darkR, splitL, splitR, toneL, toneR;
    DcBlock dcL, dcR;
    float fbL = 0, fbR = 0;                   // last processed repeat (fed back)
    float timeL = 0, timeR = 0;               // smoothed delay time in samples
    float env = 0;                            // ducking envelope of the dry input
    float grDb = 0;                           // for the UI
    float energy = 0;                         // wet level for the UI

    void prepare (double sr)
    {
        dl.prepare ((int) (sr * 4.3));
        dr.prepare ((int) (sr * 4.3));
        const int apMax = (int) (sr * 0.06) + 4;
        for (auto* set : { &diffL, &diffR })
            for (auto& a : *set) a.prepare (apMax);
        static const float decoMs[] = { 3.1f, 4.7f, 6.3f };
        for (int i = 0; i < 3; ++i)
        {
            decoL[(size_t) i].prepare ((int) (sr * 0.01) + 2);
            decoR[(size_t) i].prepare ((int) (sr * 0.01) + 2);
            decoL[(size_t) i].setLength ((int) (decoMs[i] * 0.001 * sr));
            decoR[(size_t) i].setLength ((int) (decoMs[2 - i] * 1.13 * 0.001 * sr));
        }
        reset();
    }
    void reset()
    {
        for (auto& f : svfL) f.reset();
        for (auto& f : svfR) f.reset();
        dl.reset(); dr.reset();
        for (auto& a : diffL) a.reset();
        for (auto& a : diffR) a.reset();
        for (auto& a : decoL) a.reset();
        for (auto& a : decoR) a.reset();
        for (auto* f : { &darkL, &darkR, &splitL, &splitR, &toneL, &toneR }) f->reset();
        dcL.reset(); dcR.reset();
        fbL = fbR = 0;
        env = 0; grDb = 0; energy = 0;
        timeL = timeR = 0;
    }
};

//==============================================================================
// FFT engine: splits the input into flat-topped bands with steep walls.
// Every band gets its own stereo output; latency = fft size.
class SpectralSplitter
{
public:
    static constexpr int maxBands = 8;

    void prepare (double sampleRate)
    {
        order = sampleRate > 60000.0 ? 11 : 10;
        N = 1 << order;
        hop = N / 4;
        sr = (float) sampleRate;
        fft = std::make_unique<juce::dsp::FFT> (order);
        window.resize ((size_t) N);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = std::sqrt (0.5f - 0.5f * std::cos (2.0f * pi * (float) i / (float) N));
        for (auto& ch : in) ch.assign ((size_t) N, 0.0f);
        for (auto& b : out)
            for (auto& ch : b) ch.assign ((size_t) N, 0.0f);
        frameL.assign ((size_t) N * 2, 0.0f);
        frameR.assign ((size_t) N * 2, 0.0f);
        work.assign ((size_t) N * 2, 0.0f);
        juce::Random rnd (4242);
        for (auto& b : pan)
        {
            b.resize ((size_t) N / 2 + 1);
            for (auto& v : b) v = rnd.nextFloat() * 2.0f - 1.0f;
        }
        pos = 0;
        count = 0;
    }

    int latency() const { return N; }

    struct BandSetup { bool active = false; float freq = 1000, q = 2, scatter = 0; bool spectral = false; };
    std::array<BandSetup, maxBands> setup;

    // one stereo sample in, every active band's stereo sample out
    void process (float l, float r, std::array<std::array<float, 2>, maxBands>& result)
    {
        in[0][(size_t) pos] = l;
        in[1][(size_t) pos] = r;
        for (int b = 0; b < maxBands; ++b)
        {
            result[(size_t) b][0] = out[(size_t) b][0][(size_t) pos];
            result[(size_t) b][1] = out[(size_t) b][1][(size_t) pos];
            out[(size_t) b][0][(size_t) pos] = 0.0f;
            out[(size_t) b][1][(size_t) pos] = 0.0f;
        }
        pos = (pos + 1) % N;
        if (++count >= hop)
        {
            count = 0;
            frame();
        }
    }

    bool anyActive() const
    {
        for (auto& s : setup) if (s.active) return true;
        return false;
    }

private:
    void frame()
    {
        // oldest sample sits at pos
        for (int i = 0; i < N; ++i)
        {
            const int idx = (pos + i) % N;
            frameL[(size_t) i] = in[0][(size_t) idx] * window[(size_t) i];
            frameR[(size_t) i] = in[1][(size_t) idx] * window[(size_t) i];
        }
        std::fill (frameL.begin() + N, frameL.end(), 0.0f);
        std::fill (frameR.begin() + N, frameR.end(), 0.0f);
        fft->performRealOnlyForwardTransform (frameL.data(), true);
        fft->performRealOnlyForwardTransform (frameR.data(), true);

        const float binHz = sr / (float) N;
        const float norm = 0.5f / (float) N;   // hann^2 at 75% overlap sums to 2; JUCE's inverse is unnormalised
        for (int b = 0; b < maxBands; ++b)
        {
            const auto& s = setup[(size_t) b];
            if (! s.active)
                continue;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto& src = ch == 0 ? frameL : frameR;
                for (int k = 0; k <= N / 2; ++k)
                {
                    float m = fftResponse ((float) k * binHz, s.freq, s.q);
                    if (s.spectral && s.scatter > 0.0f)
                    {
                        const float theta = (pan[(size_t) b][(size_t) k] * s.scatter + 1.0f) * 0.25f * pi;
                        m *= (ch == 0 ? std::cos (theta) : std::sin (theta)) * 1.41421356f;
                    }
                    work[(size_t) (2 * k)] = src[(size_t) (2 * k)] * m;
                    work[(size_t) (2 * k + 1)] = src[(size_t) (2 * k + 1)] * m;
                }
                for (int k = N / 2 + 1; k < N; ++k)
                {
                    work[(size_t) (2 * k)] = work[(size_t) (2 * (N - k))];
                    work[(size_t) (2 * k + 1)] = -work[(size_t) (2 * (N - k) + 1)];
                }
                fft->performRealOnlyInverseTransform (work.data());
                auto& o = out[(size_t) b][(size_t) ch];
                for (int i = 0; i < N; ++i)
                    o[(size_t) ((pos + i) % N)] += work[(size_t) i] * window[(size_t) i] * norm * (float) N;
            }
        }
    }

    int order = 10, N = 1024, hop = 256, pos = 0, count = 0;
    float sr = 44100;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, frameL, frameR, work;
    std::array<std::vector<float>, 2> in;
    std::array<std::array<std::vector<float>, 2>, maxBands> out;
    std::array<std::vector<float>, maxBands> pan;
};
} // namespace dly
