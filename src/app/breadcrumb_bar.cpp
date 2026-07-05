#include "breadcrumb_bar.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>

BreadcrumbBar::BreadcrumbBar(QWidget *parent) : QWidget(parent) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(8);

    iconLabel = new QLabel("\xF0\x9F\x93\x84");
    iconLabel->setStyleSheet("color: #999999; font-size: 14px;");
    layout->addWidget(iconLabel);

    pathLabel = new QLabel("");
    pathLabel->setStyleSheet(
        "color: #808080; font-size: 12px; font-family: 'Segoe UI', sans-serif;");
    layout->addWidget(pathLabel);

    fileLabel = new QLabel("");
    fileLabel->setStyleSheet(
        "color: #cccccc; font-size: 12px; font-family: 'Segoe UI', sans-serif; "
        "font-weight: 500;");
    layout->addWidget(fileLabel);

    symbolLabel = new QLabel("");
    symbolLabel->setStyleSheet(
        "color: #dcdcaa; font-size: 12px; font-family: 'Consolas', monospace;");
    layout->addWidget(symbolLabel);

    layout->addStretch();
    setFixedHeight(30);
    setStyleSheet("QWidget { background-color: #252526; border-bottom: 1px solid "
                  "#3e3e42; }");
}

void BreadcrumbBar::updatePath(const QString &filePath, const QString &symbol) {
    if (filePath.isEmpty()) {
        pathLabel->setText("");
        fileLabel->setText("Untitled");
        symbolLabel->setText("");
        return;
    }

    QFileInfo info(filePath);
    pathLabel->setText(info.absolutePath() + " > ");
    fileLabel->setText(info.fileName());

    if (!symbol.isEmpty()) {
        symbolLabel->setText(" > " + symbol);
        symbolLabel->show();
    } else {
        symbolLabel->setText("");
        symbolLabel->hide();
    }
}
