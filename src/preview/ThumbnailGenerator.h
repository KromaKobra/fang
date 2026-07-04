#pragma once

#include <QImage>
#include <QString>

#include <functional>

namespace fang {

/// freedesktop Thumbnail Managing Standard cache + pluggable generation
/// (docs/03 §10). All statics; XDG env is read at call time so tests can
/// redirect $XDG_CACHE_HOME. Thread-safe: called from the provider's pool.
class ThumbnailGenerator {
public:
    enum class SizeClass { Normal, Large }; // 128 px / 256 px

    static int pixels(SizeClass size) { return size == SizeClass::Normal ? 128 : 256; }
    static SizeClass classFor(const QSize& requested);

    /// $XDG_CACHE_HOME/thumbnails/{normal,large}
    static QString cacheDir(SizeClass size);
    /// <cacheDir>/<md5(file-uri)>.png
    static QString cachePathFor(const QString& absPath, SizeClass size);

    /// Cached thumbnail, validated via Thumb::URI/Thumb::MTime; null when
    /// missing or stale.
    static QImage loadCached(const QString& absPath, SizeClass size);
    /// Writes with the spec's PNG text keys, 0600 perms, atomic replace.
    static bool writeCache(const QString& absPath, SizeClass size, QImage image);

    /// Dispatches to the registered generator for the file's mime type;
    /// null for unsupported mimes or files above the size cap.
    static QImage generate(const QString& absPath, SizeClass size);
    /// loadCached, else generate + writeCache (shared-cache interop).
    static QImage getOrCreate(const QString& absPath, SizeClass size);

    /// `mimePattern` is a prefix glob like "image/*" or an exact mime name.
    /// Later registrations win so video/pdf generators can slot in (v2).
    using GenerateFn = std::function<QImage(const QString& absPath, int px)>;
    static void registerGenerator(const QString& mimePattern, GenerateFn fn);

    /// previews.max_file_size_mb; wired from Settings in stage 7.
    static void setMaxFileSizeBytes(qint64 bytes);
    static qint64 maxFileSizeBytes();
};

} // namespace fang
