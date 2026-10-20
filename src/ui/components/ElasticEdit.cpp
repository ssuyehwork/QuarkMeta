#include "ElasticEdit.h"
#include <QTextDocument>
#include <QKeyEvent>
#include <QtMath>
#include <QEvent>

namespace QuarkMeta {

ElasticEdit::ElasticEdit(QWidget* parent) : QTextEdit(parent) {
    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Minimum);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QTextEdit::WidgetWidth);
    QTextOption opt = document()->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document()->setDefaultTextOption(opt);
    document()->setDocumentMargin(2); // 留出 2px 文档内边距，保证字不贴顶且不切字
    connect(this, &QTextEdit::textChanged, this, &QWidget::updateGeometry);
}

bool ElasticEdit::hasHeightForWidth() const {
    return true;
}

int ElasticEdit::heightForWidth(int w) const {
    if (w <= 0) return 32;

    QTextDocument* doc = document();
    if (!doc) return 32;

    if (toPlainText().trimmed().isEmpty()) {
        return 32;
    }

    int chromeHeight = 10;
    int chromeWidth = 16 + 2;

    int availableTextW = qMax(10, w - chromeWidth);

    QTextDocument calcDoc;
    calcDoc.setDefaultFont(font());
    calcDoc.setDocumentMargin(2);
    calcDoc.setDefaultTextOption(doc->defaultTextOption());
    calcDoc.setPlainText(toPlainText());
    calcDoc.setTextWidth(availableTextW);

    int calculatedH = static_cast<int>(qCeil(calcDoc.size().height())) + chromeHeight;

    return qMax(32, calculatedH);
}

QSize ElasticEdit::sizeHint() const {
    int w = width() > 0 ? width() : 200;
    return QSize(w, heightForWidth(w));
}

QSize ElasticEdit::minimumSizeHint() const {
    int w = width() > 0 ? width() : 200;
    return QSize(w, heightForWidth(w));
}

void ElasticEdit::changeEvent(QEvent* e) {
    QTextEdit::changeEvent(e);
    if (e && (e->type() == QEvent::StyleChange || e->type() == QEvent::FontChange)) {
        updateGeometry();
    }
}

void ElasticEdit::keyPressEvent(QKeyEvent* e) {
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !(e->modifiers() & Qt::ShiftModifier)) {
        emit returnPressed();
        clearFocus();
        return;
    }
    QTextEdit::keyPressEvent(e);
}

} // namespace QuarkMeta