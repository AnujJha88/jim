#include "todo_panel.h"

#include "codeeditor.h"

#include <QColor>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextDocument>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

static const QString kTagColors[] = {
    "#f38ba8",
    "#fab387",
    "#f9e2af",
    "#89b4fa",
    "#ff5555",
};

static const QString kTags[] = {"TODO", "FIXME", "HACK", "NOTE", "BUG"};

TodoPanel::TodoPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setFixedHeight(32);
    header->setStyleSheet("background:#252526; border-bottom:1px solid #3c3c3c;");
    auto *hbox = new QHBoxLayout(header);
    hbox->setContentsMargins(8, 0, 8, 0);
    auto *title = new QLabel("TODO / FIXME", header);
    title->setStyleSheet(
        "color:#cccccc; font-family:Consolas; font-size:11px; font-weight:bold;");
    hbox->addWidget(title);
    hbox->addStretch();
    auto *refreshBtn = new QPushButton("↻", header);
    refreshBtn->setFixedSize(24, 24);
    refreshBtn->setStyleSheet(
        "QPushButton { background:transparent; color:#cccccc; border:none; "
        "font-size:14px; }"
        "QPushButton:hover { color:#ffffff; }");
    hbox->addWidget(refreshBtn);
    layout->addWidget(header);

    tree = new QTreeWidget(this);
    tree->setColumnCount(4);
    tree->setHeaderLabels({"Tag", "File", "Line", "Text"});
    tree->setStyleSheet(
        "QTreeWidget { background:#1e1e1e; color:#cccccc; border:none;"
        "              font-family:Consolas,monospace; font-size:11px; outline:none; }"
        "QTreeWidget::item { padding:3px 4px; }"
        "QTreeWidget::item:selected { background:#094771; }"
        "QTreeWidget::item:hover { background:#2a2d2e; }"
        "QHeaderView::section { background:#252526; color:#888; border:none;"
        "                       border-bottom:1px solid #3c3c3c; padding:4px; }");
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    tree->setRootIsDecorated(false);
    tree->setSortingEnabled(true);
    tree->sortByColumn(1, Qt::AscendingOrder);
    layout->addWidget(tree);

    connect(refreshBtn, &QPushButton::clicked, this,
            [this]() { emit jumpRequested(QString(), -1); });

    connect(tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item) {
                QString filePath = item->data(0, Qt::UserRole).toString();
                int line = item->data(1, Qt::UserRole).toInt();
                if (!filePath.isEmpty() && line >= 0) {
                    emit jumpRequested(filePath, line);
                }
            });

    setStyleSheet("background:#1e1e1e;");
}

void TodoPanel::scan(QTabWidget *tabs) {
    tree->clear();
    static QRegularExpression tagRe(
        R"(\b(TODO|FIXME|HACK|NOTE|BUG)\b[:\s]?\s*(.*))",
        QRegularExpression::CaseInsensitiveOption);

    for (int i = 0; i < tabs->count(); ++i) {
        CodeEditor *ed = qobject_cast<CodeEditor *>(tabs->widget(i));
        if (!ed) {
            continue;
        }

        QString filePath = ed->getFileName();
        QString displayName =
            filePath.isEmpty() ? tabs->tabText(i) : QFileInfo(filePath).fileName();
        QTextDocument *doc = ed->document();
        for (QTextBlock blk = doc->begin(); blk != doc->end(); blk = blk.next()) {
            QRegularExpressionMatch m = tagRe.match(blk.text());
            if (!m.hasMatch()) {
                continue;
            }

            QString tag = m.captured(1).toUpper();
            QString text = m.captured(2).trimmed();
            int lineNum = blk.blockNumber() + 1;

            auto *item = new QTreeWidgetItem(tree);
            item->setText(0, tag);
            item->setText(1, displayName);
            item->setText(2, QString::number(lineNum));
            item->setText(3, text);
            item->setData(0, Qt::UserRole, filePath);
            item->setData(1, Qt::UserRole, lineNum - 1);

            for (int t = 0; t < 5; ++t) {
                if (kTags[t] == tag) {
                    item->setForeground(0, QColor(kTagColors[t]));
                    break;
                }
            }
            item->setForeground(2, QColor("#888888"));
            item->setForeground(3, QColor("#a0a0a0"));
        }
    }
}
