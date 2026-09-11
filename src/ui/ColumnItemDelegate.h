#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QLineEdit>
#include <QKeyEvent>
#include "UiHelper.h"
#include "ThumbnailDelegate.h"
#include "../core/ModelContract.h"

namespace QuarkMeta {

class ColumnItemDelegate : public QStyledItemDelegate {
public:
    explicit ColumnItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(option);
        FileNameLineEdit* editor = new FileNameLineEdit(parent);
        editor->setObjectName("ColumnItemEditor");
        bool isFolder = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool();
        editor->setIsFolder(isFolder);
        editor->installEventFilter(const_cast<ColumnItemDelegate*>(this));
        return editor;
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(index);
        QRect r = option.rect;
        r.adjust(32, 1, -22, -1);
        editor->setGeometry(r);
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        QString value = index.model()->data(index, Qt::EditRole).toString();
        FileNameLineEdit* lineEdit = qobject_cast<FileNameLineEdit*>(editor);
        if (lineEdit) {
            lineEdit->setText(value);
        }
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
        if (!lineEdit) return;
        QString newName = lineEdit->text().trimmed();
        if (!newName.isEmpty()) {
            model->setData(index, newName, Qt::EditRole);
        }
    }

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            QLineEdit* editor = qobject_cast<QLineEdit*>(obj);
            if (editor) {
                int key = keyEvent->key();
                if (key == Qt::Key_Up || key == Qt::Key_Down) {
                    keyEvent->accept();
                    return true;
                }
                if (key == Qt::Key_Left || key == Qt::Key_Right) {
                    if (editor->hasSelectedText()) {
                        if (key == Qt::Key_Left) {
                            editor->setCursorPosition(0);
                        } else {
                            QString val = editor->text();
                            int lastDot = val.lastIndexOf('.');
                            if (lastDot > 0) {
                                editor->setCursorPosition(lastDot);
                            } else {
                                editor->setCursorPosition(val.length());
                            }
                        }
                        editor->deselect();
                        keyEvent->accept();
                        return true;
                    }
                    return false;
                }
            }
        }
        return QStyledItemDelegate::eventFilter(obj, event);
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        Q_UNUSED(index);
        QSize sz = QStyledItemDelegate::sizeHint(option, index);
        sz.setHeight(32);
        return sz;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) return;

        bool selected = option.state & QStyle::State_Selected;
        bool hover = option.state & QStyle::State_MouseOver;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // 1. 绘制背景
        QColor bg;
        if (selected) {
            bg = QColor("#378ADD");
            bg.setAlphaF(0.2f);
        } else if (hover) {
            bg = QColor("#2A2D2E");
        } else {
            bg = QColor("#1E1E1E");
        }
        painter->setBrush(bg);
        painter->setPen(Qt::NoPen);
        painter->drawRect(option.rect);

        // 2. 绘制图标或缩略图 (精确定位 18x18px)
        QRect iconRect(option.rect.left() + 8, option.rect.top() + (option.rect.height() - 18) / 2, 18, 18);
        QVariant deco = index.data(Qt::DecorationRole);
        if (deco.canConvert<QIcon>()) {
            QIcon icon = deco.value<QIcon>();
            if (!icon.isNull()) {
                icon.paint(painter, iconRect, Qt::AlignCenter);
            }
        } else if (deco.canConvert<QPixmap>()) {
            QPixmap pix = deco.value<QPixmap>();
            if (!pix.isNull()) {
                painter->drawPixmap(iconRect, pix.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }

        // 3. 绘制文字 (预留右侧 22px 保证 chevron_right 箭头画廊排他)
        QString name = index.data(Qt::DisplayRole).toString();
        QRect textRect = option.rect.adjusted(32, 0, -22, 0);
        QColor textColor = selected ? QColor("#FFFFFF") : QColor("#EEEEEE");
        painter->setPen(textColor);
        painter->setFont(option.font);
        QString elidedText = option.fontMetrics.elidedText(name, Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);

        // 4. 如果是文件夹，最右侧绘制向右箭头 chevron_right；若是空文件夹可辅助描边
        bool isDir = index.data(TypeRole).toString() == "folder" || index.data(Qt::UserRole + 2).toBool();
        bool isEmpty = index.data(IsEmptyRole).toBool();

        if (isDir) {
            QRect arrowRect(option.rect.right() - 20, option.rect.top() + (option.rect.height() - 14) / 2, 14, 14);
            QColor arrowColor = selected ? QColor("#FFFFFF") : (isEmpty ? QColor("#41F2F2") : QColor("#888888"));
            UiHelper::getIcon("chevron_right", arrowColor, 14).paint(painter, arrowRect, Qt::AlignCenter);
        }

        painter->restore();
    }
};

} // namespace QuarkMeta
