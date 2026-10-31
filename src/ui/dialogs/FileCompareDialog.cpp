#include "FileCompareDialog.h"
#include "../FramelessWindowHelper.h"
#include "../../core/ItemRecord.h"
#include "../../core/TaskProgressService.h"
#include "../../meta/DuplicateDetectorService.h"
#include "../../meta/MetadataManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QFontDatabase>
#include <QLocale>
#include <QtConcurrent>

namespace QuarkMeta {

// =========================================================================
// FileCompareModel 实现
// =========================================================================

FileCompareModel::FileCompareModel(const QStringList& filePaths, QObject* parent)
    : QAbstractTableModel(parent) {
    m_items.reserve(filePaths.size());
    for (const QString& p : filePaths) {
        FileCompareItem item;
        item.path = QDir::toNativeSeparators(p);
        QFileInfo fi(p);
        item.folderPath = QDir::toNativeSeparators(fi.absolutePath());
        item.name = fi.fileName();
        item.hash = "计算中…";
        item.size = -1;
        item.ctime = 0;
        item.mtime = 0;
        item.isLoaded = false;
        item.hasError = false;
        m_items.push_back(item);
    }
}

int FileCompareModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_items.size());
}

int FileCompareModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant FileCompareModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return QVariant();
    }

    const auto& item = m_items[index.row()];
    int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
            case ColIndex:
                // 序号：展示为 1-based 序号
                return index.row() + 1;
            case ColPath:
                return item.folderPath;
            case ColName:
                return item.name;
            case ColHash:
                return item.hash;
            case ColSize: {
                if (!item.isLoaded) return "-";
                if (item.hasError || item.size < 0) return "-";
                double kb = item.size / 1024.0;
                return QLocale().toString(kb, 'f', 2) + " KB";
            }
            case ColCreatedDate: {
                if (!item.isLoaded || item.hasError || item.ctime <= 0) return "-";
                return QDateTime::fromMSecsSinceEpoch(item.ctime).toString("yyyy-MM-dd HH:mm:ss");
            }
            case ColModifiedDate: {
                if (!item.isLoaded || item.hasError || item.mtime <= 0) return "-";
                return QDateTime::fromMSecsSinceEpoch(item.mtime).toString("yyyy-MM-dd HH:mm:ss");
            }
            default:
                break;
        }
    } else if (role == Qt::ToolTipRole) {
        switch (col) {
            case ColPath:
                return item.folderPath;
            case ColName:
                return item.name;
            case ColHash:
                return item.hash;
            case ColSize: {
                if (item.isLoaded && !item.hasError && item.size >= 0) {
                    return QString("%L1 字节").arg(item.size);
                }
                break;
            }
            case ColCreatedDate: {
                if (item.isLoaded && !item.hasError && item.ctime > 0) {
                    return QDateTime::fromMSecsSinceEpoch(item.ctime).toString("yyyy-MM-dd HH:mm:ss");
                }
                break;
            }
            case ColModifiedDate: {
                if (item.isLoaded && !item.hasError && item.mtime > 0) {
                    return QDateTime::fromMSecsSinceEpoch(item.mtime).toString("yyyy-MM-dd HH:mm:ss");
                }
                break;
            }
            default:
                break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (col == ColIndex) {
            return static_cast<int>(Qt::AlignCenter);
        }
        if (col == ColSize) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role == Qt::UserRole) {
        // 用于代理模型精确数值排序
        switch (col) {
            case ColIndex:
                return index.row() + 1;
            case ColPath:
                return item.folderPath;
            case ColName:
                return item.name;
            case ColHash:
                return item.hash;
            case ColSize:
                return static_cast<qint64>(item.size);
            case ColCreatedDate:
                return static_cast<qint64>(item.ctime);
            case ColModifiedDate:
                return static_cast<qint64>(item.mtime);
            default:
                break;
        }
    }

    return QVariant();
}

QVariant FileCompareModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case ColIndex: return "序号";
            case ColPath: return "路径";
            case ColName: return "名称";
            case ColHash: return "哈希值 (SHA-256)";
            case ColSize: return "大小";
            case ColCreatedDate: return "创建日期";
            case ColModifiedDate: return "修改日期";
            default: break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void FileCompareModel::updateItemResult(int row, qint64 size, long long ctime, long long mtime, const QString& hash, bool hasError) {
    if (row < 0 || row >= static_cast<int>(m_items.size())) return;

    m_items[row].size = size;
    m_items[row].ctime = ctime;
    m_items[row].mtime = mtime;
    m_items[row].hash = hash;
    m_items[row].isLoaded = true;
    m_items[row].hasError = hasError;

    emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
}

// =========================================================================
// HashCompareDelegate 实现 (逐段标色)
// =========================================================================

HashCompareDelegate::HashCompareDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {
}

void HashCompareDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QString currentHash = index.data(Qt::DisplayRole).toString();

    // 状态文本（如"计算中…"、"文件不存在"、"无法读取"）
    if (currentHash.length() != 64) {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        painter->save();
        if (currentHash == "文件不存在" || currentHash == "无法读取") {
            opt.palette.setColor(QPalette::Text, QColor("#888888"));
            opt.palette.setColor(QPalette::WindowText, QColor("#888888"));
        } else {
            opt.palette.setColor(QPalette::Text, QColor("#AAAAAA"));
            opt.palette.setColor(QPalette::WindowText, QColor("#AAAAAA"));
        }
        QStyledItemDelegate::paint(painter, opt, index);
        painter->restore();
        return;
    }

    // 绘制基础单元格背景及选中态高亮
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.text.clear();
    QStyledItemDelegate::paint(painter, opt, index);

    // 获取当前视图显示的第一行的哈希值作为基准
    QString baseHash;
    const QAbstractItemModel* m = index.model();
    if (m && m->rowCount() > 0) {
        baseHash = m->index(0, FileCompareModel::ColHash).data(Qt::DisplayRole).toString();
    }

    // 设置等宽字体
    QFont monoFont("Consolas");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(opt.font.pointSize() > 0 ? opt.font.pointSize() : 9);

    painter->save();
    painter->setFont(monoFont);
    QFontMetrics fm(monoFont);
    int charWidth = fm.horizontalAdvance('0');

    QRect textRect = opt.rect.adjusted(8, 0, -8, 0);
    int xPos = textRect.left();
    int yPos = textRect.top() + (textRect.height() - fm.height()) / 2 + fm.ascent();

    QColor normalColor = (opt.state & QStyle::State_Selected) ? QColor("#FFFFFF") : QColor("#EEEEEE");
    QColor diffColor("#ff551c");

    // 若当前为第一行，或基准哈希尚未算好/格式不符合，则全串使用普通文字色
    bool canCompare = (index.row() > 0) && (baseHash.length() == 64);

    if (!canCompare) {
        painter->setPen(normalColor);
        painter->drawText(xPos, yPos, currentHash);
    } else {
        // 逐字符比较并按连续相同/不同片段合并渲染
        int len = currentHash.length();
        int segStart = 0;
        bool segIsDiff = (currentHash[0] != baseHash[0]);

        for (int i = 1; i <= len; ++i) {
            bool isDiff = (i < len) ? (currentHash[i] != baseHash[i]) : !segIsDiff;
            if (isDiff != segIsDiff || i == len) {
                QString segText = currentHash.mid(segStart, i - segStart);
                painter->setPen(segIsDiff ? diffColor : normalColor);
                painter->drawText(xPos, yPos, segText);
                xPos += charWidth * segText.length();

                segStart = i;
                segIsDiff = isDiff;
            }
        }
    }

    painter->restore();
}

// =========================================================================
// FileCompareDialog 实现
// =========================================================================

FileCompareDialog::FileCompareDialog(const QStringList& filePaths, QWidget* parent)
    : FramelessDialog("文件对比 - QuarkMeta", parent),
      m_filePaths(filePaths),
      m_cancelToken(std::make_shared<std::atomic<bool>>(false)) {

    // 仅保留关闭与最大化按钮
    setVisibleButtons(Close | Max);

    initUi();

    if (m_filePaths.size() < 2) {
        QTimer::singleShot(0, this, &QDialog::reject);
        return;
    }

    startBackgroundHashCalculation();
}

FileCompareDialog::~FileCompareDialog() {
    cancelCalculation();
}

