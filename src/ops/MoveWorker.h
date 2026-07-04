#pragma once

#include "ops/CopyWorker.h"

namespace fang {

/// Move = CopyWorker in move mode: same-device rename fast path per entry,
/// else copy + verify + delete-source (docs/03 §6).
class MoveWorker : public CopyWorker {
    Q_OBJECT

public:
    MoveWorker(const QStringList& sources, const QString& destDir,
               std::shared_ptr<OpControl> control);
};

} // namespace fang
