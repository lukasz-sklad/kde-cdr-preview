# kde-cdr-preview — Natywny podgląd plików CorelDRAW (`.cdr`) w KDE Plasma 6 / SteamOS

Kompletny zestaw dla **KDE Plasma 6 (Qt6 / KDE Frameworks 6)** zapewniający stabilny i natywny podgląd oraz miniaturki dla plików **CorelDRAW (`.cdr`)** oraz **PDF (`.pdf`)** w systemie Linux / **SteamOS** — działający identycznie jak natywna obsługa plików `.xcf` (GIMP) czy `.psd` (Photoshop).

Do renderowania krzywych wektorowych wtyczki wykorzystują silnik CLI **`vectorcraft-cli`** (`~/bin/vectorcraft-cli`).

---

## Jak to działa?

1. **`cdrthumbnail.thumbnailer` (`~/.local/share/thumbnailers/cdrthumbnail.thumbnailer`)**
   - Natywny generator miniaturek zgodny ze standardem Freedesktop / KDE Frameworks 6 (`KIO::PreviewJob`).
   - Generuje miniaturki w osobnym procesie, jest w 100% bezpieczny (nie powoduje awarii procesu `kioworker`) i działa niezależnie od wersji bibliotek C++/Qt.
   - Odpowiada za miniaturki na żywo w menedżerze plików **Dolphin**.

2. **`kimg_cdr.so` (`~/.local/lib/qt6/plugins/imageformats/kimg_cdr.so`)**
   - Natywna wtyczka formatu obrazu **Qt6 `QImageIOPlugin`** (odpowiednik systemowego `kimg_xcf.so`).
   - Umożliwia silnikowi Qt6 (`QImage` / `QImageReader`) bezpośrednie otwieranie plików `.cdr` jako obrazów.
   - Pozwala na podgląd w **Gwenview** (*Otwórz za pomocą → Gwenview*), w panelu bocznym **Informacje (`F11`)** w Dolphinie oraz w systemowych oknach wyboru plików.

3. **`coreldraw-cdr.xml` (`~/.local/share/mime/packages/coreldraw-cdr.xml`)**
   - Rejestracja typów MIME i aliasów CorelDRAW w systemie (`application/vnd.corel-draw`, `application/x-cdr`, `image/x-cdr` itd.).

---

## Wymagania

- **KDE Plasma 6**
- Binarka **`vectorcraft-cli`** zainstalowana w `~/bin/vectorcraft-cli` (lub w `$PATH`)

---

## Szybka instalacja

```bash
git clone https://github.com/lukasz-sklad/kde-cdr-preview.git
cd kde-cdr-preview
./install.sh
```

W celu przebudowania wtyczki obrazu `kimg_cdr.so` ze źródeł C++ (przez lokalny kompilator lub kontener Podmana):

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
│   └── kimg_cdr.so                             # Skompilowana wtyczka Qt6 QImageIOPlugin (x86_64)
├── share/
│   ├── mime/
│   │   └── coreldraw-cdr.xml                   # Definicja MIME dla plików .cdr
│   └── thumbnailers/
│       └── cdrthumbnail.thumbnailer            # Generator miniaturek dla KDE Dolphin / KIO
└── src/
    └── kimg_cdr.cpp                            # Kod źródłowy wtyczki Qt6 imageformats (kimg_cdr.so)
```
