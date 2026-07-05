#include "title_bar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(0);

    QLabel *icon = new QLabel("J");
    icon->setStyleSheet("color: #569cd6; font-weight: bold; font-family: "
                        "Consolas; font-size: 14px;");
    layout->addWidget(icon);

    layout->addSpacing(10);

    titleLabel = new QLabel("Jim");
    titleLabel->setStyleSheet(
        "color: #cccccc; font-size: 12px; font-family: 'Segoe UI', sans-serif;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel, 1);

    QString btnStyle =
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #cccccc;"
        "    border: none;"
        "    width: 45px;"
        "    height: 30px;"
        "    font-family: 'Segoe MDL2 Assets', 'Segoe UI Symbol', sans-serif;"
        "    font-size: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3e3e42;"
        "}";

    QString closeBtnStyle =
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #cccccc;"
        "    border: none;"
        "    width: 45px;"
        "    height: 30px;"
        "    font-family: 'Segoe MDL2 Assets', 'Segoe UI Symbol', sans-serif;"
        "    font-size: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e81123;"
        "    color: white;"
        "}";

    QPushButton *minBtn =
        new QPushButton(QString::fromUtf8("\xE2\x80\x94"));
    minBtn->setStyleSheet(btnStyle);
    connect(minBtn, &QPushButton::clicked, this, [this]() {
        if (window()) {
            window()->showMinimized();
        }
    });
    layout->addWidget(minBtn);

    QPushButton *maxBtn =
        new QPushButton(QString::fromUtf8("\xE2\x96\xA1"));
    maxBtn->setStyleSheet(btnStyle);
    connect(maxBtn, &QPushButton::clicked, this, &TitleBar::toggleMaximized);
    layout->addWidget(maxBtn);

    QPushButton *closeBtn =
        new QPushButton(QString::fromUtf8("\xE2\x95\xB3"));
    closeBtn->setStyleSheet(closeBtnStyle);
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        if (window()) {
            window()->close();
        }
    });
    layout->addWidget(closeBtn);

    setFixedHeight(30);
    setStyleSheet("background-color: #323233;");
}

void TitleBar::setTitle(const QString &title) {
    titleLabel->setText(title);
}

void TitleBar::toggleMaximized() {
    if (!window()) {
        return;
    }
    if (window()->isMaximized()) {
        window()->showNormal();
        qobject_cast<QPushButton *>(sender())->setText(
            QString::fromUtf8("\xE2\x96\xA1"));
    } else {
        window()->showMaximized();
        qobject_cast<QPushButton *>(sender())->setText(
            QString::fromUtf8("\xE2\x9D\x90"));
    }
}

void TitleBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragStartPos =
            event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        if (window()->isMaximized()) {
            window()->showNormal();
            dragStartPos = QPoint(window()->width() / 2, height() / 2);
        }
        window()->move(event->globalPosition().toPoint() - dragStartPos);
        event->accept();
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        toggleMaximized();
        event->accept();
    }
}
