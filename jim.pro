QT += core gui widgets
TARGET = jim
TEMPLATE = app
RC_ICONS = logo.ico
CONFIG += c++17

# Try to use QTermWidget if available
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += qtermwidget5
    DEFINES += USE_QTERMWIDGET
}

SOURCES += texteditor.cpp linenumberarea.cpp hexeditor.cpp main.cpp
HEADERS += texteditor.h linenumberarea.h hexeditor.h
