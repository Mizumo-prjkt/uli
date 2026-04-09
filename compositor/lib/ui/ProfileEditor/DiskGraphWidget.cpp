#include "DiskGraphWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <QHelpEvent>

namespace uli {
namespace compositor {
namespace ui {

DiskGraphWidget::DiskGraphWidget(storage::StorageModel *model, QWidget *parent) 
    : QWidget(parent), m_model(model) {
    setFixedHeight(80);
    setMouseTracking(true);
    connect(m_model, &storage::StorageModel::modelChanged, this, QOverload<>::of(&DiskGraphWidget::update));
}

void DiskGraphWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    int w = width();
    int h = height();
    long long total = m_model->totalBytes();
    if (total == 0) return;

    // Draw background (unallocated)
    painter.setBrush(QColor("#252532"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRect(0, 0, w, h), 8, 8);

    const auto &parts = m_model->partitions();
    if (parts.isEmpty()) {
        // Draw centered "Unallocated" text
        painter.setPen(QColor("#555570"));
        QFont freeFont;
        freeFont.setPixelSize(12);
        freeFont.setItalic(true);
        painter.setFont(freeFont);
        double totalGb = static_cast<double>(total) / (1024.0*1024*1024);
        painter.drawText(QRect(0, 0, w, h), Qt::AlignCenter,
            QString("Unallocated • %1 GB").arg(totalGb, 0, 'f', 1));
        return;
    }

    // Pre-calculate: sum of explicit (non-remainder) sizes
    long long explicitUsed = 0;
    int remainderCount = 0;
    for (const auto &p : parts) {
        QString clean = p.size_cmd.trimmed();
        if (clean.startsWith('+')) clean = clean.mid(1);
        if (clean == "0") {
            remainderCount++;
        } else {
            explicitUsed += storage::StorageModel::parseSizeCmdToBytes(p.size_cmd, total);
        }
    }
    long long remainderEach = 0;
    if (remainderCount > 0) {
        long long leftover = (total > explicitUsed) ? (total - explicitUsed) : 0;
        remainderEach = leftover / remainderCount;
    }

    int currentX = 0;
    for (int i = 0; i < parts.size(); ++i) {
        const auto &p = parts[i];

        // Determine actual byte size for this partition
        QString clean = p.size_cmd.trimmed();
        if (clean.startsWith('+')) clean = clean.mid(1);
        bool isRemainder = (clean == "0");
        long long sz = isRemainder ? remainderEach
                                   : storage::StorageModel::parseSizeCmdToBytes(p.size_cmd, total);
        if (sz <= 0) continue;

        int partW = static_cast<int>((static_cast<double>(sz) / total) * w);
        if (partW < 2) partW = 2;
        if (currentX + partW > w) partW = w - currentX;

        QRect pRect(currentX, 0, partW, h);
        
        QColor baseColor;
        if (p.fs_type == "vfat") baseColor = QColor("#00F2FE");
        else if (p.fs_type == "swap") baseColor = QColor("#667eea");
        else if (p.fs_type == "ext4") baseColor = QColor("#4FACFE");
        else if (p.fs_type == "btrfs") baseColor = QColor("#00cdac");
        else baseColor = QColor("#888899");

        QLinearGradient grad(pRect.topLeft(), pRect.bottomLeft());
        grad.setColorAt(0, baseColor);
        grad.setColorAt(1, baseColor.darker(160));

        // Round corners on first/last segments
        QPainterPath path;
        bool isFirst = (i == 0);
        bool isLast = (i == parts.size() - 1) && (remainderCount > 0 || explicitUsed >= total);
        if (isFirst && isLast) {
            path.addRoundedRect(pRect, 8, 8);
        } else if (isFirst) {
            path.addRoundedRect(pRect.adjusted(0, 0, 4, 0), 8, 8);
            path.addRect(pRect.adjusted(pRect.width() - 4, 0, 0, 0));
        } else if (isLast) {
            path.addRoundedRect(pRect.adjusted(-4, 0, 0, 0), 8, 8);
            path.addRect(pRect.adjusted(0, 0, -(pRect.width() - 4), 0));
        } else {
            path.addRect(pRect);
        }

        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawPath(path);

        // Draw inline label if segment is wide enough
        if (partW > 50) {
            painter.setPen(QColor(0, 0, 0, 200));
            QFont labelFont;
            labelFont.setPixelSize(11);
            labelFont.setBold(true);
            painter.setFont(labelFont);

            QString label = p.fs_type;
            if (!p.mount_point.isEmpty()) label = p.mount_point;
            painter.drawText(pRect.adjusted(6, 4, -4, -h/2), Qt::AlignLeft | Qt::AlignVCenter, label);

            QFont sizeFont;
            sizeFont.setPixelSize(10);
            painter.setFont(sizeFont);
            painter.setPen(QColor(0, 0, 0, 150));

            // Show computed size for remainder partitions
            QString sizeLabel = p.size_cmd;
            if (isRemainder) {
                double gb = static_cast<double>(sz) / (1024.0*1024*1024);
                sizeLabel = QString("~%1 GB (remainder)").arg(gb, 0, 'f', 1);
            }
            painter.drawText(pRect.adjusted(6, h/2 - 2, -4, -4), Qt::AlignLeft | Qt::AlignVCenter, sizeLabel);
        }

        // Separator line between partitions
        if (i < parts.size() - 1) {
            painter.setPen(QPen(QColor("#0F0F13"), 1));
            painter.drawLine(currentX + partW, 2, currentX + partW, h - 2);
        }

        currentX += partW;
    }

    // Draw 'Free' label in remaining space (only if no remainder partition)
    int freeW = w - currentX;
    if (freeW > 60 && remainderCount == 0) {
        painter.setPen(QColor("#555570"));
        QFont freeFont;
        freeFont.setPixelSize(11);
        freeFont.setItalic(true);
        painter.setFont(freeFont);
        QRect freeRect(currentX, 0, freeW, h);
        
        double freeGb = static_cast<double>(total - explicitUsed) / (1024.0*1024*1024);
        painter.drawText(freeRect, Qt::AlignCenter, 
            QString("Unallocated\n%1 GB").arg(freeGb, 0, 'f', 1));
    }
}



bool DiskGraphWidget::event(QEvent *event) {
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QString info = getPartitionInfoAt(helpEvent->x());
        if (!info.isEmpty()) {
            QToolTip::showText(helpEvent->globalPos(), info);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QWidget::event(event);
}

QString DiskGraphWidget::getPartitionInfoAt(int x) {
    int w = width();
    long long total = m_model->totalBytes();
    if (total == 0) return "";

    int currentX = 0;
    const auto &parts = m_model->partitions();
    for (const auto &p : parts) {
        long long sz = storage::StorageModel::parseSizeCmdToBytes(p.size_cmd, total);
        int partW = static_cast<int>((static_cast<double>(sz) / total) * w);
        if (partW == 0) partW = 1;

        if (x >= currentX && x <= currentX + partW) {
            return QString("<b>Partition %1</b><br/>Mount: %2<br/>FS: %3<br/>Size: %4")
                .arg(p.part_num)
                .arg(p.mount_point.isEmpty() ? "None" : p.mount_point)
                .arg(p.fs_type)
                .arg(p.size_cmd);
        }
        currentX += partW;
    }
    
    if (x >= currentX) return "<b>Unallocated Space</b>";

    return "";
}

} // namespace ui
} // namespace compositor
} // namespace uli
