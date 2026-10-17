#pragma once
#include <juce_core/juce_core.h>

// Hints and tour texts in English and Russian. The interface itself is always in English.
namespace Help
{
    enum class Lang { en, ru };

    struct Entry { const char* key; const char* en; const char* ru; };

    inline const Entry tips[] = {
        { "dry", "Dry: level of the untouched signal. Put it at 0 and Wet at 100 to use Dellayrium on a send.",
                 "Dry: громкость чистого сигнала. Поставь 0, а Wet 100, и Dellayrium можно вешать на посыл." },
        { "wet", "Wet: level of all the echoes together.", "Wet: громкость всех эхо вместе." },
        { "inMeter", "Input level. The number on top is the peak.", "Уровень на входе. Сверху пик." },
        { "outMeter", "Output level. The peak turns red above 0 dB.", "Уровень на выходе. Пик краснеет выше 0 dB." },
        { "presets", "Presets. Arrows to browse, the name for the list, the disk to save. * means unsaved changes.",
                     "Пресеты. Стрелки листают, по названию открывается список, дискетка сохраняет. * значит, есть несохранённые изменения." },
        { "gear", "Settings: theme, help, help language, the tour and window size.",
                  "Настройки: тема, подсказки, язык подсказок, обучение и размер окна." },
        { "screen", "Every point is its own frequency band with its own delay. Drag a point: where and how much goes into the delay. "
                    "Mouse wheel on a point: width. Double-click: add or remove a point. Right-click: options.",
                    "Каждая точка это своя полоса частот со своим дилеем. Тяни точку: какие частоты и сколько уходит в дилей. "
                    "Колёсико на точке: ширина. Двойной клик: добавить или убрать точку. Правый клик: меню." },
        { "engine", "Analog: a filter cuts the band, no extra latency, warm repeats. FFT: cuts straight through the spectrum, razor-sharp edges, glassy repeats.",
                    "Analog: полосу вырезает фильтр, без задержки, тёплые повторы. FFT: режет прямо по спектру, края как бритва, стеклянные повторы." },
        { "time", "Delay time in notes, synced to the project tempo.", "Время дилея в долях, привязано к темпу проекта." },
        { "timeMs", "Delay time in milliseconds.", "Время дилея в миллисекундах." },
        { "feel", "1/1: straight notes. Dot: dotted. T: triplets. ms: free time in milliseconds.",
                  "1/1: ровные доли. Точка: с точкой. T: триоли. ms: свободное время в миллисекундах." },
        { "feedback", "Feedback: how many times the echo repeats.", "Feedback: сколько раз повторяется эхо." },
        { "mode", "Mono: one echo in the middle. Ping-Pong: left, right, left... Wide: a wide stereo echo. Spectral Wide: the frequencies of the band fly apart across the stereo field.",
                  "Mono: одно эхо по центру. Ping-Pong: лево, право, лево... Wide: широкое стерео-эхо. Spectral Wide: частоты внутри полосы разлетаются по стерео." },
        { "darken", "Darken: every repeat gets a little duller, closed from the top.", "Darken: каждый повтор чуть глуше, закрывается сверху." },
        { "swing", "Swing: shifts the right-side bounce later, so the ping-pong grooves instead of ticking evenly.",
                   "Swing: сдвигает правый отскок позже, и пинг-понг начинает качать, а не стучать ровно." },
        { "width", "Width: from almost mono to as wide as it gets.", "Width: от почти моно до максимально широко." },
        { "scatter", "Scatter: how far the frequencies inside the band fly apart across the stereo field.",
                     "Scatter: насколько сильно частоты внутри полосы разлетаются по стерео." },
        { "duck", "Duck: the echo gets out of the way while the dry signal plays. The orange ring shows how much it is pushed down right now.",
                  "Duck: эхо уступает место, пока играет чистый сигнал. Оранжевое кольцо показывает, насколько его поджимает прямо сейчас." },
        { "drive", "Drive: saturation inside the echo, every repeat gets dirtier. Pick the character in the box above.",
                   "Drive: перегруз внутри эха, каждый повтор грязнее. Характер выбирается в списке над ручкой." },
        { "driveType", "Tape: round and soft. Tube: warm, adds even harmonics. Gentle: barely there. Transform: folds the wave, metallic.",
                       "Tape: мягко и кругло. Tube: тепло, добавляет чётные гармоники. Gentle: еле заметно. Transform: складывает волну, металлически." },
        { "size", "Size: how big the tiny space is that each repeat sits in (2-50 ms).",
                  "Size: размер маленькой комнатки, куда попадает каждый повтор (2-50 мс)." },
        { "diffuse", "Diffuse: how much each repeat is smeared inside that space. 0 = clean repeats, 100 = soft clouds.",
                     "Diffuse: насколько каждый повтор размазывается в этой комнатке. 0 = чёткие повторы, 100 = мягкие облака." },
        { "mix", "Mix: output level of this band's echo.", "Mix: громкость эха этой полосы." },
        { "power", "Switch this band on or off.", "Включить или выключить эту полосу." },
        { "chevron", "Hide or show the band settings.", "Спрятать или показать настройки полосы." },
    };

    inline juce::String tip (const char* key, Lang lang)
    {
        for (const auto& e : tips)
            if (juce::String (e.key) == key)
                return juce::String::fromUTF8 (lang == Lang::ru ? e.ru : e.en);
        return {};
    }

