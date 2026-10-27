#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#ifndef QT_VER_MAJOR
#define QT_VER_MAJOR 6
#endif
#ifndef QT_VER_MINOR
#define QT_VER_MINOR 11
#endif
#ifndef QT_VER_PATCH
#define QT_VER_PATCH 1
#endif

#define QT_VER_HEX ((QT_VER_MAJOR << 16) | (QT_VER_MINOR << 8) | QT_VER_PATCH)

class QObject;
class QEvent;
class QTimerEvent;
class QChildEvent;
class QMetaMethod;
class QVariant;
class QRect;
struct QMetaObject {
    enum Call { InvokeMetaMethod };
};

struct QArrayData {
    int32_t ref_;
    int32_t flags;
    long long alloc;
    static void deallocate(QArrayData *data, long long objectSize, long long alignment) noexcept;
};

class QByteArray {
public:
    QArrayData *d = nullptr;
    char *ptr = nullptr;
    long long size = 0;

    QByteArray(const char *str, long long n = -1);
    ~QByteArray() {
        if (d) {
            if (__sync_sub_and_fetch(&d->ref_, 1) == 0) {
                QArrayData::deallocate(d, 1, 8);
            }
        }
    }
};

class QIODevice {
public:
    bool isOpen() const;
    bool isReadable() const;
    long long peek(char *data, long long maxSize);
    long long read(char *data, long long maxSize);
};

class QImage {
    alignas(8) unsigned char opaque[32];
public:
    QImage();
    ~QImage();
    bool loadFromData(const unsigned char *buf, int len, const char *format = nullptr);
    bool isNull() const;
};

class QObject {
protected:
    void *d_ptr;
public:
    static const QMetaObject staticMetaObject;
    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **);
    virtual ~QObject();
    virtual bool event(QEvent *);
    virtual bool eventFilter(QObject *, QEvent *);
    virtual void timerEvent(QTimerEvent *);
    virtual void childEvent(QChildEvent *);
    virtual void customEvent(QEvent *);
    virtual void connectNotify(const QMetaMethod &);
    virtual void disconnectNotify(const QMetaMethod &);
};

class QImageIOHandler {
protected:
    void *d_ptr;
public:
    enum ImageOption { Size = 0 };
    QImageIOHandler();
    virtual ~QImageIOHandler();
    void setDevice(QIODevice *device);
    QIODevice *device() const;
    void setFormat(const QByteArray &format);
    void setFormat(const QByteArray &format) const;
    virtual bool canRead() const = 0;
    virtual bool read(QImage *image) = 0;
    virtual bool write(const QImage &image);
    virtual QVariant option(ImageOption option) const;
    virtual void setOption(ImageOption option, const QVariant &value);
    virtual bool supportsOption(ImageOption option) const;
    virtual bool jumpToNextImage();
    virtual bool jumpToImage(int imageNumber);
    virtual int loopCount() const;
    virtual int imageCount() const;
    virtual int nextImageDelay() const;
    virtual int currentImageNumber() const;
    virtual QRect currentImageRect() const;
};

class QImageIOPlugin : public QObject {
public:
    static const QMetaObject staticMetaObject;
    explicit QImageIOPlugin(QObject *parent = nullptr);
    const QMetaObject *metaObject() const override;
    void *qt_metacast(const char *) override;
    int qt_metacall(QMetaObject::Call, int, void **) override;
    ~QImageIOPlugin() override;
    virtual int capabilities(QIODevice *device, const QByteArray &format) const = 0;
    virtual QImageIOHandler *create(QIODevice *device, const QByteArray &format) const = 0;
};

namespace QtSharedPointer {
struct ExternalRefCountData {
    int32_t weakref;
    int32_t strongref;
    static ExternalRefCountData *getAndRef(const QObject *);
};
}

static bool isCdrHeader(QIODevice *dev) {
    if (!dev) return false;
    char hdr[12] = {0};
    long long n = dev->peek(hdr, 12);
    if (n >= 11 && std::memcmp(hdr, "RIFF", 4) == 0 &&
        (std::memcmp(hdr + 8, "CDR", 3) == 0 || std::memcmp(hdr + 8, "cdr", 3) == 0)) {
        return true;
    }
    if (n >= 4 && std::memcmp(hdr, "PK\x03\x04", 4) == 0) {
        return true;
    }
    if (n >= 4 && std::memcmp(hdr, "WL", 2) == 0) {
        return true;
    }
    return false;
}

static std::string findVectorCraftCli() {
    const char *home = std::getenv("HOME");
    if (home && *home) {
        std::string candidate = std::string(home) + "/bin/vectorcraft-cli";
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
    }
    if (access("/home/deck/bin/vectorcraft-cli", X_OK) == 0) {
        return "/home/deck/bin/vectorcraft-cli";
    }
    return "vectorcraft-cli";
}

class CdrHandler final : public QImageIOHandler {
public:
    CdrHandler() = default;
    ~CdrHandler() override = default;

    bool canRead() const override {
        if (isCdrHeader(device())) {
            setFormat(QByteArray("cdr", 3));
            return true;
        }
        return false;
    }

