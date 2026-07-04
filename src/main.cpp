#include "app/Application.h"
#include "fs/IconResolver.h"
#include "util/Logging.h"

#include <QCommandLineParser>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(FangPlugin)

namespace {

const QUrl kBundledMain(QStringLiteral("qrc:/Fang/ui/Main.qml"));

QString uiOverrideDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/fang/ui");
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    // Fang draws every control itself; pin the unstyled base so no platform
    // QQC2 style can inject foreign colors (docs/05 Appearance).
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QCoreApplication::setOrganizationName(QStringLiteral("KromaKobra"));
    QCoreApplication::setApplicationName(QStringLiteral("Fang"));
    QCoreApplication::setApplicationVersion(QStringLiteral(FANG_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("dev.kromakobra.Fang"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Fang — a lightweight, fully customizable file manager"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("path"), QStringLiteral("Folder(s) to open."),
                                 QStringLiteral("[path...]"));
    QCommandLineOption newWindowOpt(QStringLiteral("new-window"),
                                    QStringLiteral("Open a new window (always the case; "
                                                   "Fang is multi-process)."));
    QCommandLineOption selectOpt(QStringLiteral("select"),
                                 QStringLiteral("Open the containing folder and select <file>."),
                                 QStringLiteral("file"));
    parser.addOption(newWindowOpt);
    parser.addOption(selectOpt);
    parser.process(app);

    QStringList paths = parser.positionalArguments();
    if (parser.isSet(selectOpt)) {
        const QFileInfo sel(parser.value(selectOpt));
        paths.prepend(sel.absolutePath());
    }

    fang::Application fangApp;
    fangApp.setStartupPaths(paths);

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImageProvider(QStringLiteral("fang-icon"), new fang::IconImageProvider);

    // UI override resolution (docs/03 §1, docs/04): a user project at
    // ~/.config/fang/ui/main.qml replaces the bundled UI; a broken override
    // logs, surfaces a banner, and falls back — it never bricks Fang.
    const QString overrideMain = uiOverrideDir() + QStringLiteral("/main.qml");
    bool usingOverride = QFileInfo::exists(overrideMain);
    QUrl rootUrl = kBundledMain;
    if (usingOverride) {
        engine.addImportPath(uiOverrideDir());
        rootUrl = QUrl::fromLocalFile(overrideMain);
        qCInfo(fangUi) << "loading UI override" << overrideMain;
    }

    auto lastWarnings = std::make_shared<QString>();
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
                     [lastWarnings](const QList<QQmlError>& warnings) {
                         QStringList lines;
                         for (const QQmlError& w : warnings)
                             lines << w.toString();
                         *lastWarnings = lines.join(QLatin1Char('\n'));
                     });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [&engine, &fangApp, &usingOverride, lastWarnings](const QUrl& url) {
            if (usingOverride && url != kBundledMain) {
                qCWarning(fangUi) << "UI override failed to load, falling back to bundled UI:"
                                  << *lastWarnings;
                usingOverride = false;
                fangApp.setOverrideError(*lastWarnings);
                engine.load(kBundledMain);
            } else {
                qCCritical(fangUi) << "bundled UI failed to load:" << *lastWarnings;
                QCoreApplication::exit(1);
            }
        },
        Qt::QueuedConnection);

    engine.load(rootUrl);
    return app.exec();
}
