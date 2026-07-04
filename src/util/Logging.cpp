#include "Logging.h"

// Quiet by default; enable with e.g. QT_LOGGING_RULES="fang.*.debug=true".
Q_LOGGING_CATEGORY(fangFs, "fang.fs", QtWarningMsg)
Q_LOGGING_CATEGORY(fangOps, "fang.ops", QtWarningMsg)
Q_LOGGING_CATEGORY(fangUi, "fang.ui", QtWarningMsg)
Q_LOGGING_CATEGORY(fangSearch, "fang.search", QtWarningMsg)
Q_LOGGING_CATEGORY(fangThumbs, "fang.thumbs", QtWarningMsg)
Q_LOGGING_CATEGORY(fangConfig, "fang.config", QtWarningMsg)
Q_LOGGING_CATEGORY(fangPlaces, "fang.places", QtWarningMsg)
Q_LOGGING_CATEGORY(fangMime, "fang.mime", QtWarningMsg)
Q_LOGGING_CATEGORY(fangActions, "fang.actions", QtWarningMsg)
