#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <QWidget>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>
#include <QString>
#include <QProcess>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QSplitter>
#include <QTextCursor>
#include <QTextBlock>

// ---------------------------------------------------------------------------
// AsmSyntaxHighlighter
// ---------------------------------------------------------------------------

class AsmSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit AsmSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    QVector<HighlightRule> m_rules;

    void setupRules();
};

// ---------------------------------------------------------------------------
// DisassemblerWidget
// ---------------------------------------------------------------------------

class DisassemblerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DisassemblerWidget(QWidget *parent = nullptr);

    bool    loadFile(const QString &filePath);
    QString getFilePath() const;

private slots:
    void onFunctionSelected(int row);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessReadyRead();

private:
    // ---- data ---------------------------------------------------------------
    struct FunctionEntry {
        QString name;
        int     lineNumber;
        QString address;
    };

    QString              m_filePath;
    QProcess            *m_process        = nullptr;
    QString              m_accumulatedOutput;
    QString              m_currentTool;

    QVector<FunctionEntry> m_functions;

    // ---- widgets ------------------------------------------------------------
    QListWidget         *m_functionList   = nullptr;
    QPlainTextEdit      *m_asmView        = nullptr;
    AsmSyntaxHighlighter*m_highlighter    = nullptr;
    QLabel              *m_statusLabel    = nullptr;
    QLabel              *m_toolLabel      = nullptr;
    QLabel              *m_fileLabel      = nullptr;

    // ---- helpers ------------------------------------------------------------
    void setupUI();
    void parseAndDisplay(const QString &output);
    bool tryDisassemble(const QString &tool, const QStringList &args);
};

#endif // DISASSEMBLER_H