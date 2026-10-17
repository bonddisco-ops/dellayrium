#include "PresetManager.h"
#include "Params.h"

namespace
{
    struct Factory { const char* name; std::vector<std::pair<juce::String, float>> values; };

    // a preset lists only what differs from the defaults
    const std::vector<Factory>& factory()
    {
        using namespace Params;
        static const std::vector<Factory> list {
            { "Spectral Rain", { { id (5, amt), 70 }, { id (5, diff), 35 }, { id (3, fb), 50 }, { id (1, fb), 30 } } },
            { "Ping-Pong Hats", { { id (1, on), 0 }, { id (5, on), 0 }, { id (3, on), 0 },
                                  { id (6, on), 1 }, { id (6, mode), 1 }, { id (6, Params::sync), 2 }, { id (6, level), 0.8f }, { id (6, fb), 55 }, { id (6, amt), 25 },
                                  { id (7, on), 1 }, { id (7, mode), 1 }, { id (7, Params::sync), 1 }, { id (7, feel), 2 }, { id (7, fb), 40 } } },
            // one wide mid band repeating dotted quarters, each repeat darker and dirtier, plus a quiet
            // ping-pong shimmer on top; both duck under the dry signal and swell back in
            { "Dub Darkness", { { id (1, on), 0 }, { id (5, on), 0 },
                                { id (3, on), 1 }, { id (3, freq), 900 }, { id (3, q), 0.7f }, { id (3, level), 0.9f }, { id (3, mode), 0 },
                                { id (3, Params::sync), 4 }, { id (3, feel), 1 }, { id (3, fb), 62 }, { id (3, amt), 65 },
                                { id (3, drive), 30 }, { id (3, dtype), 0 }, { id (3, duck), 35 }, { id (3, drel), 400 },
                                { id (3, diff), 12 }, { id (3, size), 18 },
                                { id (4, on), 1 }, { id (4, freq), 2600 }, { id (4, q), 1.2f }, { id (4, level), 0.55f }, { id (4, mode), 1 },
                                { id (4, Params::sync), 3 }, { id (4, feel), 1 }, { id (4, fb), 45 }, { id (4, amt), 15 },
                                { id (4, drive), 20 }, { id (4, duck), 35 }, { id (4, drel), 400 }, { id (4, mix), 70 } } },
            { "Glass Cloud", { { id (3, diff), 80 }, { id (3, size), 30 }, { id (5, diff), 85 }, { id (5, size), 22 }, { id (1, on), 0 },
                               { id (4, on), 1 }, { id (4, mode), 3 }, { id (4, engine), 1 }, { id (4, diff), 70 }, { id (4, amt), 80 } } },
            { "Tape Slapback", { { id (5, on), 0 }, { id (1, on), 0 }, { id (3, q), 0.5f }, { id (3, level), 0.9f }, { id (3, feel), 3 }, { id (3, ms), 110 },
                                 { id (3, fb), 15 }, { id (3, mode), 0 }, { id (3, drive), 40 }, { id (3, dtype), 0 }, { id (3, amt), 30 } } },
            { "Shattered Air", { { id (1, on), 0 }, { id (3, on), 0 }, { id (5, engine), 1 }, { id (5, amt), 100 }, { id (5, fb), 65 },
                                 { id (6, on), 1 }, { id (6, mode), 3 }, { id (6, engine), 1 }, { id (6, amt), 90 }, { id (6, Params::sync), 0 }, { id (6, fb), 50 },
                                 { id (7, on), 1 }, { id (7, mode), 3 }, { id (7, engine), 1 }, { id (7, amt), 100 } } },
        };
        return list;
    }
}

juce::StringArray PresetManager::getFactoryNames()
{
    juce::StringArray s;
    for (auto& f : factory()) s.add (f.name);
    return s;
}

bool PresetManager::loadFactory (int index)
{
    if (index < 0 || index >= (int) factory().size()) return false;
    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());
    for (auto& [id, v] : factory()[(size_t) index].values)
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    currentName = factory()[(size_t) index].name;
    markClean();
    ++loads;
    return true;
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state) : apvts (state)
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.addParameterListener (ranged->paramID, this);
}

PresetManager::~PresetManager()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.removeParameterListener (ranged->paramID, this);
}

juce::File PresetManager::getFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("Primal Rage Lab").getChildFile ("Dellayrium").getChildFile ("Presets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
        names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

void PresetManager::parameterChanged (const juce::String&, float)
{
    if (juce::Time::getMillisecondCounter() > ignoreUntil.load())
        modified = true;
}

void PresetManager::markClean()
{
    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    modified = false;
}

void PresetManager::loadInit()
{
    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());
    currentName = "Init";
    markClean();
    ++loads;
}

bool PresetManager::load (const juce::String& name)
{
    const auto file = getFolder().getChildFile (name + extension);
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    ignoreUntil = juce::Time::getMillisecondCounter() + 400;
    // Start from defaults so parameters missing in older presets are reset
    for (auto* p : apvts.processor.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    currentName = name;
    markClean();
    ++loads;
    return true;
}

bool PresetManager::save (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty())
        return false;

    auto state = apvts.copyState();
    state.removeProperty ("presetName", nullptr);
    if (auto xml = state.createXml())
    {
        if (xml->writeTo (getFolder().getChildFile (clean + extension)))
        {
            currentName = clean;
            markClean();
            return true;
        }
    }
    return false;
}

bool PresetManager::remove (const juce::String& name)
{
    const bool ok = getFolder().getChildFile (name + extension).deleteFile();
    if (ok && name == currentName)
    {
        currentName = "Init";
        modified = true;
    }
    return ok;
}

bool PresetManager::rename (const juce::String& from, const juce::String& to)
{
    const auto clean = juce::File::createLegalFileName (to.trim());
    if (clean.isEmpty())
        return false;
    const auto src = getFolder().getChildFile (from + extension);
    const auto dst = getFolder().getChildFile (clean + extension);
    if (! src.existsAsFile() || dst.exists())
        return false;
    const bool ok = src.moveFileTo (dst);
    if (ok && from == currentName)
        currentName = clean;
    return ok;
}

void PresetManager::step (int direction)
{
    auto names = getFactoryNames();
    const int nf = names.size();
    names.addArray (getPresetNames());
    if (names.isEmpty())
        return;
    int idx = names.indexOf (currentName);
    if (idx < 0)
        idx = direction > 0 ? -1 : 0;
    idx = (idx + direction + names.size()) % names.size();
    if (idx < nf) loadFactory (idx);
    else load (names[idx]);
}
