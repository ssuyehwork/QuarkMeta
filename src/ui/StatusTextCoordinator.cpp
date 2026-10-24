#include "StatusTextCoordinator.h"
#include <QTextDocument>

namespace QuarkMeta {

constexpr const char* kHighlightColorHex = "#ff551c";

StatusTextCoordinator::StatusTextCoordinator(QObject* parent)
    : QObject(parent) {
    m_finishTimer = new QTimer(this);
    m_finishTimer->setSingleShot(true);
    m_finishTimer->setInterval(3000);

    connect(m_finishTimer, &QTimer::timeout, this, [this]() {
        m_showingFinishMessage = false;
        reevaluate();
    });

    connect(&TaskProgressService::instance(), &TaskProgressService::snapshotChanged,
            this, &StatusTextCoordinator::onSnapshotChanged);
}

void StatusTextCoordinator::setNormalText(const QString& text) {
    m_normalText = text;
    reevaluate();
}

void StatusTextCoordinator::onSnapshotChanged(const TaskProgressSnapshot& snapshot) {
    if (snapshot.finished && !snapshot.finishSegments.isEmpty()) {
        m_finishSegments = snapshot.finishSegments;
        m_showingFinishMessage = true;
        m_finishTimer->start();
    }

    if (snapshot.active) {
        m_activeSegments = snapshot.statusSegments;
        m_hasActiveTask = true;
    } else {
        m_activeSegments.clear();
        m_hasActiveTask = false;
    }

    reevaluate();
}

QString StatusTextCoordinator::formatSegmentsToHtml(const QList<TextSegment>& segments) {
    QString html;
    for (const auto& seg : segments) {
        QString escaped = seg.text.toHtmlEscaped();
        if (seg.highlight) {
            html += QString("<span style=\"color:%1; font-weight:bold;\">%2</span>").arg(kHighlightColorHex, escaped);
        } else {
            html += escaped;
        }
    }
    return html;
}

void StatusTextCoordinator::reevaluate() {
    // 优先级 1：完成提示（显示 3 秒）
    if (m_showingFinishMessage) {
        emit finalTextChanged(formatSegmentsToHtml(m_finishSegments), true);
        return;
    }

    // 优先级 2：任务进行中状态
    if (m_hasActiveTask) {
        emit finalTextChanged(formatSegmentsToHtml(m_activeSegments), true);
        return;
    }

    // 优先级 3：常规文字
    emit finalTextChanged(m_normalText, false);
}

} // namespace QuarkMeta
