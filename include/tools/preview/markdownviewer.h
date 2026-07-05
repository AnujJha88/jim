#ifndef MARKDOWNVIEWER_H
#define MARKDOWNVIEWER_H

#include <QWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QString>
#include <QStringList>
#include <QTimer>

// ─────────────────────────────────────────────────────────────────────────────
//  MarkdownConverter
//  Pure static utility — converts a Markdown string to a self-contained HTML
//  document ready for display in QTextBrowser.  No external dependencies.
// ─────────────────────────────────────────────────────────────────────────────
class MarkdownConverter
{
public:
    // Convert markdown text to a full HTML document (with embedded CSS)
    static QString toHtml(const QString &markdown);

    // Just the CSS string — useful if you want to inject it separately
    static QString darkCss();

private:
    // Process inline markdown within a single text span (recursive)
    static QString processInline(const QString &text);

    // HTML-escape a plain string (no markdown processing)
    static QString escape(const QString &text);

    // Detect whether a line is a table separator (|---|:---:|...)
    static bool isTableSeparator(const QString &line);

    // Split a pipe-delimited table row into trimmed cell strings
    static QStringList splitTableRow(const QString &line);
};


// ─────────────────────────────────────────────────────────────────────────────
//  MarkdownPreviewWidget
//  A self-contained widget that renders Markdown via MarkdownConverter and
//  displays it in a QTextBrowser with the Jim dark theme applied.
// ─────────────────────────────────────────────────────────────────────────────
class MarkdownPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownPreviewWidget(QWidget *parent = nullptr);

    // Replace the displayed content.  baseDir is used to resolve relative
    // image paths (pass the directory that contains the .md file).
    void setContent(const QString &markdown,
                    const QString &baseDir = QString());

    // Convenience: set only the base directory (keeps current content)
    void setBaseDirectory(const QString &dir);

    // Returns the QTextBrowser so the caller can sync scroll positions etc.
    QTextBrowser *browser() const { return m_browser; }

signals:
    // Emitted when the user clicks a link inside the preview
    void linkActivated(const QString &url);

private slots:
    void onAnchorClicked(const QUrl &url);

private:
    QTextBrowser *m_browser    = nullptr;
    QLabel       *m_titleLabel = nullptr;
    QString       m_baseDir;
    QString       m_currentMarkdown;

    void setupUI();
};

#endif // MARKDOWNVIEWER_H