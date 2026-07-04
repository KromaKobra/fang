#include "MountMonitor.h"

#include "util/Logging.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDir>
#include <QFile>

#include <fcntl.h>
#include <unistd.h>

namespace fang {

namespace {

bool isPseudoFs(const QString& fsType, const QString& mountPoint)
{
    static const QStringList pseudo{
        QStringLiteral("proc"),   QStringLiteral("sysfs"),    QStringLiteral("devtmpfs"),
        QStringLiteral("devpts"), QStringLiteral("cgroup2"),  QStringLiteral("securityfs"),
        QStringLiteral("pstore"), QStringLiteral("bpf"),      QStringLiteral("configfs"),
        QStringLiteral("debugfs"), QStringLiteral("tracefs"), QStringLiteral("fusectl"),
        QStringLiteral("mqueue"), QStringLiteral("hugetlbfs"), QStringLiteral("binfmt_misc"),
        QStringLiteral("autofs"), QStringLiteral("efivarfs"), QStringLiteral("ramfs"),
        QStringLiteral("squashfs"),
    };
    if (pseudo.contains(fsType))
        return true;
    if (fsType == QLatin1String("tmpfs")
        && !mountPoint.startsWith(QLatin1String("/run/media")))
        return true;
    return false;
}

bool deviceIsRemovable(const QString& device, const QString& mountPoint)
{
    if (mountPoint.startsWith(QLatin1String("/run/media"))
        || mountPoint.startsWith(QLatin1String("/media")))
        return true;
    if (!device.startsWith(QLatin1String("/dev/")))
        return false;
    QString base = device.mid(5);
    while (!base.isEmpty() && base.back().isDigit())
        base.chop(1); // sda1 -> sda; nvme0n1p2 -> nvme0n1p (handled below)
    if (base.endsWith(QLatin1Char('p')) && device.contains(QLatin1String("nvme")))
        base.chop(1);
    QFile f(QStringLiteral("/sys/class/block/%1/removable").arg(base));
    if (f.open(QIODevice::ReadOnly))
        return f.readAll().trimmed() == "1";
    return false;
}

} // namespace

MountMonitor::MountMonitor(QObject* parent)
    : QObject(parent)
{
    m_mounts = enumerate();

    // /proc/self/mountinfo signals changes via POLLPRI/exception.
    m_mountinfoFd = ::open("/proc/self/mountinfo", O_RDONLY);
    if (m_mountinfoFd >= 0) {
        m_notifier = new QSocketNotifier(m_mountinfoFd, QSocketNotifier::Exception, this);
        connect(m_notifier, &QSocketNotifier::activated, this, [this] { refresh(); });
    } else {
        // Fallback: low-frequency polling.
        m_pollTimer.setInterval(5000);
        connect(&m_pollTimer, &QTimer::timeout, this, [this] { refresh(); });
        m_pollTimer.start();
    }

    const auto* dbusIface = QDBusConnection::systemBus().interface();
    m_udisksAvailable = dbusIface
        && dbusIface->isServiceRegistered(QStringLiteral("org.freedesktop.UDisks2"));
    qCDebug(fangPlaces) << "udisks2 available:" << m_udisksAvailable;
}

void MountMonitor::refresh()
{
    QList<Mount> fresh = enumerate();
    bool differs = fresh.size() != m_mounts.size();
    if (!differs) {
        for (int i = 0; i < fresh.size(); ++i) {
            if (fresh[i].mountPoint != m_mounts[i].mountPoint
                || fresh[i].device != m_mounts[i].device) {
                differs = true;
                break;
            }
        }
    }
    m_mounts = std::move(fresh);
    if (differs)
        emit mountsChanged();
}

QList<MountMonitor::Mount> MountMonitor::enumerate()
{
    QList<Mount> out;
    const QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo& si : volumes) {
        if (!si.isValid() || !si.isReady())
            continue;
        const QString fsType = QString::fromUtf8(si.fileSystemType());
        const QString mountPoint = si.rootPath();
        if (isPseudoFs(fsType, mountPoint))
            continue;
        if (mountPoint.startsWith(QLatin1String("/nix/store"))
            || mountPoint.startsWith(QLatin1String("/boot/efi")))
            continue;

        Mount m;
        m.device = QString::fromUtf8(si.device());
        m.mountPoint = mountPoint;
        m.fsType = fsType;
        m.bytesTotal = si.bytesTotal();
        m.bytesFree = si.bytesAvailable();
        m.label = si.displayName();
        if (m.label.isEmpty() || m.label == m.device)
            m.label = mountPoint == QLatin1String("/")
                ? QStringLiteral("File System")
                : QFileInfo(mountPoint).fileName();
        m.isRemovable = deviceIsRemovable(m.device, mountPoint);
        m.isOptical = m.device.startsWith(QLatin1String("/dev/sr"));
        out << m;
    }

