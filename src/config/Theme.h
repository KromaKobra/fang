#pragma once

#include <QColor>
#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace fang {

/// Active theme palette/metrics/typography exposed to QML as Application.theme.
/// All properties notify via the single changed() signal so a theme swap
/// recolors the whole UI instantly.
class Theme : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Theme is owned by Application")

    // Colors
    Q_PROPERTY(QColor background READ background NOTIFY changed)
    Q_PROPERTY(QColor backgroundAlt READ backgroundAlt NOTIFY changed)
    Q_PROPERTY(QColor surface READ surface NOTIFY changed)
    Q_PROPERTY(QColor surfaceHover READ surfaceHover NOTIFY changed)
    Q_PROPERTY(QColor surfaceActive READ surfaceActive NOTIFY changed)
    Q_PROPERTY(QColor foreground READ foreground NOTIFY changed)
    Q_PROPERTY(QColor foregroundMuted READ foregroundMuted NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(QColor border READ border NOTIFY changed)
    Q_PROPERTY(QColor selectionBackground READ selectionBackground NOTIFY changed)
    Q_PROPERTY(QColor selectionForeground READ selectionForeground NOTIFY changed)
    Q_PROPERTY(QColor error READ error NOTIFY changed)
    Q_PROPERTY(QColor warning READ warning NOTIFY changed)
    Q_PROPERTY(QColor success READ success NOTIFY changed)
    Q_PROPERTY(QColor shadow READ shadow NOTIFY changed)
    Q_PROPERTY(QColor scrollbar READ scrollbar NOTIFY changed)
    Q_PROPERTY(QColor scrollbarHover READ scrollbarHover NOTIFY changed)

    // Metrics
    Q_PROPERTY(qreal radiusSmall READ radiusSmall NOTIFY changed)
    Q_PROPERTY(qreal radiusMedium READ radiusMedium NOTIFY changed)
    Q_PROPERTY(qreal radiusLarge READ radiusLarge NOTIFY changed)
    Q_PROPERTY(qreal spacingSmall READ spacingSmall NOTIFY changed)
    Q_PROPERTY(qreal spacingMedium READ spacingMedium NOTIFY changed)
    Q_PROPERTY(qreal spacingLarge READ spacingLarge NOTIFY changed)
    Q_PROPERTY(int rowHeight READ rowHeight NOTIFY changed)
    Q_PROPERTY(int toolbarHeight READ toolbarHeight NOTIFY changed)
    Q_PROPERTY(int sidebarWidth READ sidebarWidth NOTIFY changed)
    Q_PROPERTY(int iconSizeSmall READ iconSizeSmall NOTIFY changed)
    Q_PROPERTY(int iconSizeMedium READ iconSizeMedium NOTIFY changed)
    Q_PROPERTY(int iconSizeLarge READ iconSizeLarge NOTIFY changed)

    // Typography
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY changed)
    Q_PROPERTY(QString fontFamilyMono READ fontFamilyMono NOTIFY changed)
    Q_PROPERTY(int fontSizeSmall READ fontSizeSmall NOTIFY changed)
    Q_PROPERTY(int fontSizeNormal READ fontSizeNormal NOTIFY changed)
    Q_PROPERTY(int fontSizeLarge READ fontSizeLarge NOTIFY changed)

    // State
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(bool isDark READ isDark NOTIFY changed)
    Q_PROPERTY(bool followSystem READ followSystem NOTIFY changed)

