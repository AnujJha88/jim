#include "command_palette.h"

#include <QAction>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QVBoxLayout>

static const QString kPaletteStyle =
    "QDialog { background:#1e1e2e; border:1px solid #45475a; border-radius:8px; }"
    "QLineEdit { background:#313244; color:#cdd6f4; border:none; border-radius:4px;"
    "            padding:8px 12px; font-family:Consolas,monospace; font-size:13px; }"
    "QListWidget { background:#1e1e2e; color:#cdd6f4; border:none;"
    "              font-family:Consolas,monospace; font-size:12px; outline:none; }"
    "QListWidget::item { padding:6px 12px; border-radius:4px; }"
    "QListWidget::item:selected { background:#313244; color:#89b4fa; }"
    "QListWidget::item:hover { background:#2a2a3e; }";

CommandPalette::CommandPalette(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Popup) {
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(560);
    setStyleSheet(kPaletteStyle);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText("Type a command...");
    searchBox->installEventFilter(this);
    root->addWidget(searchBox);

    resultList = new QListWidget(this);
    resultList->setMaximumHeight(340);
    resultList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    resultList->setFocusProxy(searchBox);
    root->addWidget(resultList);

    connect(searchBox, &QLineEdit::textChanged, this, &CommandPalette::filter);
    connect(resultList, &QListWidget::itemActivated, this,
            [this](QListWidgetItem *) { runSelected(); });
}

void CommandPalette::populate(const QList<QAction *> &actions) {
    allActions = actions;
    filter(QString());
    if (resultList->count() > 0) {
        resultList->setCurrentRow(0);
    }
}

void CommandPalette::filter(const QString &text) {
    resultList->clear();
    for (QAction *act : allActions) {
        QString label = act->text().remove('&').trimmed();
        if (label.isEmpty() || !act->isEnabled()) {
            continue;
        }
        if (text.isEmpty() || label.contains(text, Qt::CaseInsensitive)) {
            auto *item = new QListWidgetItem(resultList);
            QString sc = act->shortcut().toString(QKeySequence::NativeText);
            item->setText(label + (sc.isEmpty() ? "" : "   " + sc));
            item->setData(Qt::UserRole, QVariant::fromValue(act));
            resultList->addItem(item);
        }
    }
    if (resultList->count() > 0) {
        resultList->setCurrentRow(0);
    }
    int rows = qMin(resultList->count(), 12);
    resultList->setMaximumHeight(rows * 30 + 8);
    adjustSize();
}

void CommandPalette::runSelected() {
    QListWidgetItem *item = resultList->currentItem();
    if (!item) {
        return;
    }

    auto *act = item->data(Qt::UserRole).value<QAction *>();
    if (act && act->isEnabled()) {
        accept();
        act->trigger();
    }
}

void CommandPalette::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        runSelected();
        return;
    }
    if (event->key() == Qt::Key_Down) {
        int next = resultList->currentRow() + 1;
        if (next < resultList->count()) {
            resultList->setCurrentRow(next);
        }
        return;
    }
    if (event->key() == Qt::Key_Up) {
        int prev = resultList->currentRow() - 1;
        if (prev >= 0) {
            resultList->setCurrentRow(prev);
        }
        return;
    }
    QDialog::keyPressEvent(event);
}

bool CommandPalette::eventFilter(QObject *obj, QEvent *event) {
    if (obj == searchBox && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Down || ke->key() == Qt::Key_Up ||
            ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter ||
            ke->key() == Qt::Key_Escape) {
            keyPressEvent(ke);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}
