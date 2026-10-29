#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLI_BIN="${VECTORCRAFT_CLI:-$HOME/bin/vectorcraft-cli}"
PLUGIN_BASE="$HOME/.local/lib/qt6/plugins"
IMG_PLUGIN_DIR="$PLUGIN_BASE/imageformats"
OBSOLETE_THUMB_SO="$PLUGIN_BASE/kf6/thumbcreator/cdrthumbnail.so"

echo "==> Instalacja natywnego podglądu plików CorelDRAW (.cdr) w KDE Plasma 6 (Dolphin + Gwenview)..."

# Usunięcie przestarzałej wtyczki C++ thumbcreator, która mogła powodować błąd w kioworkerze
if [[ -f "$OBSOLETE_THUMB_SO" ]]; then
    echo "==> Usuwanie przestarzałego $OBSOLETE_THUMB_SO (zastąpionego przez bezpieczny .thumbnailer)..."
    rm -f "$OBSOLETE_THUMB_SO"
fi

if [[ ! -x "$CLI_BIN" ]]; then
    if command -v vectorcraft-cli >/dev/null 2>&1; then
        CLI_BIN="$(command -v vectorcraft-cli)"
    else
        echo "UWAGA: Nie znaleziono wykonywalnego pliku $CLI_BIN."
        echo "       Upewnij się, że vectorcraft-cli znajduje się w ~/bin/vectorcraft-cli."
    fi
fi

# 1. Wykrycie wersji Qt6 w systemie
read -r QT_MAJ QT_MIN QT_PAT < <(python3 - << 'PY'
import glob, re

def detect():
    for p in glob.glob("/usr/lib/qt6/plugins/imageformats/*.so") + glob.glob("/usr/lib/qt6/plugins/kf6/thumbcreator/*.so"):
        try:
            data = open(p, "rb").read()
            m = re.search(b"qt-project!\x00\x01([\x05-\x07])([\x00-\x20])([\x00-\x20])\xbf", data)
            if m:
                return m.group(1)[0], m.group(2)[0], m.group(3)[0]
        except Exception:
            pass
    return 6, 11, 1

maj, min_, pat = detect()
print(f"{maj} {min_} {pat}")
PY
)
echo "==> Wykryta wersja Qt6 w systemie: ${QT_MAJ}.${QT_MIN}.${QT_PAT}"

mkdir -p "$IMG_PLUGIN_DIR"

BUILD_FLAGS="-DQT_VER_MAJOR=${QT_MAJ} -DQT_VER_MINOR=${QT_MIN} -DQT_VER_PATCH=${QT_PAT}"

build_with_gpp() {
    local libdir="$1"
    g++ -shared -fPIC -O2 -std=c++20 ${BUILD_FLAGS} "$SCRIPT_DIR/src/kimg_cdr.cpp" \
        "$libdir/libQt6Gui.so.6" \
        "$libdir/libQt6Core.so.6" \
        -Wl,-rpath,/usr/lib \
        -o "$IMG_PLUGIN_DIR/kimg_cdr.so"
}

if [[ "${1:-}" == "--rebuild" ]] || [[ ! -f "$SCRIPT_DIR/prebuilt/kimg_cdr.so" ]]; then
    if command -v g++ >/dev/null 2>&1; then
        echo "==> Kompilacja wtyczki obrazu kimg_cdr.so przy użyciu lokalnego g++..."
        build_with_gpp "/usr/lib"
    elif command -v podman >/dev/null 2>&1; then
        IMAGE="localhost/vectorcraft-arch-builder:latest"
        if ! podman image exists "$IMAGE"; then
            IMAGE="docker.io/library/archlinux:latest"
        fi
        echo "==> Kompilacja wtyczki obrazu kimg_cdr.so w kontenerze Podman ($IMAGE)..."
        podman run --rm \
            -v "$SCRIPT_DIR/src:/src:ro,Z" \
            -v "$PLUGIN_BASE:/out:Z" \
            -v /usr/lib:/host-lib:ro \
            "$IMAGE" \
            bash -c "
                g++ -shared -fPIC -O2 -std=c++20 ${BUILD_FLAGS} /src/kimg_cdr.cpp \
                    /host-lib/libQt6Gui.so.6 /host-lib/libQt6Core.so.6 \
                    -Wl,-rpath,/usr/lib -o /out/imageformats/kimg_cdr.so
            "
    else
        echo "BŁĄD: Brak kompilatora g++ oraz podmana, a plik prebuilt/kimg_cdr.so nie istnieje."
        exit 1
    fi
