#include "MoveWorker.h"

namespace fang {

MoveWorker::MoveWorker(const QStringList& sources, const QString& destDir,
                       std::shared_ptr<OpControl> control)
    : CopyWorker(sources, destDir, std::move(control), /*moveMode=*/true)
{
}

} // namespace fang
