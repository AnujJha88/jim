#include "markdownviewer.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QUrl>
#include <QTextCursor>

// ─────────────────────────────────────────────────────────────────────────────
//  CSS
// ─────────────────────────────────────────────────────────────────────────────
QString MarkdownConverter::darkCss()
{
    return R"(
* { box-sizing: border-box; }
body {
    background-color: #1e1e1e;
    color: #d4d4d4;
    font-family: -apple-system, 'Segoe UI', 'Helvetica Neue', Arial, sans-serif;
    font-size: 14px;
    line-height: 1.7;
    max-width: 860px;
    margin: 0 auto;
    padding: 24px 36px 48px 36px;
}
h1, h2, h3, h4, h5, h6 {
    color: #569cd6;
    font-weight: 600;
    margin-top: 1.6em;
    margin-bottom: 0.4em;
    line-height: 1.3;
}
h1 { font-size: 2em;    border-bottom: 1px solid #3c3c3c; padding-bottom: 6px; }
h2 { font-size: 1.5em;  border-bottom: 1px solid #3c3c3c; padding-bottom: 4px; }
h3 { font-size: 1.25em; }
h4 { font-size: 1.1em;  }
h5 { font-size: 1em;    }
h6 { font-size: 0.9em; color: #858585; }
p  { margin: 0.8em 0; }
a  { color: #569cd6; text-decoration: none; }
a:hover { text-decoration: underline; }
strong { color: #d4d4d4; font-weight: 700; }
em     { color: #d4d4d4; font-style: italic; }
del    { color: #858585; text-decoration: line-through; }
code {
    background-color: #252526;
    color: #ce9178;
    font-family: 'Consolas', 'Courier New', monospace;
    font-size: 0.9em;
    padding: 2px 6px;
    border-radius: 3px;
    border: 1px solid #3c3c3c;
}
pre {
    background-color: #252526;
    border: 1px solid #3c3c3c;
    border-radius: 6px;
    padding: 16px 20px;
    overflow-x: auto;
    margin: 1em 0;
    line-height: 1.5;
}
pre code {
    background: none;
    color: #d4d4d4;
    padding: 0;
    border: none;
    border-radius: 0;
    font-size: 0.88em;
}
blockquote {
    border-left: 4px solid #569cd6;
    background: #252526;
    margin: 1em 0;
    padding: 8px 16px;
    color: #a0a0a0;
    border-radius: 0 4px 4px 0;
}
blockquote p { margin: 0.3em 0; }
ul, ol { padding-left: 2em; margin: 0.6em 0; }
li { margin: 4px 0; }
li > p { margin: 2px 0; }
table {
    border-collapse: collapse;
    width: 100%;
    margin: 1em 0;
    font-size: 0.93em;
}
th {
    background-color: #252526;
    color: #d4d4d4;
    font-weight: 600;
    border: 1px solid #3c3c3c;
    padding: 8px 14px;
    text-align: left;
}
td {
    border: 1px solid #3c3c3c;
    padding: 7px 14px;
}
tr:nth-child(even) { background-color: #252526; }
tr:hover           { background-color: #2a2d2e; }
img { max-width: 100%; border-radius: 4px; margin: 8px 0; }
hr {
    border: none;
    border-top: 1px solid #3c3c3c;
    margin: 1.8em 0;
}
input[type="checkbox"] { margin-right: 6px; }
kbd {
    background: #252526;
    border: 1px solid #3c3c3c;
    border-radius: 3px;
    padding: 1px 6px;
    font-family: monospace;
    font-size: 0.85em;
    color: #d4d4d4;
}
mark {
    background-color: #3a3a00;
    color: #dcdcaa;
    padding: 1px 4px;
    border-radius: 2px;
}
.task-list-item { list-style: none; margin-left: -1.4em; }
    )";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
QString MarkdownConverter::escape(const QString &text)
{
    QString out;
    out.reserve(text.size() + 16);
    for (const QChar &c : text) {
        if      (c == '&')  out += "&amp;";
        else if (c == '<')  out += "&lt;";
        else if (c == '>')  out += "&gt;";
        else if (c == '"')  out += "&quot;";
        else                out += c;
    }
    return out;
}

bool MarkdownConverter::isTableSeparator(const QString &line)
{
    // Must contain at least one dash and only |, -, :, space
    if (!line.contains('-')) return false;
    for (const QChar &c : line) {
        if (c != '|' && c != '-' && c != ':' && c != ' ' && c != '\t')
            return false;
    }
    return line.contains('|');
}

QStringList MarkdownConverter::splitTableRow(const QString &line)
{
    QStringList cells = line.split('|');
    // Strip leading/trailing empty cells produced by surrounding pipes
    if (!cells.isEmpty() && cells.first().trimmed().isEmpty())
        cells.removeFirst();
    if (!cells.isEmpty() && cells.last().trimmed().isEmpty())
        cells.removeLast();
    for (QString &c : cells) c = c.trimmed();
    return cells;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inline processor  (character-scanner, recursive for nested spans)
// ─────────────────────────────────────────────────────────────────────────────
QString MarkdownConverter::processInline(const QString &line)
{
    QString result;
    result.reserve(line.size() * 2);
    int i = 0;
    const int len = line.length();

    while (i < len) {
        QChar c = line[i];

        // ── raw HTML passthrough ──────────────────────────────────────────
        if (c == '<') {
            // Let raw HTML tags pass through verbatim until closing >
            int end = line.indexOf('>', i + 1);
            if (end > i) {
                result += line.mid(i, end - i + 1);
                i = end + 1;
                continue;
            }
        }

        // ── escaped character  \*  \_  etc. ──────────────────────────────
        if (c == '\\' && i + 1 < len) {
            QChar next = line[i + 1];
            static const QString escapable = R"(\`*_{}[]()#+-.!)";
            if (escapable.contains(next)) {
                result += escape(QString(next));
                i += 2;
                continue;
            }
        }

        // ── inline code  `...` ────────────────────────────────────────────
        if (c == '`') {
            // Support double-backtick spans ``...``
            int ticks = 1;
            while (i + ticks < len && line[i + ticks] == '`') ++ticks;
            QString fence = line.mid(i, ticks);
            int end = line.indexOf(fence, i + ticks);
            if (end > i) {
                QString code = line.mid(i + ticks, end - i - ticks).trimmed();
                result += "<code>" + escape(code) + "</code>";
                i = end + ticks;
                continue;
            }
        }

        // ── image  ![alt](url "title") ────────────────────────────────────
        if (c == '!' && i + 1 < len && line[i + 1] == '[') {
            int altStart = i + 2;
            int altEnd   = line.indexOf(']', altStart);
            if (altEnd > 0 && altEnd + 1 < len && line[altEnd + 1] == '(') {
                int urlEnd = line.indexOf(')', altEnd + 2);
                if (urlEnd > 0) {
                    QString alt   = line.mid(altStart, altEnd - altStart);
                    QString inner = line.mid(altEnd + 2, urlEnd - altEnd - 2).trimmed();
                    // Separate optional title  url "title"
                    QString url   = inner;
                    QString title;
                    int tq = inner.indexOf('"');
                    if (tq > 0) {
                        url   = inner.left(tq).trimmed();
                        title = inner.mid(tq + 1);
                        if (title.endsWith('"')) title.chop(1);
                    }
                    result += "<img src=\"" + escape(url) + "\""
                              " alt=\""   + escape(alt)  + "\""
                              + (title.isEmpty() ? "" : " title=\"" + escape(title) + "\"")
                              + " style=\"max-width:100%\">";
                    i = urlEnd + 1;
                    continue;
                }
            }
        }

        // ── link  [text](url "title") ─────────────────────────────────────
        if (c == '[') {
            int textEnd = line.indexOf(']', i + 1);
            if (textEnd > i && textEnd + 1 < len && line[textEnd + 1] == '(') {
                int urlEnd = line.indexOf(')', textEnd + 2);
                if (urlEnd > 0) {
                    QString text  = processInline(line.mid(i + 1, textEnd - i - 1));
                    QString inner = line.mid(textEnd + 2, urlEnd - textEnd - 2).trimmed();
                    QString url   = inner;
                    QString title;
                    int tq = inner.indexOf('"');
                    if (tq > 0) {
                        url   = inner.left(tq).trimmed();
                        title = inner.mid(tq + 1);
                        if (title.endsWith('"')) title.chop(1);
                    }
                    result += "<a href=\"" + escape(url) + "\""
                              + (title.isEmpty() ? "" : " title=\"" + escape(title) + "\"")
                              + ">" + text + "</a>";
                    i = urlEnd + 1;
                    continue;
                }
            }
        }

        // ── bold + italic  ***text*** or ___text___ ───────────────────────
        if ((c == '*' || c == '_') && i + 2 < len &&
            line[i + 1] == c && line[i + 2] == c)
        {
            QString fence = QString(3, c);
            int end = line.indexOf(fence, i + 3);
            if (end > i) {
                result += "<strong><em>"
                          + processInline(line.mid(i + 3, end - i - 3))
                          + "</em></strong>";
                i = end + 3;
                continue;
            }
        }

        // ── bold  **text** or __text__ ────────────────────────────────────
        if ((c == '*' || c == '_') && i + 1 < len && line[i + 1] == c) {
            QString fence = QString(2, c);
            int end = line.indexOf(fence, i + 2);
            if (end > i) {
                result += "<strong>"
                          + processInline(line.mid(i + 2, end - i - 2))
                          + "</strong>";
                i = end + 2;
                continue;
            }
        }

        // ── italic  *text* or _text_ ──────────────────────────────────────
        if (c == '*' || c == '_') {
            int end = line.indexOf(c, i + 1);
            if (end > i) {
                result += "<em>"
                          + processInline(line.mid(i + 1, end - i - 1))
                          + "</em>";
                i = end + 1;
                continue;
            }
        }

        // ── strikethrough  ~~text~~ ───────────────────────────────────────
        if (c == '~' && i + 1 < len && line[i + 1] == '~') {
            int end = line.indexOf("~~", i + 2);
            if (end > i) {
                result += "<del>"
                          + processInline(line.mid(i + 2, end - i - 2))
                          + "</del>";
                i = end + 2;
                continue;
            }
        }

        // ── highlight  ==text== ───────────────────────────────────────────
        if (c == '=' && i + 1 < len && line[i + 1] == '=') {
            int end = line.indexOf("==", i + 2);
            if (end > i) {
                result += "<mark>"
                          + processInline(line.mid(i + 2, end - i - 2))
                          + "</mark>";
                i = end + 2;
                continue;
            }
        }

        // ── plain character ───────────────────────────────────────────────
        result += escape(QString(c));
        ++i;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Block-level converter
// ─────────────────────────────────────────────────────────────────────────────
QString MarkdownConverter::toHtml(const QString &markdown)
{
    // Normalise line endings
    QStringList lines = QString(markdown).replace("\r\n", "\n").replace('\r', '\n').split('\n');

    QString body;
    body.reserve(markdown.size() * 3);

    // ── parser state ─────────────────────────────────────────────────────────
    bool        inCodeBlock     = false;
    QString     codeLang;
    bool        inUnorderedList = false;
    int         ulIndent        = 0;
    bool        inOrderedList   = false;
    bool        inBlockquote    = false;
    bool        inTable         = false;
    bool        tableHasHeader  = false;
    QStringList paraLines;

    // ── helpers ───────────────────────────────────────────────────────────────
    auto flushParagraph = [&]() {
        if (paraLines.isEmpty()) return;
        body += "<p>" + processInline(paraLines.join(' ')) + "</p>\n";
        paraLines.clear();
    };

    auto closeLists = [&]() {
        if (inUnorderedList) { body += "</ul>\n";  inUnorderedList = false; }
        if (inOrderedList)   { body += "</ol>\n";  inOrderedList   = false; }
    };

    auto closeAll = [&]() {
        flushParagraph();
        closeLists();
        if (inBlockquote) { body += "</blockquote>\n"; inBlockquote = false; }
        if (inTable)      { body += "</tbody></table>\n"; inTable = false; tableHasHeader = false; }
    };

    // ── line loop ─────────────────────────────────────────────────────────────
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];

        // ── fenced code block ─────────────────────────────────────────────
        if (line.startsWith("```") || line.startsWith("~~~")) {
            if (!inCodeBlock) {
                closeAll();
                codeLang = line.mid(3).trimmed();
                QString cls = codeLang.isEmpty()
                              ? QString()
                              : " class=\"language-" + escape(codeLang) + "\"";
                body += "<pre><code" + cls + ">";
                inCodeBlock = true;
            } else {
                body += "</code></pre>\n";
                inCodeBlock = false;
                codeLang.clear();
            }
            continue;
        }
        if (inCodeBlock) {
            body += escape(line) + "\n";
            continue;
        }

        // ── 4-space / tab indented code ───────────────────────────────────
        if ((line.startsWith("    ") || line.startsWith("\t"))
            && !inUnorderedList && !inOrderedList)
        {
            if (paraLines.isEmpty()) {
                closeAll();
                QString code = line.startsWith('\t') ? line.mid(1) : line.mid(4);
                body += "<pre><code>" + escape(code) + "\n";
                // Consume subsequent indented lines
                while (i + 1 < lines.size() &&
                       (lines[i + 1].startsWith("    ") || lines[i + 1].startsWith("\t")))
                {
                    ++i;
                    QString next = lines[i].startsWith('\t') ? lines[i].mid(1) : lines[i].mid(4);
                    body += escape(next) + "\n";
                }
                body += "</code></pre>\n";
                continue;
            }
        }

        // ── raw HTML block (line starts with <) ───────────────────────────
        if (line.trimmed().startsWith('<') && !line.trimmed().startsWith("<http")) {
            flushParagraph();
            closeLists();
            body += line + "\n";
            continue;
        }

        // ── horizontal rule ───────────────────────────────────────────────
        {
            QString t = line.trimmed();
            if ((t == "---" || t == "***" || t == "___" ||
                 (t.length() >= 3 &&
                  (t.count('-') == (int)t.length() ||
                   t.count('*') == (int)t.length() ||
                   t.count('_') == (int)t.length()))))
            {
                // Make sure it isn't a setext header underline
                bool isSetext = !paraLines.isEmpty();
                if (!isSetext) {
                    closeAll();
                    body += "<hr>\n";
                    continue;
                }
                // Setext h2
                if (isSetext && t.count('-') == (int)t.length()) {
                    QString heading = processInline(paraLines.join(' '));
                    paraLines.clear();
                    closeLists();
                    body += "<h2>" + heading + "</h2>\n";
                    continue;
                }
            }
            // Setext h1 (underline with ===)
            if (!paraLines.isEmpty() && t.length() >= 2 &&
                t.count('=') == (int)t.length())
            {
                QString heading = processInline(paraLines.join(' '));
                paraLines.clear();
                closeLists();
                body += "<h1>" + heading + "</h1>\n";
                continue;
            }
        }

        // ── ATX headers  # … ###### ───────────────────────────────────────
        if (line.startsWith('#')) {
            int level = 0;
            while (level < line.length() && line[level] == '#') ++level;
            if (level <= 6 && level < line.length() &&
                (line[level] == ' ' || line[level] == '\t'))
            {
                closeAll();
                QString text = line.mid(level + 1).trimmed();
                // Strip optional trailing # chars
                while (!text.isEmpty() && text.back() == '#') text.chop(1);
                text = text.trimmed();
                body += QString("<h%1>%2</h%1>\n").arg(level).arg(processInline(text));
                continue;
            }
        }

        // ── blockquote  > ─────────────────────────────────────────────────
        if (line.startsWith("> ") || line == ">") {
            if (inTable)  { closeAll(); }
            closeLists();
            flushParagraph();
            if (!inBlockquote) {
                body += "<blockquote>\n";
                inBlockquote = true;
            }
            QString content = (line.length() > 2) ? line.mid(2) : QString();
            body += "<p>" + processInline(content) + "</p>\n";
            continue;
        } else if (inBlockquote && line.trimmed().isEmpty()) {
            body += "</blockquote>\n";
            inBlockquote = false;
            continue;
        } else if (inBlockquote) {
            // Continuation of blockquote without >
            body += "<p>" + processInline(line) + "</p>\n";
            continue;
        }

        // ── unordered list  - / * / + ─────────────────────────────────────
        {
            QRegularExpression ulRe(R"(^(\s*)([-*+])\s+(.*)$)");
            QRegularExpressionMatch m = ulRe.match(line);
            if (m.hasMatch()) {
                int indent = m.captured(1).length();
                QString item = m.captured(3);

                flushParagraph();
                if (inOrderedList && indent <= ulIndent) {
                    body += "</ol>\n";
                    inOrderedList = false;
                }
                if (!inUnorderedList) {
                    body += "<ul>\n";
                    inUnorderedList = true;
                    ulIndent = indent;
                }

                // Task list checkbox
                QString li;
                if (item.startsWith("[ ] ")) {
                    body += "<li class=\"task-list-item\">";
                    li = "<span style=\"color:#569cd6; font-family:monospace; margin-right:4px;\">☐</span> "
                         + processInline(item.mid(4));
                } else if (item.startsWith("[x] ") || item.startsWith("[X] ")) {
                    body += "<li class=\"task-list-item\">";
                    li = "<span style=\"color:#569cd6; font-family:monospace; margin-right:4px;\">☑</span> "
                         + processInline(item.mid(4));
                } else {
                    body += "<li>";
                    li = processInline(item);
                }
                body += li + "</li>\n";
                continue;
            }
        }

        // ── ordered list  1. / 2. ────────────────────────────────────────
        {
            QRegularExpression olRe(R"(^(\s*)\d+[.)]\s+(.*)$)");
            QRegularExpressionMatch m = olRe.match(line);
            if (m.hasMatch()) {
                flushParagraph();
                if (inUnorderedList) { body += "</ul>\n"; inUnorderedList = false; }
                if (!inOrderedList)  { body += "<ol>\n";  inOrderedList = true; }
                body += "<li>" + processInline(m.captured(2)) + "</li>\n";
                continue;
            }
        }

        // Close lists if we hit a non-list line
        if (inUnorderedList || inOrderedList) {
            if (line.trimmed().isEmpty()) {
                closeLists();
                continue;
            }
            // Continuation paragraph inside list item — append to current <li>
            // For simplicity treat as closing list
            closeLists();
        }

        // ── table ─────────────────────────────────────────────────────────
        if (line.contains('|')) {
            // Check ahead for separator
            bool nextIsSep = (i + 1 < lines.size()) && isTableSeparator(lines[i + 1]);

            if (!inTable && nextIsSep) {
                // Header row
                closeAll();
                body += "<table>\n<thead>\n<tr>\n";
                for (const QString &cell : splitTableRow(line))
                    body += "<th>" + processInline(cell) + "</th>\n";
                body += "</tr>\n</thead>\n<tbody>\n";
                inTable = true;
                tableHasHeader = true;
                ++i; // skip separator line
                continue;
            }

            if (inTable && !isTableSeparator(line)) {
                body += "<tr>\n";
                for (const QString &cell : splitTableRow(line))
                    body += "<td>" + processInline(cell) + "</td>\n";
                body += "</tr>\n";
                continue;
            }
        }
        if (inTable) {
            body += "</tbody></table>\n";
            inTable = false;
            tableHasHeader = false;
        }

        // ── empty line ────────────────────────────────────────────────────
        if (line.trimmed().isEmpty()) {
            closeAll();
            continue;
        }

        // ── regular paragraph text ────────────────────────────────────────
        // Hard line break: two trailing spaces
        if (line.endsWith("  ")) {
            paraLines.append(line.trimmed() + "<br>");
        } else {
            paraLines.append(line);
        }
    }

    closeAll();

    return "<!DOCTYPE html>\n"
           "<html><head>\n"
           "<meta charset=\"utf-8\">\n"
           "<style>\n" + darkCss() + "\n</style>\n"
           "</head><body>\n"
           + body +
           "\n</body></html>";
}

// ─────────────────────────────────────────────────────────────────────────────
//  MarkdownPreviewWidget
// ─────────────────────────────────────────────────────────────────────────────
MarkdownPreviewWidget::MarkdownPreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void MarkdownPreviewWidget::setupUI()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    setStyleSheet("background-color: #1e1e1e;");

    // ── header bar ────────────────────────────────────────────────────────
    auto *bar = new QWidget;
    bar->setFixedHeight(32);
    bar->setStyleSheet(
        "background-color: #252526;"
        "border-bottom: 1px solid #3c3c3c;");

    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(12, 0, 8, 0);
    barLayout->setSpacing(6);

    auto *icon = new QLabel("⬡");
    icon->setStyleSheet("color: #569cd6; font-size: 14px; background: transparent;");
    barLayout->addWidget(icon);

    m_titleLabel = new QLabel("Markdown Preview");
    m_titleLabel->setStyleSheet(
        "color: #d4d4d4; font-size: 12px; font-weight: 500; background: transparent;");
    barLayout->addWidget(m_titleLabel);
    barLayout->addStretch();

    root->addWidget(bar);

    // ── browser ───────────────────────────────────────────────────────────
    m_browser = new QTextBrowser;
    m_browser->setOpenLinks(false);   // we handle link clicks manually
    m_browser->setOpenExternalLinks(false);
    m_browser->setStyleSheet(
        "QTextBrowser {"
        "  background-color: #1e1e1e;"
        "  color: #d4d4d4;"
        "  border: none;"
        "  padding: 0px;"
        "}"
        "QScrollBar:vertical {"
        "  background: #252526; width: 10px; border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #424242; border-radius: 5px; min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #555; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal {"
        "  background: #252526; height: 10px; border: none;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #424242; border-radius: 5px; min-width: 20px;"
        "}"
        "QScrollBar::handle:horizontal:hover { background: #555; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }");

    connect(m_browser, &QTextBrowser::anchorClicked,
            this,      &MarkdownPreviewWidget::onAnchorClicked);

    root->addWidget(m_browser, 1);
}

void MarkdownPreviewWidget::setBaseDirectory(const QString &dir)
{
    m_baseDir = dir;
    if (!m_baseDir.isEmpty())
        m_browser->setSearchPaths({ m_baseDir });
}

void MarkdownPreviewWidget::setContent(const QString &markdown, const QString &baseDir)
{
    m_currentMarkdown = markdown;

    if (!baseDir.isEmpty())
        setBaseDirectory(baseDir);

    // Remember scroll position so live-preview doesn't jump
    int scrollVal = m_browser->verticalScrollBar()->value();

    QString html = MarkdownConverter::toHtml(markdown);
    m_browser->setHtml(html);

    // Restore scroll
    m_browser->verticalScrollBar()->setValue(scrollVal);
}

void MarkdownPreviewWidget::onAnchorClicked(const QUrl &url)
{
    emit linkActivated(url.toString());
}