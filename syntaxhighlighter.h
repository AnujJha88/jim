#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QRegularExpression>
#include <QColor>

// ── Shared enums / structs used by both SyntaxHighlighter and CodeEditor ─────

enum class Language {
    PlainText,
    CPP,
    Python,
    JavaScript,
    HTML,
    CSS,
    Rust,
    Go,
    JSON,
    YAML,
    Markdown,
    Solidity,
    Yul
};

struct ColorTheme {
    QString name;
    QColor background;
    QColor foreground;
    QColor lineNumberBg;
    QColor lineNumberFg;
    QColor currentLine;
    QColor selection;
    QColor keyword;
    QColor string;
    QColor comment;
    QColor number;
    QColor function;
};

// ── SyntaxHighlighter ─────────────────────────────────────────────────────────

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    SyntaxHighlighter(QTextDocument *parent = nullptr);
    void applyTheme(const ColorTheme &theme);
    void setLanguage(Language lang);
    void setAudioPulse(float intensity); // 0..1 for audio-reactive highlighting

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat tagFormat;
    QTextCharFormat attributeFormat;
    QTextCharFormat headingFormat;
    QTextCharFormat boldFormat;
    QTextCharFormat linkFormat;

    Language currentLanguage;
    float audioPulseIntensity = 0.f;

    static QRegularExpression multiLineCommentStart;
    static QRegularExpression multiLineCommentEnd;

    void setupRules();
    void setupCppRules();
    void setupPythonRules();
    void setupJavaScriptRules();
    void setupHtmlRules();
    void setupCssRules();
    void setupRustRules();
    void setupGoRules();
    void setupJsonRules();
    void setupYamlRules();
    void setupMarkdownRules();
    void setupSolidityRules();
    void setupYulRules();
};

#endif // SYNTAXHIGHLIGHTER_H