public:
    explicit Theme(QObject* parent = nullptr);

    /// Loads a theme by name: user themes dir first, then bundled qrc themes.
    /// Returns false (and keeps the current values) if nothing loadable is found.
    bool loadByName(const QString& name);
    /// Parses TOML theme content. Returns false on parse error (state unchanged).
    bool loadFromData(const QByteArray& data, const QString& fallbackName);

    void setFollowSystem(bool follow);
    /// Overrides typography from config ("auto" family / <=0 size keep theme values).
    void setFontOverride(const QString& family, int pointSize);

    /// Directory searched for user themes (default: $XDG_CONFIG_HOME/fang/themes).
    void setUserThemesDir(const QString& dir);

    QColor background() const { return m_colors.background; }
    QColor backgroundAlt() const { return m_colors.backgroundAlt; }
    QColor surface() const { return m_colors.surface; }
    QColor surfaceHover() const { return m_colors.surfaceHover; }
    QColor surfaceActive() const { return m_colors.surfaceActive; }
    QColor foreground() const { return m_colors.foreground; }
    QColor foregroundMuted() const { return m_colors.foregroundMuted; }
    QColor accent() const { return m_colors.accent; }
    QColor accentForeground() const { return m_colors.accentForeground; }
    QColor border() const { return m_colors.border; }
    QColor selectionBackground() const { return m_colors.selectionBackground; }
    QColor selectionForeground() const { return m_colors.selectionForeground; }
    QColor error() const { return m_colors.error; }
    QColor warning() const { return m_colors.warning; }
    QColor success() const { return m_colors.success; }
    QColor shadow() const { return m_colors.shadow; }
    QColor scrollbar() const { return m_colors.scrollbar; }
    QColor scrollbarHover() const { return m_colors.scrollbarHover; }

    qreal radiusSmall() const { return m_metrics.radiusSmall; }
    qreal radiusMedium() const { return m_metrics.radiusMedium; }
    qreal radiusLarge() const { return m_metrics.radiusLarge; }
    qreal spacingSmall() const { return m_metrics.spacingSmall; }
    qreal spacingMedium() const { return m_metrics.spacingMedium; }
    qreal spacingLarge() const { return m_metrics.spacingLarge; }
    int rowHeight() const { return m_metrics.rowHeight; }
    int toolbarHeight() const { return m_metrics.toolbarHeight; }
    int sidebarWidth() const { return m_metrics.sidebarWidth; }
    int iconSizeSmall() const { return m_metrics.iconSizeSmall; }
    int iconSizeMedium() const { return m_metrics.iconSizeMedium; }
    int iconSizeLarge() const { return m_metrics.iconSizeLarge; }

    QString fontFamily() const;
    QString fontFamilyMono() const { return m_typography.fontFamilyMono; }
    int fontSizeSmall() const;
    int fontSizeNormal() const;
    int fontSizeLarge() const;

    QString name() const { return m_name; }
    bool isDark() const { return m_isDark; }
    bool followSystem() const { return m_followSystem; }

signals:
    void changed();

private:
    struct Colors {
        QColor background{"#1e1e2e"};
        QColor backgroundAlt{"#181825"};
        QColor surface{"#252537"};
        QColor surfaceHover{"#313145"};
        QColor surfaceActive{"#3a3a52"};
        QColor foreground{"#cdd6f4"};
        QColor foregroundMuted{"#9399b2"};
        QColor accent{"#89b4fa"};
        QColor accentForeground{"#11111b"};
        QColor border{"#313244"};
        QColor selectionBackground{"#45475a"};
        QColor selectionForeground{"#cdd6f4"};
        QColor error{"#f38ba8"};
        QColor warning{"#f9e2af"};
        QColor success{"#a6e3a1"};
        QColor shadow{"#00000066"};
        QColor scrollbar{"#45475a"};
        QColor scrollbarHover{"#585b70"};
    };
    struct Metrics {
        qreal radiusSmall = 4;
        qreal radiusMedium = 8;
        qreal radiusLarge = 12;
        qreal spacingSmall = 4;
        qreal spacingMedium = 8;
        qreal spacingLarge = 16;
        int rowHeight = 28;
        int toolbarHeight = 40;
        int sidebarWidth = 220;
        int iconSizeSmall = 16;
        int iconSizeMedium = 24;
        int iconSizeLarge = 48;
    };
    struct Typography {
        QString fontFamily = QStringLiteral("Inter, sans-serif");
        QString fontFamilyMono = QStringLiteral("JetBrains Mono, monospace");
        int fontSizeSmall = 9;
        int fontSizeNormal = 10;
        int fontSizeLarge = 13;
    };

    Colors m_colors;
    Metrics m_metrics;
    Typography m_typography;
    QString m_name = QStringLiteral("dark");
    bool m_isDark = true;
    bool m_followSystem = false;
    QString m_userThemesDir;
    QString m_fontFamilyOverride; // empty or "auto" = use theme value
    int m_fontSizeOverride = 0;   // <= 0 = use theme value
};

} // namespace fang
