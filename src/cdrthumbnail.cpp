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
class QWidget;
class QEvent;
class QTimerEvent;
class QChildEvent;
class QMetaMethod;
struct QMetaObject {
    enum Call { InvokeMetaMethod };
};
class KPluginMetaData;
class QVariant;
template <typename T> class QList {};

struct QArrayData {
    int32_t ref_;
    int32_t flags;
    long long alloc;
    static void deallocate(QArrayData *data, long long objectSize, long long alignment) noexcept;
};

struct QString {
    QArrayData *d = nullptr;
    char16_t *ptr = nullptr;
    int64_t size = 0;

    ~QString() {
        if (d) {
            if (__sync_sub_and_fetch(&d->ref_, 1) == 0) {
                QArrayData::deallocate(d, 2, 8);
            }
        }
    }

    std::string toStdString() const {
        std::string out;
        if (!ptr || size <= 0) return out;
        out.reserve(size * 3);
        for (int64_t i = 0; i < size; ++i) {
            uint32_t cp = ptr[i];
            if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < size) {
                uint32_t low = ptr[i + 1];
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                    ++i;
                }
            }
            if (cp < 0x80) {
                out.push_back(static_cast<char>(cp));
            } else if (cp < 0x800) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        return out;
    }
};

class QUrl {
    void *d = nullptr;
public:
    ~QUrl();
    QString toLocalFile() const;
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

namespace KIO {
class ThumbnailRequest {
public:
    QUrl url() const;
};

class ThumbnailResult {
    void *d;
public:
    static ThumbnailResult pass(const QImage &image);
    static ThumbnailResult fail();
};

class ThumbnailCreator : public QObject {
    void *d;
public:
    static const QMetaObject staticMetaObject;
    ThumbnailCreator(QObject *parent, const QList<QVariant> &args);
    const QMetaObject *metaObject() const override;
    void *qt_metacast(const char *) override;
    int qt_metacall(QMetaObject::Call, int, void **) override;
    ~ThumbnailCreator() override;
    virtual ThumbnailResult create(const ThumbnailRequest &request) = 0;
};
} // namespace KIO

class KPluginFactory : public QObject {
    void *d;
public:
    using CreateInstanceFn = QObject *(*)(QWidget *, QObject *, const KPluginMetaData &, const QList<QVariant> &);
    static const QMetaObject staticMetaObject;
    KPluginFactory();
    const QMetaObject *metaObject() const override;
    void *qt_metacast(const char *) override;
    int qt_metacall(QMetaObject::Call, int, void **) override;
    ~KPluginFactory() override;
    virtual QObject *create(const char *iface, QWidget *parentWidget, QObject *parent, const QList<QVariant> &args);
    void registerPlugin(const QMetaObject *metaObject, CreateInstanceFn instanceFunction);
};

namespace QtSharedPointer {
struct ExternalRefCountData {
    int32_t weakref;
    int32_t strongref;
    static ExternalRefCountData *getAndRef(const QObject *);
};
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

class CdrCreator final : public KIO::ThumbnailCreator {
public:
    CdrCreator(QObject *parent, const QList<QVariant> &args)
        : KIO::ThumbnailCreator(parent, args) {}

    ~CdrCreator() override = default;

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override {
        QUrl u = request.url();
        QString qpath = u.toLocalFile();
        std::string path = qpath.toStdString();
        if (path.empty()) {
            return KIO::ThumbnailResult::fail();
        }

        static uint32_t seq = 0;
        uint32_t id = __sync_add_and_fetch(&seq, 1);
        char tmpPath[128];
        std::snprintf(tmpPath, sizeof(tmpPath), "/tmp/cdrthumb-%d-%u.png", static_cast<int>(getpid()), id);

        std::string cli = findVectorCraftCli();
        pid_t pid = fork();
        if (pid == 0) {
            int dn = open("/dev/null", O_WRONLY);
            if (dn >= 0) {
                dup2(dn, STDOUT_FILENO);
                dup2(dn, STDERR_FILENO);
                close(dn);
            }
            const char *argv[] = {cli.c_str(), "convert", path.c_str(), tmpPath, nullptr};
            execvp(cli.c_str(), const_cast<char *const *>(argv));
            _exit(127);
        } else if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);
        } else {
            return KIO::ThumbnailResult::fail();
        }

        FILE *f = std::fopen(tmpPath, "rb");
        if (!f) {
            return KIO::ThumbnailResult::fail();
        }
        std::fseek(f, 0, SEEK_END);
        long len = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> buf;
        if (len > 0 && len < (32 << 20)) {
            buf.resize(static_cast<size_t>(len));
            if (std::fread(buf.data(), 1, buf.size(), f) != buf.size()) {
                buf.clear();
            }
        }
        std::fclose(f);
        unlink(tmpPath);

        if (buf.empty()) {
            return KIO::ThumbnailResult::fail();
        }

        QImage img;
        if (img.loadFromData(buf.data(), static_cast<int>(buf.size()), "PNG") && !img.isNull()) {
            return KIO::ThumbnailResult::pass(img);
        }
        return KIO::ThumbnailResult::fail();
    }
};

static QObject *createCdrCreator(QWidget *, QObject *parent, const KPluginMetaData &, const QList<QVariant> &args) {
    return new CdrCreator(parent, args);
}

class CdrFactory final : public KPluginFactory {
public:
    CdrFactory() {
        registerPlugin(&KIO::ThumbnailCreator::staticMetaObject, &createCdrCreator);
    }
    ~CdrFactory() override = default;
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
    0x02, 0x76, 'o','r','g','.','k','d','e','.','K','P','l','u','g','i','n','F','a','c','t','o','r','y',
    0x03, 0x6a, 'C','d','r','F','a','c','t','o','r','y',
    0x04, 0xa2,             // UserData map (2 entries)
        0x6e, 'C','a','c','h','e','T','h','u','m','b','n','a','i','l', 0xf5, // true
        0x67, 'K','P','l','u','g','i','n', 0xa3, // KPlugin map (3 entries)
            0x69, 'M','i','m','e','T','y','p','e','s', 0x87, // array of 7 MIME types
                0x78, 0x1a, 'a','p','p','l','i','c','a','t','i','o','n','/','v','n','d','.','c','o','r','e','l','-','d','r','a','w',
                0x71, 'a','p','p','l','i','c','a','t','i','o','n','/','x','-','c','d','r',
                0x6f, 'a','p','p','l','i','c','a','t','i','o','n','/','c','d','r',
                0x6b, 'i','m','a','g','e','/','x','-','c','d','r',
                0x69, 'i','m','a','g','e','/','c','d','r',
                0x78, 0x1e, 'z','z','-','a','p','p','l','i','c','a','t','i','o','n','/','z','z','-','w','i','n','a','s','s','o','c','-','c','d','r',
                0x6f, 'a','p','p','l','i','c','a','t','i','o','n','/','p','d','f',
            0x64, 'N','a','m','e', 0x73, 'C','o','r','e','l','D','R','A','W',' ','D','o','c','u','m','e','n','t','s',
            0x68, 'N','a','m','e','[','p','l',']', 0x71, 'R','y','s','u','n','k','i',' ','C','o','r','e','l','D','R','A','W',
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
        instance = new CdrFactory();
        refData = QtSharedPointer::ExternalRefCountData::getAndRef(instance);
    }
    return instance;
}
