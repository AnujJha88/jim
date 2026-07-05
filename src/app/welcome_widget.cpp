#include "welcome_widget.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>
#include <QVBoxLayout>

WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void WelcomeWidget::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);

    QLabel *logo = new QLabel("Jim");
    logo->setStyleSheet(
        "font-size: 64px; font-weight: 300; color: #569cd6; letter-spacing: 8px; "
        "font-family: 'Segoe UI', 'Consolas', monospace;");
    logo->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logo);

    QLabel *subtitle = new QLabel("Lightweight Code Editor");
    subtitle->setStyleSheet("font-size: 16px; color: #808080; font-weight: 300; "
                            "letter-spacing: 2px; margin-bottom: 30px;");
    subtitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(subtitle);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(16);

    QString btnStyle =
        "QPushButton { background-color: #0e639c; color: #ffffff; border: none; "
        "padding: 12px 28px; border-radius: 6px; font-size: 14px; font-weight: "
        "500; min-width: 140px; } QPushButton:hover { background-color: #1177bb; "
        "} QPushButton:pressed { background-color: #094771; }";

    QPushButton *openFileBtn = new QPushButton("Open File");
    openFileBtn->setStyleSheet(btnStyle);
    connect(openFileBtn, &QPushButton::clicked, this,
            &WelcomeWidget::openFileRequested);
    buttonLayout->addWidget(openFileBtn);

    QPushButton *openFolderBtn = new QPushButton("Open Folder");
    openFolderBtn->setStyleSheet(btnStyle);
    connect(openFolderBtn, &QPushButton::clicked, this,
            &WelcomeWidget::openFolderRequested);
    buttonLayout->addWidget(openFolderBtn);

    mainLayout->addLayout(buttonLayout);

    QLabel *recentLabel = new QLabel("Recent Files");
    recentLabel->setStyleSheet("font-size: 13px; color: #cccccc; font-weight: "
                               "600; margin-top: 30px; letter-spacing: 1px;");
    recentLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(recentLabel);

    recentFilesLayout = new QVBoxLayout();
    recentFilesLayout->setAlignment(Qt::AlignCenter);
    recentFilesLayout->setSpacing(4);
    mainLayout->addLayout(recentFilesLayout);
    mainLayout->addStretch();
    setStyleSheet("QWidget { background-color: #1e1e1e; }");
}

void WelcomeWidget::setRecentFiles(const QStringList &files) {
    QLayoutItem *item;
    while ((item = recentFilesLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    int count = 0;
    for (const QString &file : files) {
        if (count >= 8) {
            break;
        }

        QPushButton *btn = new QPushButton(QFileInfo(file).fileName());
        btn->setToolTip(file);
        btn->setStyleSheet(
            "QPushButton { background-color: transparent; color: #3794ff; border: "
            "none; padding: 6px 16px; font-size: 13px; text-align: center; "
            "border-radius: 4px; min-width: 200px; } QPushButton:hover { "
            "background-color: #2a2d2e; color: #58b0ff; }");
        QString filePath = file;
        connect(btn, &QPushButton::clicked, this,
                [this, filePath]() { emit recentFileClicked(filePath); });
        recentFilesLayout->addWidget(btn);
        count++;
    }

    if (files.isEmpty()) {
        QLabel *noFiles = new QLabel("No recent files");
        noFiles->setStyleSheet("color: #555555; font-size: 12px; padding: 8px;");
        noFiles->setAlignment(Qt::AlignCenter);
        recentFilesLayout->addWidget(noFiles);
    }
}
