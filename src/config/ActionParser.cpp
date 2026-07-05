#include "ActionParser.h"

#include "util/Logging.h"

#include <toml++/toml.hpp>

namespace fang {

namespace {

QString str(const toml::table& t, const char* key, const QString& fallback = {})
{
    if (auto v = t[key].value<std::string>())
        return QString::fromStdString(*v);
    return fallback;
}

QStringList strList(const toml::table& t, const char* key)
{
    QStringList out;
    if (const toml::array* arr = t[key].as_array()) {
        for (const toml::node& n : *arr)
            if (auto s = n.value<std::string>())
                out << QString::fromStdString(*s);
    }
    return out;
}

} // namespace

QList<ActionDef> ActionParser::parse(const QByteArray& data, QString* errorOut)
{
    toml::table root;
    try {
        root = toml::parse(std::string_view(data.constData(), size_t(data.size())));
    } catch (const toml::parse_error& e) {
        if (errorOut)
            *errorOut = QString::fromUtf8(e.what());
        return {};
    }

    QList<ActionDef> out;
    const toml::array* actions = root["action"].as_array();
    if (!actions)
        return out;

    for (const toml::node& node : *actions) {
        const toml::table* t = node.as_table();
        if (!t)
            continue;
        ActionDef def;
        def.id = str(*t, "id");
        def.label = str(*t, "label", def.id);
        def.iconName = str(*t, "icon");
        def.command = str(*t, "command");
        def.appliesTo = str(*t, "applies_to", QStringLiteral("files"));
        def.submenu = str(*t, "submenu");
        def.mimeTypes = strList(*t, "mime_types");
        def.extensions = strList(*t, "extensions");
        for (QString& ext : def.extensions)
            ext = ext.toLower();
        def.minSelection = int((*t)["min_selection"].value_or<int64_t>(0));
        def.maxSelection = int((*t)["max_selection"].value_or<int64_t>(0));
        def.showInMenu = (*t)["show_in_menu"].value_or<bool>(true);
        def.runInTerminal = (*t)["run_in_terminal"].value_or<bool>(false);
        if (def.id.isEmpty()) {
            qCWarning(fangActions) << "dropping [[action]] without id";
            continue;
        }
        out << def;
    }
    return out;
}

} // namespace fang
