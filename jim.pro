QT += core gui widgets network multimedia
TARGET = jim
TEMPLATE = app
RC_ICONS = logo.ico
CONFIG += c++17

# Optionally link QTermWidget for a richer terminal experience on Unix
unix {
    CONFIG += link_pkgconfig
    packagesExist(qtermwidget5) {
        PKGCONFIG += qtermwidget5
        DEFINES += USE_QTERMWIDGET
        message("qtermwidget5 found — enabling enhanced terminal")
    } else {
        message("qtermwidget5 not found — using built-in QProcess terminal")
    }
}

win32 {
    LIBS += -lole32 -luuid
}

SOURCES += texteditor.cpp linenumberarea.cpp hexeditor.cpp main.cpp \
           aiautocomplete.cpp aisettingsdialog.cpp \
           disassembler.cpp binaryinspector.cpp \
           markdownviewer.cpp audiomonitor.cpp
HEADERS += texteditor.h linenumberarea.h hexeditor.h \
           aiautocomplete.h aisettingsdialog.h \
           disassembler.h binaryinspector.h \
           markdownviewer.h audiomonitor.h

