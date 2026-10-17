#!/bin/bash
# Dellayrium installer for macOS: copies the plugins and removes the download quarantine.
cd "$(dirname "$0")"
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3" "$AU"
echo "Installing Dellayrium..."
rm -rf "$VST3/Dellayrium.vst3" "$AU/Dellayrium.component"
cp -R "Dellayrium.vst3" "$VST3/" && echo "  VST3 -> $VST3"
cp -R "Dellayrium.component" "$AU/" && echo "  AU   -> $AU"
xattr -cr "$VST3/Dellayrium.vst3" "$AU/Dellayrium.component" 2>/dev/null
xattr -cr "Dellayrium.app" 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null
echo ""
echo "Done! Rescan plugins in your DAW. / Готово! Пересканируй плагины в DAW."
echo "You can close this window. / Это окно можно закрыть."
