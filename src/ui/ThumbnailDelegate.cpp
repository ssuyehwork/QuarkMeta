#include "ThumbnailDelegate.h"
#include "CardLayoutEngine.h"
#include "ContentPanel.h"
#include "CardPainterHelper.h"
#include "ElidedTextUtility.h"
#include "UiHelper.h"
#include "../core/ModelContract.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QFileInfo>
#include <QLineEdit>

namespace QuarkMeta {

ThumbnailDelegate::ThumbnailDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void ThumbnailDelegate::setHasThumbnailRole(int role) { m_hasThumbnailRole = role; }
void ThumbnailDelegate::setRatingRole(int role) { m_ratingRole = role; }
void ThumbnailDelegate::setPathRole(int role) { m_pathRole = role; }
void ThumbnailDelegate::setPinnedRole(int role) { m_pinnedRole = role; }
void ThumbnailDelegate::setTypeRole(int role) { m_typeRole = role; }
void ThumbnailDelegate::setIsEmptyRole(int role) { m_isEmptyRole = role; }
void ThumbnailDelegate::setColorRole(int role) { m_colorRole = role; }

ThumbnailDelegate::Metrics ThumbnailDelegate::calculateMetrics(const QStyleOptionViewItem& option) const {
    // 🚀【归一化对接】：直接从 CardLayoutEngine 提取数据
    CardLayout l = CardLayoutEngine::calculate(option.rect, option.decorationSize.width());
    Metrics m;
    m.cardRect = l.coverRect;
    m.textRect = l.textRect;
    m.banRect = l.banRect;
    m.ratingH = l.capsuleRect.height();
    m.ratingY = l.capsuleRect.top();
    m.starSize = l.starRects[0].width();
    m.starSpacing = (l.starRects[1].left() - l.starRects[0].right());
    m.starsStartX = l.starRects[0].left();
    return m;
}

void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) return;

    // 🚀【全要素归一化提取】
    CardLayout l = CardLayoutEngine::calculate(option.rect, option.decorationSize.width());
    bool isSelected = (option.state & QStyle::State_Selected);

    bool hasThumb = index.data(m_hasThumbnailRole).toBool();
    QVariant decoData = index.data(Qt::DecorationRole);
    QPixmap thumb;
    if (decoData.canConvert<QPixmap>()) {
        thumb = decoData.value<QPixmap>();
    } else if (decoData.canConvert<QIcon>()) {
        QIcon icon = decoData.value<QIcon>();
        if (!icon.isNull()) thumb = icon.pixmap(l.coverRect.size());
    }

    bool isWaitingThumb = false;
    if (m_pathRole != -1 && thumb.isNull()) {
        QString path = index.data(m_pathRole).toString();
        QString ext = QFileInfo(path).suffix().toLower();
        if (UiHelper::isGraphicsFile(ext) || ext == "svg") isWaitingThumb = true;
    }

    bool isGrid = option.widget ? option.widget->property("gridMode").toBool() : false;

    // ① 绘制缩略图 Cover
    CardPainterHelper::drawCardCover(painter, l.coverRect, isSelected, hasThumb, thumb,
                                     qvariant_cast<QIcon>(decoData), isGrid, isWaitingThumb);

    // ② 绘制卡片外边框
    CardPainterHelper::drawCardBorder(painter, l.coverRect, isSelected);

    // ③ 绘制置顶标记
    if (m_pinnedRole != -1) {
        bool isPinned = index.data(m_pinnedRole).toBool();
        CardPainterHelper::drawStatusIndicators(painter, l.coverRect, isPinned);
    }

    // ④ 绘制扩展名徽章
    if (m_pathRole != -1) {
        QString type = (m_typeRole != -1) ? index.data(m_typeRole).toString() : "";
        QString ext = (type == "folder") ? "DIR" : QFileInfo(index.data(m_pathRole).toString()).suffix().toUpper();
        if (ext.isEmpty()) ext = "FILE";
        CardPainterHelper::drawExtensionBadge(painter, l.coverRect, ext, hasThumb);
    }

    // ⑤ 绘制归一化色标与星级 (带 5px 呼吸间距)
    if (m_ratingRole != -1) {
        int rating = index.data(m_ratingRole).toInt();
        QString colorStr = (m_colorRole != -1) ? index.data(m_colorRole).toString() : "";

        CardPainterHelper::drawRatingStars(painter, l.banRect, l.coverRect,
                                          l.starRects[0].width(), 5,
                                          l.capsuleRect.top(), l.capsuleRect.height(), l.starRects[0].left(),
                                          rating, colorStr, isSelected);
    }

    // ⑥ 绘制文件名 (限制2行)
    drawFileNameText(painter, l.textRect, isSelected, index, option);

    // ⑦ 空文件夹虚线边框
    if (!isSelected && m_isEmptyRole != -1 && m_typeRole != -1) {
        if (index.data(m_typeRole).toString() == "folder" && index.data(m_isEmptyRole).toBool()) {
            CardPainterHelper::drawEmptyFolderBorder(painter, l.coverRect);
        }
    }
}

