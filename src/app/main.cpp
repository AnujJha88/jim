#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include "texteditor.h"

int main(int argc, char *argv[]) {
    // Suppress Qt font warnings
    qputenv("QT_LOGGING_RULES", "qt.text.font.db=false");
    
    QApplication app(argc, argv);
    
    TextEditor editor;
    editor.show();
    
    // Handle command line arguments
    if (argc > 1) {
        QString arg = argv[1];
        QFileInfo fileInfo(arg);
        
        if (fileInfo.isDir()) {
            // Open folder in file tree
            editor.openFolderPath(arg);
        } else if (fileInfo.isFile()) {
            // Open file
            editor.openFilePath(arg);
        } else if (arg == ".") {
            // Open current directory
            editor.openFolderPath(QDir::currentPath());
        }
    }
    
    return app.exec();
}