    struct Step { const char* target; const char* titleEn; const char* textEn; const char* titleRu; const char* textRu; };

    inline const Step tour[] = {
        { "", "Welcome to Dellayrium",
              "A delay that looks like an EQ. A quick tour of every part: use Next and Back, or Skip at any time. You can start it again from the gear menu.",
              "Добро пожаловать в Dellayrium",
              "Дилей, который выглядит как эквалайзер. Короткий тур по всем частям: листай кнопками «Дальше» и «Назад» или пропусти в любой момент. Запустить заново можно из меню шестерёнки." },
        { "screen", "The screen", "Every point is a frequency band with its own delay. Its height is how much of that band goes into the delay. The coloured curve shows all bands together.",
                    "Экран", "Каждая точка это полоса частот со своим дилеем. Её высота это сколько этой полосы уходит в дилей. Цветная кривая показывает все полосы вместе." },
        { "node", "Points", "Drag a point to move it, mouse wheel on it changes the width (Q). Double-click empty space to add a point (up to 8), double-click a point to remove it. Shift = fine moves.",
                  "Точки", "Тяни точку, чтобы двигать, колёсико на ней меняет ширину (Q). Двойной клик по пустому месту добавляет точку (до 8), двойной клик по точке убирает её. Shift = точнее." },
        { "solo", "Listen to one band", "Press the headphones and you hear only this band and its echo. Hold them and drag: the point moves while you listen. Alt+click a point works too.",
                  "Прослушать полосу", "Нажми наушники, и слышно только эту полосу с её эхом. Зажми их и веди: точка двигается, пока слушаешь. Alt+клик по точке тоже работает." },
        { "analyzer", "Waterfall", "The spectrum falls down like a waterfall, so you see the echoes ring out. Sparks fly where a band is ringing. OFF hides it.",
                      "Водопад", "Спектр стекает вниз водопадом, и видно, как звенят повторы. Искры летят там, где звенит полоса. OFF прячет его." },
        { "drawer", "Band settings", "Click a point and its settings slide up here. The arrow hides them, the power button switches the band off.",
                    "Настройки полосы", "Кликни точку, и её настройки выезжают сюда. Стрелка их прячет, кнопка питания выключает полосу." },
        { "engine", "Analog or FFT", "Analog: a filter, no extra latency, warm. FFT: cuts straight through the spectrum with razor-sharp edges, glassy repeats.",
                    "Analog или FFT", "Analog: фильтр, без задержки, тепло. FFT: режет прямо по спектру, края как бритва, стеклянные повторы." },
        { "time", "Time and Feedback", "Time in notes synced to the tempo (straight, dotted, triplet) or in ms. Feedback sets how many times it repeats.",
                  "Время и Feedback", "Время в долях по темпу (ровно, с точкой, триоль) или в мс. Feedback задаёт, сколько раз повторяется." },
        { "mode", "Mode", "Mono, Ping-Pong, Wide or Spectral Wide. The knob under it changes with the mode: Darken, Swing, Width or Scatter.",
                  "Режим", "Mono, Ping-Pong, Wide или Spectral Wide. Ручка под ним меняется вместе с режимом: Darken, Swing, Width или Scatter." },
        { "duck", "Duck", "The echo steps back while the dry signal plays. The orange ring shows how much it is pushed down.",
                  "Duck", "Эхо отступает, пока играет чистый сигнал. Оранжевое кольцо показывает, насколько его поджимает." },
        { "drive", "Drive", "Saturation inside the echo: every repeat gets dirtier. Tape, Tube, Gentle or Transform.",
                   "Drive", "Перегруз внутри эха: каждый повтор грязнее. Tape, Tube, Gentle или Transform." },
        { "space", "Size and Diffuse", "Size is how big the tiny space is that each repeat sits in. Diffuse is how much the repeat is smeared in it: from clean echoes to soft clouds.",
                   "Size и Diffuse", "Size это размер маленькой комнатки, куда попадает каждый повтор. Diffuse это насколько он в ней размазывается: от чётких повторов до мягких облаков." },
        { "mix", "Mix", "Output level of this band's echo.", "Mix", "Громкость эха этой полосы." },
        { "inCol", "Dry", "Level of the untouched signal.", "Dry", "Громкость чистого сигнала." },
        { "outCol", "Wet", "Level of all echoes. Dry at 0 and Wet at 100 = use it on a send.",
                    "Wet", "Громкость всех эхо. Dry в 0 и Wet на 100 = вешай на посыл." },
        { "presets", "Presets", "Arrows to browse, the name for the list, the disk to save. Factory presets come first.",
                     "Пресеты", "Стрелки листают, по названию открывается список, дискетка сохраняет. Сначала идут заводские." },
        { "gear", "Settings", "Theme, help on or off, help language, this tour and window size. Drag the bottom-right corner to resize the window any way you like.",
                  "Настройки", "Тема, подсказки, язык подсказок, этот тур и размер окна. Окно тянется за правый нижний угол в любую сторону." },
        { "", "That's it!", "Hover over anything for a hint. Go make something weird.",
              "Готово!", "Наведи мышку на что угодно, и появится подсказка. Иди твори дичь." },
    };
    inline constexpr int tourLength = (int) (sizeof (tour) / sizeof (tour[0]));

    inline juce::String ui (const char* en, const char* ru, Lang lang)
    {
        return juce::String::fromUTF8 (lang == Lang::ru ? ru : en);
    }
}