    // gvfs network mounts (already-mounted paths appear as local dirs).
    const QString gvfsRoot = QStringLiteral("/run/user/%1/gvfs").arg(::getuid());
    const QFileInfoList gvfsMounts =
        QDir(gvfsRoot).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& fi : gvfsMounts) {
        Mount m;
        m.device = fi.fileName();
        m.mountPoint = fi.absoluteFilePath();
        m.fsType = QStringLiteral("gvfs");
        m.isNetwork = true;
        m.label = fi.fileName().section(QLatin1Char(':'), 0, 0);
        if (m.label.isEmpty())
            m.label = fi.fileName();
        out << m;
    }

    std::sort(out.begin(), out.end(),
              [](const Mount& a, const Mount& b) { return a.mountPoint < b.mountPoint; });
    return out;
}

QString MountMonitor::blockObjectPath(const QString& device) const
{
    if (!device.startsWith(QLatin1String("/dev/")))
        return {};
    return QStringLiteral("/org/freedesktop/UDisks2/block_devices/") + device.mid(5);
}

void MountMonitor::mount(const QString& device)
{
    if (!m_udisksAvailable)
        return;
    QDBusInterface fs(QStringLiteral("org.freedesktop.UDisks2"), blockObjectPath(device),
                      QStringLiteral("org.freedesktop.UDisks2.Filesystem"),
                      QDBusConnection::systemBus());
    fs.asyncCall(QStringLiteral("Mount"), QVariantMap());
}

void MountMonitor::unmount(const QString& device)
{
    if (!m_udisksAvailable)
        return;
    QDBusInterface fs(QStringLiteral("org.freedesktop.UDisks2"), blockObjectPath(device),
                      QStringLiteral("org.freedesktop.UDisks2.Filesystem"),
                      QDBusConnection::systemBus());
    fs.asyncCall(QStringLiteral("Unmount"), QVariantMap());
}

void MountMonitor::eject(const QString& device)
{
    if (!m_udisksAvailable)
        return;
    // Resolve the drive object for this block device, then eject it.
    QDBusInterface block(QStringLiteral("org.freedesktop.UDisks2"), blockObjectPath(device),
                         QStringLiteral("org.freedesktop.DBus.Properties"),
                         QDBusConnection::systemBus());
    const QDBusMessage reply = block.call(QStringLiteral("Get"),
                                          QStringLiteral("org.freedesktop.UDisks2.Block"),
                                          QStringLiteral("Drive"));
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty())
        return;
    const QString drivePath =
        reply.arguments().first().value<QDBusVariant>().variant().value<QDBusObjectPath>().path();
    if (drivePath.isEmpty() || drivePath == QLatin1String("/"))
        return;
    unmount(device);
    QDBusInterface drive(QStringLiteral("org.freedesktop.UDisks2"), drivePath,
                         QStringLiteral("org.freedesktop.UDisks2.Drive"),
                         QDBusConnection::systemBus());
    drive.asyncCall(QStringLiteral("Eject"), QVariantMap());
}

} // namespace fang
