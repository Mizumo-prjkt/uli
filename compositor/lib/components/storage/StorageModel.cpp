#include "StorageModel.hpp"
#include <QtMath>

namespace uli {
namespace compositor {
namespace storage {

StorageModel::StorageModel(QObject *parent) : QObject(parent), m_totalSizeGb(500) {}

StorageModel::~StorageModel() {}

void StorageModel::setTotalSizeGb(int sizeGb) {
    if (sizeGb < 14) sizeGb = 14;
    m_totalSizeGb = sizeGb;
    emit modelChanged();
}

void StorageModel::setPartitions(const QList<PartitionConfig> &partitions) {
    m_partitions = partitions;
    emit modelChanged();
}

bool StorageModel::addPartition(const PartitionConfig &config) {
    PartitionConfig cfg = config;
    cfg.part_num = m_partitions.size() + 1;
    m_partitions.append(cfg);
    emit modelChanged();
    return true;
}

void StorageModel::removePartition(int index) {
    if (index >= 0 && index < m_partitions.size()) {
        m_partitions.removeAt(index);
        
        // Re-number partitions to avoid gaps
        for (int i = 0; i < m_partitions.size(); ++i) {
            m_partitions[i].part_num = i + 1;
        }
        
        emit modelChanged();
    }
}

void StorageModel::updatePartition(int index, const PartitionConfig &cfg) {
    if (index >= 0 && index < m_partitions.size()) {
        int originalNum = m_partitions[index].part_num;
        m_partitions[index] = cfg;
        m_partitions[index].part_num = originalNum; // Preserve numbering
        emit modelChanged();
    }
}

long long StorageModel::totalBytes() const {
    return static_cast<long long>(m_totalSizeGb) * 1024 * 1024 * 1024;
}

long long StorageModel::usedBytes() const {
    long long total = totalBytes();
    long long explicitUsed = 0;
    int remainderCount = 0;

    for (const auto &p : m_partitions) {
        QString clean = p.size_cmd.trimmed();
        if (clean.startsWith('+')) clean = clean.mid(1);

        if (clean == "0") {
            remainderCount++;
        } else {
            explicitUsed += parseSizeCmdToBytes(p.size_cmd, total);
        }
    }

    // Remainder partitions fill whatever is left
    long long remaining = (total > explicitUsed) ? (total - explicitUsed) : 0;
    return explicitUsed + remaining * (remainderCount > 0 ? 1 : 0);
}


long long StorageModel::remainingBytes() const {
    long long total = totalBytes();
    long long used = usedBytes();
    return (total > used) ? (total - used) : 0;
}

long long StorageModel::parseSizeCmdToBytes(const QString &cmd, long long totalCapacity) {
    if (cmd.isEmpty()) return 0;
    
    QString clean = cmd.trimmed();
    if (clean.startsWith('+')) clean = clean.mid(1);

    // "0" means "use remaining space"
    if (clean == "0") return totalCapacity;
    
    bool ok;
    long long val = 0;

    // Percentage: e.g. "50%"
    if (clean.endsWith('%')) {
        double pct = clean.left(clean.length() - 1).toDouble(&ok);
        if (ok) return static_cast<long long>((pct / 100.0) * totalCapacity);
    }

    // GiB / GB / G (binary giga)
    if (clean.endsWith("GiB", Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 3).toLongLong(&ok);
        if (ok) return val * 1024LL * 1024 * 1024;
    }
    if (clean.endsWith("GB", Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 2).toLongLong(&ok);
        if (ok) return val * 1024LL * 1024 * 1024;
    }
    if (clean.endsWith('G', Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 1).toLongLong(&ok);
        if (ok) return val * 1024LL * 1024 * 1024;
    }

    // MiB / MB / M (binary mega)
    if (clean.endsWith("MiB", Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 3).toLongLong(&ok);
        if (ok) return val * 1024LL * 1024;
    }
    if (clean.endsWith("MB", Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 2).toLongLong(&ok);
        if (ok) return val * 1024LL * 1024;
    }
    if (clean.endsWith('M', Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 1).toLongLong(&ok);
        if (ok) return val * 1024LL * 1024;
    }

    // KiB / KB / K
    if (clean.endsWith("KiB", Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 3).toLongLong(&ok);
        if (ok) return val * 1024LL;
    }
    if (clean.endsWith("KB", Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 2).toLongLong(&ok);
        if (ok) return val * 1024LL;
    }
    if (clean.endsWith('K', Qt::CaseInsensitive)) {
        val = clean.left(clean.length() - 1).toLongLong(&ok);
        if (ok) return val * 1024LL;
    }

    // Plain number (bytes)
    val = clean.toLongLong(&ok);
    if (ok) return val;
    
    return 0;
}


} // namespace storage
} // namespace compositor
} // namespace uli
