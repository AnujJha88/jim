#include "syntaxhighlighter.h"
#include <QFont>

QRegularExpression SyntaxHighlighter::multiLineCommentStart = QRegularExpression("/\\*");
QRegularExpression SyntaxHighlighter::multiLineCommentEnd   = QRegularExpression("\\*/");

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent), currentLanguage(Language::CPP) {
    setupRules();
}

void SyntaxHighlighter::setLanguage(Language lang) {
    currentLanguage = lang;
    setupRules();
    rehighlight();
}

void SyntaxHighlighter::setupRules() {
    highlightingRules.clear();
    switch (currentLanguage) {
    case Language::CPP:        setupCppRules();        break;
    case Language::Python:     setupPythonRules();     break;
    case Language::JavaScript: setupJavaScriptRules(); break;
    case Language::HTML:       setupHtmlRules();       break;
    case Language::CSS:        setupCssRules();        break;
    case Language::Rust:       setupRustRules();       break;
    case Language::Go:         setupGoRules();         break;
    case Language::JSON:       setupJsonRules();       break;
    case Language::YAML:       setupYamlRules();       break;
    case Language::Markdown:   setupMarkdownRules();   break;
    default:                   setupCppRules();        break;
    }
}

// ── C / C++ ───────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupCppRules() {
    HighlightingRule rule;

    // Preprocessor directives
    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(keywordFormat.foreground());
    preprocessorFormat.setFontWeight(QFont::Bold);
    QStringList preprocessors = {
        "#include","#define","#pragma","#if","#ifdef","#ifndef",
        "#elif","#else","#endif","#error","#warning","#undef","#line","#using"};
    for (const QString &p : preprocessors) {
        rule.pattern = QRegularExpression(p + "\\b");
        rule.format  = preprocessorFormat;
        highlightingRules.append(rule);
    }

    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {
        "\\balignas\\b","\\balignof\\b","\\band\\b","\\band_eq\\b","\\basm\\b",
        "\\bauto\\b","\\bbitand\\b","\\bbitor\\b","\\bbool\\b","\\bbreak\\b",
        "\\bcase\\b","\\bcatch\\b","\\bchar\\b","\\bchar8_t\\b","\\bchar16_t\\b",
        "\\bchar32_t\\b","\\bclass\\b","\\bcompl\\b","\\bconcept\\b","\\bconst\\b",
        "\\bconsteval\\b","\\bconstexpr\\b","\\bconstinit\\b","\\bconst_cast\\b",
        "\\bcontinue\\b","\\bco_await\\b","\\bco_return\\b","\\bco_yield\\b",
        "\\bdecltype\\b","\\bdefault\\b","\\bdelete\\b","\\bdo\\b","\\bdouble\\b",
        "\\bdynamic_cast\\b","\\belse\\b","\\benum\\b","\\bexplicit\\b",
        "\\bexport\\b","\\bextern\\b","\\bfalse\\b","\\bfinal\\b","\\bfloat\\b",
        "\\bfor\\b","\\bfriend\\b","\\bgoto\\b","\\bif\\b","\\binline\\b",
        "\\bint\\b","\\blong\\b","\\bmutable\\b","\\bnamespace\\b","\\bnew\\b",
        "\\bnoexcept\\b","\\bnot\\b","\\bnot_eq\\b","\\bnullptr\\b","\\boperator\\b",
        "\\bor\\b","\\bor_eq\\b","\\boverride\\b","\\bprivate\\b","\\bprotected\\b",
        "\\bpublic\\b","\\breinterpret_cast\\b","\\brequires\\b","\\breturn\\b",
        "\\bshort\\b","\\bsignals\\b","\\bsigned\\b","\\bsizeof\\b","\\bslots\\b",
        "\\bstatic\\b","\\bstatic_assert\\b","\\bstatic_cast\\b","\\bstruct\\b",
        "\\bswitch\\b","\\btemplate\\b","\\bthis\\b","\\bthread_local\\b",
        "\\bthrow\\b","\\btrue\\b","\\btry\\b","\\btypedef\\b","\\btypeid\\b",
        "\\btypename\\b","\\bunion\\b","\\bunsigned\\b","\\busing\\b",
        "\\bvirtual\\b","\\bvoid\\b","\\bvolatile\\b","\\bwchar_t\\b",
        "\\bwhile\\b","\\bxor\\b","\\bxor_eq\\b"};
    for (const QString &p : kw) {
        rule.pattern = QRegularExpression(p);
        rule.format  = keywordFormat;
        highlightingRules.append(rule);
    }
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format  = classFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\".*?\"|'.*?'");
    rule.format  = stringFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b0x[0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b");
    rule.format  = numberFormat;
    highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format  = functionFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format  = commentFormat;
    highlightingRules.append(rule);
}

