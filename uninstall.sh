#!/usr/bin/env bash
set -euo pipefail

echo "==> Usuwanie wtyczek podglądu CorelDRAW (.cdr) z profilu użytkownika KDE..."

rm -f "$HOME/.local/lib/qt6/plugins/imageformats/kimg_cdr.so"
rm -f "$HOME/.local/lib/qt6/plugins/kf6/thumbcreator/cdrthumbnail.so"
rm -f "$HOME/.local/share/thumbnailers/cdrthumbnail.thumbnailer"
rm -f "$HOME/.local/share/mime/packages/coreldraw-cdr.xml"
rm -f "$HOME/.local/share/applications/org.kde.dolphin.desktop"
rm -f "$HOME/.local/share/applications/org.kde.gwenview.desktop"
rm -f "$HOME/.config/plasma-workspace/env/qt-plugins.sh"
rm -f "$HOME/.config/environment.d/qt-plugins.conf"

update-mime-database "$HOME/.local/share/mime" 2>/dev/null || true
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
killall kioworker 2>/dev/null || true

echo "==> Odinstalowano pomyślnie."
