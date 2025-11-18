# Makefile for Jim Text Editor

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -fPIC
MOC = /usr/lib/qt6/libexec/moc
TARGET = jim

# Qt6 paths for Debian/Kali
QT_INC = /usr/include/x86_64-linux-gnu/qt6
QT_LIB = /usr/lib/x86_64-linux-gnu

INCLUDES = -I$(QT_INC) -I$(QT_INC)/QtCore -I$(QT_INC)/QtGui -I$(QT_INC)/QtWidgets
LIBS = -L$(QT_LIB) -lQt6Core -lQt6Gui -lQt6Widgets

SOURCES = main.cpp texteditor.cpp linenumberarea.cpp
HEADERS = texteditor.h linenumberarea.h
MOC_SOURCES = moc_texteditor.cpp moc_linenumberarea.cpp
OBJECTS = $(SOURCES:.cpp=.o) $(MOC_SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

moc_texteditor.cpp: texteditor.h
	$(MOC) $(INCLUDES) $< -o $@

moc_linenumberarea.cpp: linenumberarea.h
	$(MOC) $(INCLUDES) $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

clean:
	rm -f $(OBJECTS) moc_texteditor.cpp moc_linenumberarea.cpp $(TARGET)

.PHONY: all clean install uninstall