void ThumbnailDelegate::drawFileNameText(QPainter* painter, const QRect& textRect, bool isSelected, const QModelIndex& index, const QStyleOptionViewItem& option) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    QString name = index.data(Qt::DisplayRole).toString();
    painter->setPen(isSelected ? QColor("#3498db") : QColor("#EEEEEE"));

    QFont textFont = painter->font();
    textFont.setPointSize(8);
    painter->setFont(textFont);

    // 调用提取的静态排版工具方法，限制文件名最多2行
    QString displayName = ElidedTextUtility::elideTwoLinesText(name, option.fontMetrics, textRect.width() - 8);
    painter->drawText(textRect.adjusted(4, 0, -4, 0), Qt::AlignCenter | Qt::TextWordWrap, displayName);
    painter->restore();
}

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return QStyledItemDelegate::sizeHint(option, index);
}

QWidget* ThumbnailDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const { 
    FileNameLineEdit* editor = new FileNameLineEdit(parent); 
    editor->setStyleSheet( 
        "QLineEdit {" 
        "  background-color: #2D2D2D;" 
        "  color: #FFFFFF;" 
        "  selection-background-color: #3498db;" 
        "  border: 1px solid #3498db;" 
        "  border-radius: 4px;" 
        "  padding: 0px 4px;" 
        "  margin: 0px;" 
        "  font-size: 8pt;" 
        "}" 
    ); 
 
    bool isFolder = (index.data(m_typeRole).toString() == "folder"); 
    editor->setIsFolder(isFolder); 
    editor->installEventFilter(const_cast<ThumbnailDelegate*>(this)); 
    return editor; 
} 
 
void ThumbnailDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const {
    CardLayout l = CardLayoutEngine::calculate(option.rect, option.decorationSize.width());
    editor->setGeometry(l.textRect.adjusted(1, 4, -1, -4));
} 
 
void ThumbnailDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const { 
    QString value = index.model()->data(index, Qt::EditRole).toString(); 
    FileNameLineEdit* lineEdit = qobject_cast<FileNameLineEdit*>(editor);  
    if (lineEdit) { 
        lineEdit->setText(value); // 纯粹同步赋值，高亮交由 FileNameLineEdit::focusInEvent 完美同步接管 
    } 
} 

void ThumbnailDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit) return;

    QString newName = lineEdit->text().trimmed();
    if (newName.isEmpty()) return;

    // 🚀【方案 A 核心】：仅调用标准的 setData，没有任何 parent 向上引用的非标代码！
    model->setData(index, newName, Qt::EditRole);
}

bool ThumbnailDelegate::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(event); 
        QLineEdit* editor = qobject_cast<QLineEdit*>(obj); 
        if (editor) { 
            int key = keyEvent->key();
            if (key == Qt::Key_Up || key == Qt::Key_Down) {
                keyEvent->accept();
                return true; // 彻底吞噬，不让 View 漂移（对应用户原话：“用户按下向上/向下方向键时则不该向上游动选中上方/下方的项目”）
            }
            if (key == Qt::Key_Left || key == Qt::Key_Right) {
                if (editor->hasSelectedText()) {
                    // 全选高亮状态（对应用户原话：“如果用户按下向左/向右方向键，应该将光标定位到名称最前面或最后面，而不是'.'的后面，除非处于非全选状态”）
                    if (key == Qt::Key_Left) {
                        editor->setCursorPosition(0);
                    } else {
                        // 2026-07-26 极致重构：按下向右键光标一键定位到文件名基名（不含扩展名部分）的末端（点号前面）（对应用户原话：“我指的是文件名，不是后缀名...基名”）
                        QString val = editor->text();
                        int lastDot = val.lastIndexOf('.');
                        if (lastDot > 0) {
                            editor->setCursorPosition(lastDot);
                        } else {
                            editor->setCursorPosition(val.length());
                        }
                    }
                    editor->deselect(); // 清除全选高亮状态
                    keyEvent->accept();
                    return true; // 吞噬该事件，不让其触发默认定位
                }
                return false; // 非全选状态，走默认逐字位移
            }
        } 
    } 
    return QStyledItemDelegate::eventFilter(obj, event); 
} 

bool ThumbnailDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, 
                                const QStyleOptionViewItem& option, const QModelIndex& index) {
    Metrics m = calculateMetrics(option);
    QRect statusRect(m.cardRect.right() - 22, m.cardRect.top() + 8, 16, 16);

    return QStyledItemDelegate::helpEvent(event, view, option, index);
}

} // namespace QuarkMeta