    bool read(QImage *image) override {
        QIODevice *dev = device();
        if (!dev || !image) return false;

        std::vector<char> raw;
        char chunk[65536];
        while (true) {
            long long r = dev->read(chunk, sizeof(chunk));
            if (r <= 0) break;
            raw.insert(raw.end(), chunk, chunk + r);
            if (raw.size() > (128 << 20)) return false;
        }
        if (raw.empty()) return false;

        static uint32_t seq = 0;
        uint32_t id = __sync_add_and_fetch(&seq, 1);
        char tmpIn[128], tmpOut[128];
        std::snprintf(tmpIn, sizeof(tmpIn), "/tmp/kimg-cdr-%d-%u.cdr", static_cast<int>(getpid()), id);
        std::snprintf(tmpOut, sizeof(tmpOut), "/tmp/kimg-cdr-%d-%u.png", static_cast<int>(getpid()), id);

        FILE *fin = std::fopen(tmpIn, "wb");
        if (!fin) return false;
        std::fwrite(raw.data(), 1, raw.size(), fin);
        std::fclose(fin);

        std::string cli = findVectorCraftCli();
        pid_t pid = fork();
        if (pid == 0) {
            int dn = open("/dev/null", O_WRONLY);
            if (dn >= 0) {
                dup2(dn, STDOUT_FILENO);
                dup2(dn, STDERR_FILENO);
                close(dn);
            }
            const char *argv[] = {cli.c_str(), "convert", tmpIn, tmpOut, nullptr};
            execvp(cli.c_str(), const_cast<char *const *>(argv));
            _exit(127);
        } else if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
        } else {
            unlink(tmpIn);
            return false;
        }
        unlink(tmpIn);

        FILE *fout = std::fopen(tmpOut, "rb");
        if (!fout) return false;
        std::fseek(fout, 0, SEEK_END);
        long len = std::ftell(fout);
        std::fseek(fout, 0, SEEK_SET);
        std::vector<unsigned char> png;
        if (len > 0 && len < (64 << 20)) {
            png.resize(static_cast<size_t>(len));
            if (std::fread(png.data(), 1, png.size(), fout) != png.size()) {
                png.clear();
            }
        }
        std::fclose(fout);
        unlink(tmpOut);

        if (png.empty()) return false;
        return image->loadFromData(png.data(), static_cast<int>(png.size()), "PNG") && !image->isNull();
    }
};

class CdrPlugin final : public QImageIOPlugin {
public:
    explicit CdrPlugin(QObject *parent = nullptr) : QImageIOPlugin(parent) {}
    ~CdrPlugin() override = default;

    int capabilities(QIODevice *device, const QByteArray &format) const override {
        if (format.size == 3 && format.ptr &&
            (std::memcmp(format.ptr, "cdr", 3) == 0 || std::memcmp(format.ptr, "CDR", 3) == 0)) {
            return 0x1; // CanRead
        }
        if (format.size == 0 && device && device->isOpen() && device->isReadable() && isCdrHeader(device)) {
            return 0x1; // CanRead
        }
        return 0;
    }

    QImageIOHandler *create(QIODevice *device, const QByteArray &format) const override {
        QImageIOHandler *handler = new CdrHandler();
        handler->setDevice(device);
        handler->setFormat(format);
        return handler;
    }
};

extern "C" const char qt_version_tag;
struct QtVersionSection {
    const char *tag;
    uint64_t version;
};
__attribute__((section(".qtversion"), used, aligned(16)))
static const QtVersionSection qt_version_section = { &qt_version_tag, QT_VER_HEX };

static constexpr unsigned char cbor_payload[] = {
    0x01, QT_VER_MAJOR, QT_VER_MINOR, QT_VER_PATCH,
    0xbf,                   // CBOR indefinite map
    0x02, 0x78, 0x31,
        'o','r','g','.','q','t','-','p','r','o','j','e','c','t','.','Q','t','.',
        'Q','I','m','a','g','e','I','O','H','a','n','d','l','e','r','F','a','c','t','o','r','y','I','n','t','e','r','f','a','c','e',
    0x03, 0x69, 'C','d','r','P','l','u','g','i','n',
    0x04, 0xa2,
        0x64, 'K','e','y','s', 0x81,
            0x63, 'c','d','r',
        0x69, 'M','i','m','e','T','y','p','e','s', 0x86,
            0x78, 0x1a, 'a','p','p','l','i','c','a','t','i','o','n','/','v','n','d','.','c','o','r','e','l','-','d','r','a','w',
            0x71, 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','c','d','r',
            0x6f, 'a','p','p','l','i','c','a','t','i','o','n','/','c','d','r',
            0x6b, 'i','m','a','g','e','/','x','-','c','d','r',
            0x69, 'i','m','a','g','e','/','c','d','r',
            0x78, 0x1e, 'z','z','-','a','p','p','l','i','c','a','t','i','o','n','/','z','z','-','w','i','n','a','s','s','o','c','-','c','d','r',
    0xff
};

struct __attribute__((packed)) QtMetadataNote {
    uint32_t namesz;
    uint32_t descsz;
    uint32_t type;
    char name[12];
    unsigned char payload[sizeof(cbor_payload)];
};

__attribute__((section(".note.qt.metadata"), used, aligned(8)))
static constexpr QtMetadataNote qt_metadata_note = []() constexpr {
    QtMetadataNote n{12, sizeof(cbor_payload), 0x74510001u, {'q','t','-','p','r','o','j','e','c','t','!','\0'}, {}};
    for (size_t i = 0; i < sizeof(cbor_payload); ++i) n.payload[i] = cbor_payload[i];
    return n;
}();

struct QPluginMetaData {
    const void *data;
    size_t size;
};

extern "C" __attribute__((visibility("default"))) QPluginMetaData qt_plugin_query_metadata_v2() {
    return {qt_metadata_note.payload, sizeof(qt_metadata_note.payload)};
}

extern "C" __attribute__((visibility("default"))) QObject *qt_plugin_instance() {
    static QObject *instance = nullptr;
    static QtSharedPointer::ExternalRefCountData *refData = nullptr;
    if (!instance) {
        instance = new CdrPlugin();
        refData = QtSharedPointer::ExternalRefCountData::getAndRef(instance);
    }
    return instance;
}