// ── Python ────────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupPythonRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {
        "\\band\\b","\\bas\\b","\\bassert\\b","\\basync\\b","\\bawait\\b",
        "\\bbreak\\b","\\bclass\\b","\\bcontinue\\b","\\bdef\\b","\\bdel\\b",
        "\\belif\\b","\\belse\\b","\\bexcept\\b","\\bFalse\\b","\\bfinally\\b",
        "\\bfor\\b","\\bfrom\\b","\\bglobal\\b","\\bif\\b","\\bimport\\b",
        "\\bin\\b","\\bis\\b","\\blambda\\b","\\bNone\\b","\\bnonlocal\\b",
        "\\bnot\\b","\\bor\\b","\\bpass\\b","\\braise\\b","\\breturn\\b",
        "\\bTrue\\b","\\btry\\b","\\bwhile\\b","\\bwith\\b","\\byield\\b",
        "\\bmatch\\b","\\bcase\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }

    QTextCharFormat builtinFmt;
    builtinFmt.setForeground(functionFormat.foreground());
    builtinFmt.setFontItalic(true);
    QStringList builtins = {
        "\\babs\\b","\\ball\\b","\\bany\\b","\\bbin\\b","\\bbool\\b","\\bdict\\b",
        "\\bdir\\b","\\benumerate\\b","\\beval\\b","\\bfloat\\b","\\binput\\b",
        "\\bint\\b","\\blen\\b","\\blist\\b","\\bmax\\b","\\bmin\\b","\\bopen\\b",
        "\\bprint\\b","\\brange\\b","\\bround\\b","\\bstr\\b","\\bsum\\b",
        "\\btuple\\b","\\btype\\b","\\bzip\\b","\\bself\\b"};
    for (const QString &p : builtins) { rule.pattern = QRegularExpression(p); rule.format = builtinFmt; highlightingRules.append(rule); }

    rule.pattern = QRegularExpression("\".*?\"|'.*?'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_]*"); rule.format = functionFormat; highlightingRules.append(rule);
}

// ── JavaScript / TypeScript ───────────────────────────────────────────────────
void SyntaxHighlighter::setupJavaScriptRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {
        "\\bbreak\\b","\\bcase\\b","\\bcatch\\b","\\bclass\\b","\\bconst\\b",
        "\\bcontinue\\b","\\bdebugger\\b","\\bdefault\\b","\\bdelete\\b",
        "\\bdo\\b","\\belse\\b","\\bexport\\b","\\bextends\\b","\\bfalse\\b",
        "\\bfinally\\b","\\bfor\\b","\\bfunction\\b","\\bif\\b","\\bimport\\b",
        "\\bin\\b","\\binstanceof\\b","\\bnew\\b","\\bnull\\b","\\breturn\\b",
        "\\bsuper\\b","\\bswitch\\b","\\bthis\\b","\\bthrow\\b","\\btrue\\b",
        "\\btry\\b","\\btypeof\\b","\\bvar\\b","\\bvoid\\b","\\bwhile\\b",
        "\\bwith\\b","\\bawait\\b","\\blet\\b","\\bstatic\\b","\\byield\\b",
        "\\benum\\b","\\bimplements\\b","\\binterface\\b","\\bpackage\\b",
        "\\bprivate\\b","\\bprotected\\b","\\bpublic\\b","\\basync\\b",
        "\\bof\\b","\\btype\\b","\\bfrom\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }
    rule.pattern = QRegularExpression("=>"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\".*?\"|'.*?'|`[^`]*`"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b0x[0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
}

// ── HTML ──────────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupHtmlRules() {
    HighlightingRule rule;
    tagFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("</?[A-Za-z][A-Za-z0-9]*"); rule.format = tagFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("/?>"); rule.format = tagFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[A-Za-z-]+(?==)"); rule.format = attributeFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\"[^\"]*\""); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("<!--[^\n]*-->"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("&[A-Za-z]+;"); rule.format = numberFormat; highlightingRules.append(rule);
}

// ── CSS ───────────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupCssRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[.#]?[A-Za-z_-][A-Za-z0-9_-]*\\s*(?=\\{)"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("[A-Za-z-]+(?=\\s*:)"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*(px|em|rem|%|vh|vw|s|ms)?\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#[0-9a-fA-F]{3,8}\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("@[A-Za-z-]+"); rule.format = keywordFormat; highlightingRules.append(rule);
}