void FileCompareDialog::initUi() {
    QWidget* contentArea = getContentArea();
    auto* layout = new QVBoxLayout(contentArea);
    layout->setContentsMargins(12, 6, 12, 12);
    layout->setSpacing(10);

    // 1. 表格与代理模型配置
    m_tableView = new QTableView(this);
    m_tableView->setObjectName("FileCompareTableView");
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setShowGrid(false);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->verticalHeader()->setDefaultSectionSize(28);

    m_model = new FileCompareModel(m_filePaths, this);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setSortRole(Qt::UserRole);

    m_tableView->setModel(m_proxyModel);

    // 点击表头重新排序时，刷新视图重新计算与第一行对比的标色
    connect(m_tableView->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, [this]() {
        m_tableView->viewport()->update();
    });

    m_hashDelegate = new HashCompareDelegate(this);
    m_tableView->setItemDelegateForColumn(FileCompareModel::ColHash, m_hashDelegate);

    QHeaderView* header = m_tableView->horizontalHeader();
    header->setObjectName("FileCompareHeaderView");
    header->setSortIndicatorShown(true);
    header->setStretchLastSection(false);

    // 列宽自适应策略
    header->setSectionResizeMode(FileCompareModel::ColIndex, QHeaderView::Fixed);
    header->setSectionResizeMode(FileCompareModel::ColPath, QHeaderView::Interactive);
    header->setSectionResizeMode(FileCompareModel::ColName, QHeaderView::Interactive);
    header->setSectionResizeMode(FileCompareModel::ColHash, QHeaderView::Interactive);
    header->setSectionResizeMode(FileCompareModel::ColSize, QHeaderView::Interactive);
    header->setSectionResizeMode(FileCompareModel::ColCreatedDate, QHeaderView::Interactive);
    header->setSectionResizeMode(FileCompareModel::ColModifiedDate, QHeaderView::Interactive);

    m_tableView->setColumnWidth(FileCompareModel::ColIndex, 50);
    m_tableView->setColumnWidth(FileCompareModel::ColPath, 180);
    m_tableView->setColumnWidth(FileCompareModel::ColName, 160);
    m_tableView->setColumnWidth(FileCompareModel::ColHash, 470); // 确保可完整容纳 64 个等宽字符
    m_tableView->setColumnWidth(FileCompareModel::ColSize, 110);
    m_tableView->setColumnWidth(FileCompareModel::ColCreatedDate, 150);
    m_tableView->setColumnWidth(FileCompareModel::ColModifiedDate, 150);

    layout->addWidget(m_tableView, 1);

    // 2. 底部进度与关闭按钮栏
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(4, 0, 4, 0);
    bottomLayout->setSpacing(12);

    m_progressLabel = new QLabel(QString("已完成 0 / %1").arg(m_filePaths.size()), this);
    m_progressLabel->setObjectName("FileCompareProgressLabel");
    bottomLayout->addWidget(m_progressLabel);
    bottomLayout->addStretch(1);

    m_closeButton = new QPushButton("关闭", this);
    m_closeButton->setObjectName("FileCompareCloseBtn");
    m_closeButton->setCursor(Qt::PointingHandCursor);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    bottomLayout->addWidget(m_closeButton);

    layout->addLayout(bottomLayout);

    // 3. 复用 FramelessWindowHelper 获得 8 向无边框拉伸与双击最大化
    QWidget* titleBar = m_container ? m_container->findChild<QWidget*>("FramelessTitleBar") : nullptr;
    m_framelessHelper = FramelessWindowHelper::apply(this, titleBar);

    resize(1240, 520);
    setMinimumSize(880, 360);
}

void FileCompareDialog::startBackgroundHashCalculation() {
    int total = m_filePaths.size();
    m_jobId = TaskProgressService::instance().createJob("文件对比", total);

    auto cancelToken = m_cancelToken;
    QStringList files = m_filePaths;
    QPointer<FileCompareDialog> weakThis(this);
    int jobId = m_jobId;

    (void)QtConcurrent::run([weakThis, files, cancelToken, jobId]() {
        for (int i = 0; i < files.size(); ++i) {
            if (cancelToken->load()) {
                break;
            }

            QString filePath = files[i];
            QFile file(filePath);

            bool exists = file.exists();
            bool canOpen = exists && file.open(QIODevice::ReadOnly);
            if (canOpen) {
                file.close();
            }

            // 同一次处理中调用底层读取物理属性与哈希
            qint64 size = -1;
            long long ctime = 0;
            long long mtime = 0;
            QString hash;
            bool hasError = false;

            if (!exists) {
                hash = "文件不存在";
                hasError = true;
            } else if (!canOpen) {
                hash = "无法读取";
                hasError = true;
            } else {
                // 复用 ItemRecord 底层同一实现读取大小与创建/修改时间
                ItemRecord rec = ItemRecord::create(filePath);
                size = rec.size;
                ctime = rec.ctime;
                mtime = rec.mtime;

                // 计算全量哈希（分块且支持即时取消）
                hash = DuplicateDetectorService::computeFullSha256(filePath, cancelToken.get());
                if (cancelToken->load()) {
                    break;
                }
                if (hash.isEmpty()) {
                    hash = "无法读取";
                    hasError = true;
                }
            }

            QMetaObject::invokeMethod(qApp, [weakThis, i, size, ctime, mtime, hash, hasError, jobId]() {
                if (!weakThis) return;
                weakThis->m_model->updateItemResult(i, size, ctime, mtime, hash, hasError);
                weakThis->m_completedCount++;
                int totalCount = weakThis->m_filePaths.size();
                weakThis->m_progressLabel->setText(QString("已完成 %1 / %2").arg(weakThis->m_completedCount).arg(totalCount));

                TaskProgressService::instance().updateProgress(
                    jobId, weakThis->m_completedCount, totalCount,
                    QString("对比进度: %1/%2").arg(weakThis->m_completedCount).arg(totalCount)
                );
            });
        }

        QMetaObject::invokeMethod(qApp, [weakThis, jobId, cancelToken]() {
            if (!cancelToken->load()) {
                TaskProgressService::instance().finishJob(jobId);
            }
        });
    });
}

void FileCompareDialog::cancelCalculation() {
    if (m_cancelToken) {
        m_cancelToken->store(true);
    }
    if (m_jobId > 0) {
        TaskProgressService::instance().cancelJob(m_jobId);
        m_jobId = 0;
    }
}

void FileCompareDialog::closeEvent(QCloseEvent* event) {
    cancelCalculation();
    FramelessDialog::closeEvent(event);
}

void FileCompareDialog::reject() {
    cancelCalculation();
    FramelessDialog::reject();
}

bool FileCompareDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return FramelessDialog::nativeEvent(eventType, message, result);
}

} // namespace QuarkMeta
