# kde-cdr-preview — Natywny podgląd plików CorelDRAW (`.cdr`) w KDE Plasma 6 / SteamOS

Kompletny zestaw wtyczek dla **KDE Plasma 6 (Qt6 / KDE Frameworks 6)** zapewniający pełny podgląd i miniaturki dla plików **CorelDRAW (`.cdr`)** oraz **PDF (`.pdf`)** w systemie Linux / **SteamOS** — działający identycznie jak natywna obsługa plików `.xcf` (GIMP) czy `.psd` (Photoshop).

Do renderowania krzywych wektorowych wtyczki wykorzystują szybki silnik CLI **`vectorcraft-cli`** (`~/bin/vectorcraft-cli`).

---

## Co daje ten pakiet?

1. **`kimg_cdr.so` (`~/.local/lib/qt6/plugins/imageformats/kimg_cdr.so`)**
   - Natywna wtyczka formatu obrazu **Qt6 `QImageIOPlugin`** (odpowiednik systemowego `kimg_xcf.so`).
   - Sprawia, że każda aplikacja Qt6/KDE potrafi otworzyć plik `.cdr` bezpośrednio jako `QImage` / `QImageReader`.
   - Pozwala przeglądać pliki `.cdr` bezpośrednio w przeglądarce obrazów **Gwenview** (*Otwórz za pomocą → Gwenview*), w panelu bocznym **Informacje (`F11`)** w Dolphinie oraz w oknach dialogowych *Otwórz plik*.

2. **`cdrthumbnail.thumbnailer` (`~/.local/share/thumbnailers/cdrthumbnail.thumbnailer`)**
   - Natywny wpis generatora miniaturek obsługiwany bezpośrednio przez `KIO::PreviewJob` w KDE Frameworks 6.
   - Działa bez konieczności modyfikowania partycji systemowej (read-only na SteamOS) i jest całkowicie odporny na aktualizacje wersji Qt6 podczas aktualizacji systemu SteamOS.

3. **`cdrthumbnail.so` (`~/.local/lib/qt6/plugins/kf6/thumbcreator/cdrthumbnail.so`)**
   - Natywna wtyczka C++ `KIO::ThumbnailCreator` dla procesów `kioworker`.

4. **`coreldraw-cdr.xml` (`~/.local/share/mime/packages/coreldraw-cdr.xml`)**
   - Definicja i aliasy typów MIME CorelDRAW (`application/vnd.corel-draw`, `application/x-cdr`, `image/x-cdr` itd.).

---

## Wymagania

- **KDE Plasma 6** (Qt 6.x + KDE Frameworks 6)
- Binarka **`vectorcraft-cli`** zainstalowana w `~/bin/vectorcraft-cli` (lub dostępna w `$PATH`)
- Do przebudowy ze źródeł (`./install.sh --rebuild`): lokalny `g++` lub `podman` (na SteamOS skrypt domyślnie instaluje gotowe binarki z katalogu `prebuilt/` i automatycznie dopasowuje nagłówek wersji Qt6 w sekcji ELF `.note.qt.metadata`).

---

## Szybka instalacja

```bash
git clone https://github.com/lukasz-sklad/kde-cdr-preview.git
cd kde-cdr-preview
./install.sh
```

Jeśli chcesz wymusić ponowną kompilację wtyczek C++ ze źródeł (np. przez kontener Podman na SteamOS):

```bash
./install.sh --rebuild
```

---

## Odinstalowanie

```bash
./uninstall.sh
```

---

## Struktura repozytorium

```text
kde-cdr-preview/
├── install.sh                                  # Skrypt instalacyjny (auto-detekcja wersji Qt6)
├── uninstall.sh                                # Skrypt deinstalacyjny
├── README.md                                   # Dokumentacja
├── prebuilt/
│   ├── kimg_cdr.so                             # Skompilowana wtyczka Qt6 QImageIOPlugin (x86_64)
│   └── cdrthumbnail.so                         # Skompilowana wtyczka KF6 KIO::ThumbnailCreator (x86_64)
├── share/
│   ├── mime/
│   │   └── coreldraw-cdr.xml                   # Definicja MIME dla plików .cdr
│   └── thumbnailers/
│       └── cdrthumbnail.thumbnailer            # Definicja generatora miniaturek dla KIO
└── src/
    ├── kimg_cdr.cpp                            # Kod źródłowy wtyczki Qt6 imageformats (kimg_cdr.so)
    └── cdrthumbnail.cpp                        # Kod źródłowy wtyczki KF6 thumbcreator (cdrthumbnail.so)
```
