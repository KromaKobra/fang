#include "ConfigStore.h"

#include "util/Logging.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>

#include <toml++/toml.hpp>

#include <sstream>

namespace fang {

namespace {

QVariant tomlToVariant(const toml::node& node)
{
    if (auto v = node.value<std::string>())
        return QString::fromStdString(*v);
    if (node.is_boolean())
        return *node.value<bool>();
    if (node.is_integer())
        return qlonglong(*node.value<int64_t>());
    if (node.is_floating_point())
        return *node.value<double>();
    if (const toml::array* arr = node.as_array()) {
        QVariantList list;
        for (const toml::node& e : *arr)
            list << tomlToVariant(e);
        return list;
    }
    if (const toml::table* tbl = node.as_table()) {
        QVariantMap map;
        for (const auto& [k, v] : *tbl)
            map.insert(QString::fromUtf8(k.str().data(), qsizetype(k.str().size())),
                       tomlToVariant(v));
        return map;
    }
    return {};
}

void insertVariant(toml::table& tbl, const std::string& key, const QVariant& value)
{
    switch (value.typeId()) {
    case QMetaType::Bool:
        tbl.insert_or_assign(key, value.toBool());
        return;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        tbl.insert_or_assign(key, int64_t(value.toLongLong()));
        return;
    case QMetaType::Double:
    case QMetaType::Float:
        tbl.insert_or_assign(key, value.toDouble());
        return;
    case QMetaType::QStringList:
    case QMetaType::QVariantList: {
        toml::array arr;
        const QVariantList list = value.toList();
        for (const QVariant& e : list) {
            if (e.typeId() == QMetaType::Bool)
                arr.push_back(e.toBool());
            else if (e.canConvert<qlonglong>() && e.typeId() != QMetaType::QString)
                arr.push_back(int64_t(e.toLongLong()));
            else
                arr.push_back(e.toString().toStdString());
        }
        tbl.insert_or_assign(key, std::move(arr));
        return;
    }
    case QMetaType::QVariantMap: {
        toml::table sub;
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            insertVariant(sub, it.key().toStdString(), it.value());
        tbl.insert_or_assign(key, std::move(sub));
        return;
    }
    default:
        tbl.insert_or_assign(key, value.toString().toStdString());
    }
}

} // namespace

ConfigStore::ConfigStore(QObject* parent)
    : QObject(parent)
    , m_root(std::make_unique<toml::table>())
{
}

ConfigStore::~ConfigStore() = default;

QString ConfigStore::configDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QLatin1String("/fang");
}

QString ConfigStore::stateDir()
{
    // Spec path is $XDG_STATE_HOME/fang (docs/06); StateLocation would
    // append the org/app names instead.
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation)
        + QLatin1String("/fang");
}

QString ConfigStore::configFilePath() const
{
    return configDir() + QLatin1String("/config.toml");
}

bool ConfigStore::load()
{
    *m_root = toml::table{};

    QFile f(configFilePath());
    if (!f.exists())
        return true; // defaults
    if (!f.open(QIODevice::ReadOnly)) {
        emit error(tr("Could not read config"), f.fileName());
        return false;
    }
    const QByteArray data = f.readAll();
    try {
        *m_root = toml::parse(std::string_view(data.constData(), size_t(data.size())));
    } catch (const toml::parse_error& e) {
        emit error(tr("Malformed config.toml — using defaults"),
                   QString::fromStdString(std::string(e.description())));
        return false;
    }
    return true;
}

bool ConfigStore::save()
{
    if (!m_root->contains("schema_version"))
        m_root->insert("schema_version", int64_t(1));

    QDir().mkpath(configDir());
    QSaveFile out(configFilePath());
    if (!out.open(QIODevice::WriteOnly)) {
        emit error(tr("Could not write config"), out.fileName());
        return false;
    }
    std::ostringstream ss;
    ss << *m_root;
    const std::string text = ss.str();
    out.write(text.data(), qint64(text.size()));
    if (!out.commit()) {
        emit error(tr("Could not write config"), out.fileName());
        return false;
    }
    return true;
}

QVariant ConfigStore::value(const QString& group, const QString& key,
                            const QVariant& fallback) const
{
    const toml::table* tbl = m_root->get_as<toml::table>(group.toStdString());
    if (!tbl)
        return fallback;
    const toml::node* node = tbl->get(key.toStdString());
    if (!node)
        return fallback;
    QVariant v = tomlToVariant(*node);
    return v.isValid() ? v : fallback;
}

void ConfigStore::setValue(const QString& group, const QString& key, const QVariant& newValue)
{
    if (value(group, key) == newValue)
        return;
    toml::table* tbl = m_root->get_as<toml::table>(group.toStdString());
    if (!tbl) {
        m_root->insert(group.toStdString(), toml::table{});
        tbl = m_root->get_as<toml::table>(group.toStdString());
    }
    insertVariant(*tbl, key.toStdString(), newValue);
    save();
    emit groupChanged(group);
}

QVariantMap ConfigStore::shortcutOverrides() const
{
    const toml::table* tbl = m_root->get_as<toml::table>("shortcuts");
    if (!tbl)
        return {};
    return tomlToVariant(*tbl).toMap();
}

} // namespace fang