// ── Rust ──────────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupRustRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {
        "\\bas\\b","\\basync\\b","\\bawait\\b","\\bbreak\\b","\\bconst\\b",
        "\\bcontinue\\b","\\bcrate\\b","\\bdyn\\b","\\belse\\b","\\benum\\b",
        "\\bextern\\b","\\bfalse\\b","\\bfn\\b","\\bfor\\b","\\bif\\b",
        "\\bimpl\\b","\\bin\\b","\\blet\\b","\\bloop\\b","\\bmatch\\b",
        "\\bmod\\b","\\bmove\\b","\\bmut\\b","\\bpub\\b","\\bref\\b",
        "\\breturn\\b","\\bself\\b","\\bSelf\\b","\\bstatic\\b","\\bstruct\\b",
        "\\bsuper\\b","\\btrait\\b","\\btrue\\b","\\btype\\b","\\bunion\\b",
        "\\bunsafe\\b","\\buse\\b","\\bwhere\\b","\\bwhile\\b","\\btry\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }
    rule.pattern = QRegularExpression("\".*?\"|'.'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b0x[0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    classFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("[A-Z][A-Za-z0-9]+"); rule.format = classFormat; highlightingRules.append(rule);
}

// ── Go ────────────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupGoRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList kw = {
        "\\bbreak\\b","\\bcase\\b","\\bchan\\b","\\bconst\\b","\\bcontinue\\b",
        "\\bdefault\\b","\\bdefer\\b","\\belse\\b","\\bfallthrough\\b","\\bfor\\b",
        "\\bfunc\\b","\\bgo\\b","\\bgoto\\b","\\bif\\b","\\bimport\\b",
        "\\binterface\\b","\\bmap\\b","\\bpackage\\b","\\brange\\b","\\breturn\\b",
        "\\bselect\\b","\\bstruct\\b","\\bswitch\\b","\\btype\\b","\\bvar\\b",
        "\\bnil\\b","\\btrue\\b","\\bfalse\\b"};
    for (const QString &p : kw) { rule.pattern = QRegularExpression(p); rule.format = keywordFormat; highlightingRules.append(rule); }

    QTextCharFormat builtinFmt;
    builtinFmt.setForeground(functionFormat.foreground());
    builtinFmt.setFontItalic(true);
    QStringList builtins = {
        "\\bappend\\b","\\bcap\\b","\\bclose\\b","\\bcomplex\\b","\\bcopy\\b",
        "\\bdelete\\b","\\bimag\\b","\\blen\\b","\\bmake\\b","\\bnew\\b",
        "\\bpanic\\b","\\bprint\\b","\\bprintln\\b","\\breal\\b","\\brecover\\b",
        "\\bbool\\b","\\bbyte\\b","\\bfloat32\\b","\\bfloat64\\b","\\bint\\b",
        "\\bint8\\b","\\bint16\\b","\\bint32\\b","\\bint64\\b","\\brune\\b",
        "\\bstring\\b","\\buint\\b","\\buint8\\b","\\buint16\\b","\\buint32\\b",
        "\\buint64\\b","\\buintptr\\b"};
    for (const QString &p : builtins) { rule.pattern = QRegularExpression(p); rule.format = builtinFmt; highlightingRules.append(rule); }

    rule.pattern = QRegularExpression("\".*?\"|`[^`]*`|'.*?'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b0[xX][0-9a-fA-F]+\\b|\\b[0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()"); rule.format = functionFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("//[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
}

// ── JSON ──────────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupJsonRules() {
    HighlightingRule rule;
    rule.pattern = QRegularExpression("\"[^\"]*\"\\s*(?=:)"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression(":\\s*\"[^\"]*\""); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\btrue\\b|\\bfalse\\b|\\bnull\\b"); rule.format = keywordFormat; highlightingRules.append(rule);
}

