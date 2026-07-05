#include "texteditor_private.h"

void TextEditor::openFolder() {
  QString folder =
      QFileDialog::getExistingDirectory(this, "Open Folder", QDir::homePath());
  if (!folder.isEmpty()) {
    currentFolder = folder;
    fileSystemModel->setRootPath(folder);
    fileTree->setRootIndex(fileSystemModel->index(folder));

    // Show file tree, hide empty state wrapper
    if (fileTreeContainer)
      fileTreeContainer->setCurrentIndex(1);

    // Update terminal working directory
    if (terminalWidget)
      terminalWidget->setWorkingDirectory(folder);

    statusBar()->showMessage("Opened folder: " + folder, 2000);
  }
}

void TextEditor::toggleFileTree() {
  if (fileTreeDock->isVisible())
    fileTreeDock->hide();
  else
    fileTreeDock->show();
}

void TextEditor::onFileTreeDoubleClicked(const QModelIndex &index) {
  QString filePath = fileSystemModel->filePath(index);
  QFileInfo fileInfo(filePath);
  if (fileInfo.isFile()) {
    for (int i = 0; i < tabWidget->count(); ++i) {
      CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
      if (editor && editor->getFileName() == filePath) {
        tabWidget->setCurrentIndex(i);
        return;
      }
    }
    loadFile(filePath);
  }
}

void TextEditor::customizeColors() {
  CodeEditor *editor = currentEditor();
  if (!editor)
    return;
  QColor bgColor =
      QColorDialog::getColor(Qt::white, this, "Choose Background Color");
  if (bgColor.isValid()) {
    QPalette p = editor->palette();
    p.setColor(QPalette::Base, bgColor);
    int brightness =
        (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) /
        1000;
    QColor textColor = brightness > 128 ? Qt::black : Qt::white;
    p.setColor(QPalette::Text, textColor);
    editor->setPalette(p);
    editor->update();
  }
}

void TextEditor::openFilePath(const QString &filePath) {
  QFileInfo fileInfo(filePath);
  if (fileInfo.exists() && fileInfo.isFile())
    loadFile(filePath);
}

void TextEditor::openFolderPath(const QString &folderPath) {
  QFileInfo fileInfo(folderPath);
  if (fileInfo.exists() && fileInfo.isDir()) {
    currentFolder = folderPath;
    fileTree->setRootIndex(fileSystemModel->index(folderPath));
    fileTreeDock->show();
    statusBar()->showMessage("Opened folder: " + folderPath, 2000);
  }
}

void TextEditor::toggleMiniMap() {
  for (int i = 0; i < tabWidget->count(); ++i) {
    CodeEditor *editor = qobject_cast<CodeEditor *>(tabWidget->widget(i));
    if (editor) {
      MiniMap *miniMap = editor->getMiniMap();
      if (miniMap) {
        if (miniMapAct->isChecked())
          miniMap->show();
        else
          miniMap->hide();
        QResizeEvent event(editor->size(), editor->size());
        QApplication::sendEvent(editor, &event);
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Markdown Preview
// ─────────────────────────────────────────────────────────────────────────────

void TextEditor::toggleMarkdownPreview()
{
    if (markdownPreview && markdownPreview->isVisible()) {
        // Hide: collapse and remove from splitter
        disconnectMarkdownPreview();
        markdownPreview->hide();
        markdownPreviewAct->setChecked(false);
        return;
    }

    // Only makes sense for text editors (Markdown or any file)
    CodeEditor *editor = currentEditor();
    if (!editor) {
        markdownPreviewAct->setChecked(false);
        return;
    }

    // Create preview widget on first use
    if (!markdownPreview) {
        markdownPreview = new MarkdownPreviewWidget();
        markdownPreview->setMinimumWidth(280);
    }

    // Set up debounce timer
    if (!markdownTimer) {
        markdownTimer = new QTimer(this);
        markdownTimer->setSingleShot(true);
        markdownTimer->setInterval(400);
        connect(markdownTimer, &QTimer::timeout, this, &TextEditor::updateMarkdownPreview);
    }

    // Add to the horizontal splitter alongside the tab widget
    mainSplitter->addWidget(markdownPreview);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(mainSplitter->count() - 1, 2);

    // Animate the preview in with a width animation
    markdownPreview->show();
    {
        auto *eff = new QGraphicsOpacityEffect(markdownPreview);
        markdownPreview->setGraphicsEffect(eff);
        auto *anim = new QPropertyAnimation(eff, "opacity", this);
        anim->setDuration(250);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    connectMarkdownPreview(editor);
    updateMarkdownPreview();
    markdownPreviewAct->setChecked(true);
}

void TextEditor::connectMarkdownPreview(CodeEditor *editor)
{
    // Disconnect previous editor if any
    disconnectMarkdownPreview();
    markdownEditor = editor;
    if (editor) {
        connect(editor->document(), &QTextDocument::contentsChanged,
                markdownTimer,       qOverload<>(&QTimer::start));
    }
}

void TextEditor::disconnectMarkdownPreview()
{
    if (markdownEditor) {
        disconnect(markdownEditor->document(), &QTextDocument::contentsChanged,
                   markdownTimer, qOverload<>(&QTimer::start));
        markdownEditor = nullptr;
    }
}

void TextEditor::updateMarkdownPreview()
{
    if (!markdownPreview || !markdownPreview->isVisible()) return;

    CodeEditor *editor = currentEditor();
    if (!editor) {
        markdownPreview->setContent("*No markdown file open.*");
        return;
    }

    // Re-connect if the tab changed
    if (editor != markdownEditor)
        connectMarkdownPreview(editor);

    QString baseDir;
    if (!editor->getFileName().isEmpty())
        baseDir = QFileInfo(editor->getFileName()).absolutePath();

    markdownPreview->setContent(editor->toPlainText(), baseDir);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Animation helpers
// ─────────────────────────────────────────────────────────────────────────────