else
    echo "==> Instalacja gotowej wtyczki z prebuilt/ i dopasowanie wersji Qt (${QT_MAJ}.${QT_MIN}.${QT_PAT})..."
    cp -f "$SCRIPT_DIR/prebuilt/kimg_cdr.so" "$IMG_PLUGIN_DIR/kimg_cdr.so"
    python3 - "$IMG_PLUGIN_DIR/kimg_cdr.so" "$QT_MAJ" "$QT_MIN" "$QT_PAT" << 'PY'
import sys, re

maj, min_, pat = int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
path = sys.argv[1]
data = bytearray(open(path, "rb").read())
m = re.search(b"qt-project!\x00\x01[\x05-\x07][\x00-\x20][\x00-\x20]\xbf", data)
if m:
    pos = m.start() + 13
    data[pos] = maj
    data[pos + 1] = min_
    data[pos + 2] = pat
open(path, "wb").write(data)
PY
    chmod 755 "$IMG_PLUGIN_DIR/kimg_cdr.so"
fi

# 2. Rejestracja typu MIME CorelDRAW (.cdr)
echo "==> Rejestracja typu MIME application/vnd.corel-draw..."
mkdir -p "$HOME/.local/share/mime/packages"
cp -f "$SCRIPT_DIR/share/mime/coreldraw-cdr.xml" "$HOME/.local/share/mime/packages/coreldraw-cdr.xml"
update-mime-database "$HOME/.local/share/mime" 2>/dev/null || true

# 3. Instalacja natywnego generatora miniaturek KDE Frameworks 6 (.thumbnailer)
echo "==> Instalacja ~/.local/share/thumbnailers/cdrthumbnail.thumbnailer..."
mkdir -p "$HOME/.local/share/thumbnailers"
sed "s|@CLI_BIN@|${CLI_BIN}|g" "$SCRIPT_DIR/share/thumbnailers/cdrthumbnail.thumbnailer" \
    > "$HOME/.local/share/thumbnailers/cdrthumbnail.thumbnailer"

# 4. Konfiguracja ścieżki wtyczek Qt6 (QT_PLUGIN_PATH)
echo "==> Konfiguracja ścieżek wtyczek Qt6 (QT_PLUGIN_PATH)..."
mkdir -p "$HOME/.config/plasma-workspace/env" "$HOME/.config/environment.d"
cat << EOF > "$HOME/.config/plasma-workspace/env/qt-plugins.sh"
export QT_PLUGIN_PATH="$PLUGIN_BASE:/usr/lib/qt6/plugins\${QT_PLUGIN_PATH:+:\$QT_PLUGIN_PATH}"
EOF
chmod +x "$HOME/.config/plasma-workspace/env/qt-plugins.sh"

cat << EOF > "$HOME/.config/environment.d/qt-plugins.conf"
QT_PLUGIN_PATH=$PLUGIN_BASE:/usr/lib/qt6/plugins
EOF

systemctl --user set-environment QT_PLUGIN_PATH="$PLUGIN_BASE:/usr/lib/qt6/plugins" 2>/dev/null || true
dbus-update-activation-environment --systemd QT_PLUGIN_PATH="$PLUGIN_BASE:/usr/lib/qt6/plugins" 2>/dev/null || true