// ── YAML ──────────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupYamlRules() {
    HighlightingRule rule;
    keywordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^[A-Za-z_][A-Za-z0-9_-]*(?=\\s*:)"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\"[^\"]*\"|'[^']*'"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\btrue\\b|\\bfalse\\b|\\bnull\\b|\\byes\\b|\\bno\\b"); rule.format = numberFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("#[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^\\s*-\\s"); rule.format = functionFormat; highlightingRules.append(rule);
}

// ── Markdown ──────────────────────────────────────────────────────────────────
void SyntaxHighlighter::setupMarkdownRules() {
    HighlightingRule rule;
    headingFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("^#{1,6}\\s.*$"); rule.format = headingFormat; highlightingRules.append(rule);
    boldFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\*\\*[^*]+\\*\\*|__[^_]+__"); rule.format = boldFormat; highlightingRules.append(rule);
    QTextCharFormat italicFmt; italicFmt.setFontItalic(true);
    rule.pattern = QRegularExpression("\\*[^*]+\\*|_[^_]+_"); rule.format = italicFmt; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("`[^`]+`"); rule.format = stringFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("```[^\n]*"); rule.format = commentFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\\[.*?\\]\\(.*?\\)"); rule.format = linkFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^\\s*[-*+]\\s"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^\\s*\\d+\\.\\s"); rule.format = keywordFormat; highlightingRules.append(rule);
    rule.pattern = QRegularExpression("^>\\s.*$"); rule.format = commentFormat; highlightingRules.append(rule);
}

// ── highlightBlock ────────────────────────────────────────────────────────────
void SyntaxHighlighter::highlightBlock(const QString &text) {
    if (text.isEmpty()) { setCurrentBlockState(0); return; }
    setCurrentBlockState(0);
    int startIndex = 0;
    if (currentLanguage == Language::CPP || currentLanguage == Language::JavaScript ||
        currentLanguage == Language::Rust || currentLanguage == Language::Go ||
        currentLanguage == Language::CSS) {
        if (previousBlockState() != 1)
            startIndex = text.indexOf(multiLineCommentStart);
        while (startIndex >= 0) {
            int endIndex = text.indexOf(multiLineCommentEnd, startIndex);
            int commentLength;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + 2;
            }
            setFormat(startIndex, commentLength, commentFormat);
            startIndex = text.indexOf(multiLineCommentStart, startIndex + commentLength);
        }
    }
    if (previousBlockState() != 1) {
        for (const HighlightingRule &rule : highlightingRules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                if (audioPulseIntensity > 0.01f) {
                    QTextCharFormat pulsedFmt = rule.format;
                    QColor fg = rule.format.foreground().color();
                    int h, s, v, a;
                    fg.getHsv(&h, &s, &v, &a);
                    v = qMin(255, v + static_cast<int>(audioPulseIntensity * 80));
                    s = qMin(255, s + static_cast<int>(audioPulseIntensity * 40));
                    pulsedFmt.setForeground(QColor::fromHsv(h, s, v, a));
                    setFormat(match.capturedStart(), match.capturedLength(), pulsedFmt);
                } else {
                    setFormat(match.capturedStart(), match.capturedLength(), rule.format);
                }
            }
        }
    }
}

// ── Theme application ─────────────────────────────────────────────────────────
void SyntaxHighlighter::applyTheme(const ColorTheme &theme) {
    keywordFormat.setForeground(theme.keyword);
    stringFormat.setForeground(theme.string);
    commentFormat.setForeground(theme.comment);
    numberFormat.setForeground(theme.number);
    functionFormat.setForeground(theme.function);
    classFormat.setForeground(theme.keyword);
    tagFormat.setForeground(theme.keyword);
    attributeFormat.setForeground(theme.function);
    headingFormat.setForeground(theme.keyword);
    boldFormat.setForeground(theme.foreground);
    linkFormat.setForeground(QColor(86, 156, 214));
    setupRules();
    rehighlight();
}

void SyntaxHighlighter::setAudioPulse(float intensity) {
    audioPulseIntensity = qBound(0.f, intensity, 1.f);
    rehighlight();
}
