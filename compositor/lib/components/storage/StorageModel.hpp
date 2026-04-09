#ifndef ULI_COMPOSITOR_STORAGE_MODEL_HPP
#define ULI_COMPOSITOR_STORAGE_MODEL_HPP

#include <QString>
#include <QList>
#include <QObject>

namespace uli {
namespace compositor {
namespace storage {

struct PartitionConfig {
    QString device;
    QString fs_type;
    QString label;
    QString mount_point;
    QString mount_options;
    QString size_cmd;
    QString type_code;
    int part_num;
};

class StorageModel : public QObject {
    Q_OBJECT
public:
    explicit StorageModel(QObject *parent = nullptr);
    ~StorageModel();

    void setTotalSizeGb(int sizeGb);
    int totalSizeGb() const { return m_totalSizeGb; }

    void setPartitions(const QList<PartitionConfig> &partitions);
    bool addPartition(const PartitionConfig &config);
    void removePartition(int index);
    void updatePartition(int index, const PartitionConfig &cfg);
    const QList<PartitionConfig>& partitions() const { return m_partitions; }

    long long totalBytes() const;
    long long usedBytes() const;
    long long remainingBytes() const;

    static long long parseSizeCmdToBytes(const QString &cmd, long long totalCapacity);

signals:
    void modelChanged();

private:
    int m_totalSizeGb;
    QList<PartitionConfig> m_partitions;
};

} // namespace storage
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_STORAGE_MODEL_HPP