# 5. Konfiguracja skrótów Dolphin oraz Gwenview
echo "==> Konfiguracja integracji z Dolphin oraz Gwenview..."
mkdir -p "$HOME/.local/share/applications"
if [[ -f /usr/share/applications/org.kde.dolphin.desktop ]]; then
    sed "s|^Exec=dolphin|Exec=env QT_PLUGIN_PATH=$PLUGIN_BASE:/usr/lib/qt6/plugins dolphin|g" \
        /usr/share/applications/org.kde.dolphin.desktop > "$HOME/.local/share/applications/org.kde.dolphin.desktop"
fi

if [[ -f /usr/share/applications/org.kde.gwenview.desktop ]]; then
    sed \
        -e "s|^Exec=gwenview|Exec=env QT_PLUGIN_PATH=$PLUGIN_BASE:/usr/lib/qt6/plugins gwenview|g" \
        -e "s|^MimeType=\(.*\)|MimeType=\1application/vnd.corel-draw;application/x-cdr;application/cdr;image/x-cdr;image/cdr;zz-application/zz-winassoc-cdr;|g" \
        /usr/share/applications/org.kde.gwenview.desktop > "$HOME/.local/share/applications/org.kde.gwenview.desktop"
fi

# 6. Włączenie wtyczki cdrthumbnail w ~/.config/dolphinrc
python3 - << 'PY'
import os

rc_path = os.path.expanduser("~/.config/dolphinrc")
if os.path.exists(rc_path):
    lines = open(rc_path, "r", encoding="utf-8", errors="ignore").read().splitlines()
else:
    lines = []

found_sec = False
found_key = False
for i, line in enumerate(lines):
    if line.strip() == "[PreviewSettings]":
        found_sec = True
    elif found_sec and line.strip().startswith("["):
        if not found_key:
            lines.insert(i, "Plugins=appimagethumbnail,audiothumbnail,cdrthumbnail,comicbookthumbnail,cursorthumbnail,directorythumbnail,djvuthumbnail,ebookthumbnail,exrthumbnail,ffmpegthumbs,fontthumbnail,imagethumbnail,jpegthumbnail,kraorathumbnail,opendocumentthumbnail,svgthumbnail,windowsexethumbnail,windowsimagethumbnail")
            found_key = True
        break
    elif found_sec and line.startswith("Plugins="):
        plugins = [p.strip() for p in line.split("=", 1)[1].split(",") if p.strip()]
        if "cdrthumbnail" not in plugins:
            plugins.append("cdrthumbnail")
        lines[i] = "Plugins=" + ",".join(plugins)
        found_key = True
        break

if not found_sec:
    lines.extend([
        "",
        "[PreviewSettings]",
        "Plugins=appimagethumbnail,audiothumbnail,cdrthumbnail,comicbookthumbnail,cursorthumbnail,directorythumbnail,djvuthumbnail,ebookthumbnail,exrthumbnail,ffmpegthumbs,fontthumbnail,imagethumbnail,jpegthumbnail,kraorathumbnail,opendocumentthumbnail,svgthumbnail,windowsexethumbnail,windowsimagethumbnail"
    ])
elif not found_key:
    lines.append("Plugins=appimagethumbnail,audiothumbnail,cdrthumbnail,comicbookthumbnail,cursorthumbnail,directorythumbnail,djvuthumbnail,ebookthumbnail,exrthumbnail,ffmpegthumbs,fontthumbnail,imagethumbnail,jpegthumbnail,kraorathumbnail,opendocumentthumbnail,svgthumbnail,windowsexethumbnail,windowsimagethumbnail")

open(rc_path, "w", encoding="utf-8").write("\n".join(lines) + "\n")
PY

update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
killall kioworker 2>/dev/null || true

echo "==> Gotowe! Podgląd plików .cdr (i .pdf) w KDE Plasma 6 (Dolphin + Gwenview) został zainstalowany stabilnie i bezpiecznie."
