#include "storyplaytest.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>

StoryPlaytest::StoryPlaytest(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void StoryPlaytest::setupUI()
{
    setStyleSheet("background-color: #0d1117;");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Header bar
    auto *header = new QWidget();
    header->setStyleSheet("background-color: #161b22; border-bottom: 1px solid #30363d;");
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 6, 12, 6);

    auto *titleLbl = new QLabel("▶ PLAYTEST");
    titleLbl->setStyleSheet("color: #50fa7b; font-family: Consolas; font-size: 11px; font-weight: bold; letter-spacing: 2px;");
    headerLayout->addWidget(titleLbl);
    headerLayout->addStretch();

    m_restartBtn = new QPushButton("↺ Restart");
    m_restartBtn->setStyleSheet(
        "QPushButton { background: #21262d; color: #8b949e; border: 1px solid #30363d; "
        "border-radius: 4px; padding: 3px 10px; font-family: Consolas; font-size: 10px; }"
        "QPushButton:hover { background: #30363d; color: #f0f6fc; }");
    connect(m_restartBtn, &QPushButton::clicked, this, &StoryPlaytest::onRestartClicked);
    headerLayout->addWidget(m_restartBtn);
    root->addWidget(header);

    // Main content area
    auto *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Left: passage text + choices
    auto *storyPanel = new QWidget();
    storyPanel->setStyleSheet("background: transparent;");
    auto *storyLayout = new QVBoxLayout(storyPanel);
    storyLayout->setContentsMargins(16, 12, 16, 12);
    storyLayout->setSpacing(8);

    m_passageTitle = new QLabel("—");
    m_passageTitle->setStyleSheet(
        "color: #8be9fd; font-family: Consolas; font-size: 13px; font-weight: bold; "
        "border-bottom: 1px solid #21262d; padding-bottom: 6px;");
    storyLayout->addWidget(m_passageTitle);

    m_textArea = new QPlainTextEdit();
    m_textArea->setReadOnly(true);
    m_textArea->setStyleSheet(
        "QPlainTextEdit { background: transparent; color: #e6edf3; font-family: 'Georgia', serif; "
        "font-size: 13px; border: none; line-height: 1.6; }");
    m_textArea->setMinimumHeight(120);
    storyLayout->addWidget(m_textArea, 1);

    // Choices
    m_choicesWidget = new QWidget();
    m_choicesWidget->setStyleSheet("background: transparent;");
    m_choicesLayout = new QVBoxLayout(m_choicesWidget);
    m_choicesLayout->setContentsMargins(0, 8, 0, 0);
    m_choicesLayout->setSpacing(4);
    storyLayout->addWidget(m_choicesWidget);

    contentLayout->addWidget(storyPanel, 3);

    // Right: sidebar (history + vars)
    auto *sidebar = new QWidget();
    sidebar->setStyleSheet("background: #0d1117; border-left: 1px solid #21262d;");
    sidebar->setFixedWidth(200);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(10, 10, 10, 10);
    sideLayout->setSpacing(8);

    auto *histTitle = new QLabel("TRAIL");
    histTitle->setStyleSheet("color: #6272a4; font-family: Consolas; font-size: 10px; letter-spacing: 1px;");
    sideLayout->addWidget(histTitle);

    m_historyLabel = new QLabel("—");
    m_historyLabel->setStyleSheet("color: #8b949e; font-family: Consolas; font-size: 9px;");
    m_historyLabel->setWordWrap(true);
    m_historyLabel->setAlignment(Qt::AlignTop);
    sideLayout->addWidget(m_historyLabel);

    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #21262d;");
    sideLayout->addWidget(sep);

    auto *varsTitle = new QLabel("VARIABLES");
    varsTitle->setStyleSheet("color: #6272a4; font-family: Consolas; font-size: 10px; letter-spacing: 1px;");
    sideLayout->addWidget(varsTitle);

    m_varsLabel = new QLabel("—");
    m_varsLabel->setStyleSheet("color: #f1fa8c; font-family: Consolas; font-size: 9px;");
    m_varsLabel->setWordWrap(true);
    m_varsLabel->setAlignment(Qt::AlignTop);
    sideLayout->addWidget(m_varsLabel);
    sideLayout->addStretch();

    contentLayout->addWidget(sidebar);
    root->addWidget(content, 1);
}

void StoryPlaytest::loadStory(const QString &storyText)
{
    m_passages = StoryParser::parse(storyText);
    m_byName.clear();
    for (const auto &p : m_passages)
        m_byName[p.name] = &p;

    // Extract initial variable values
    auto vars = StoryParser::extractVariables(storyText);
    m_vars.clear();
    for (auto it = vars.begin(); it != vars.end(); ++it)
        m_vars[it.key()] = it.value().defaultValue;

    restart();
}

void StoryPlaytest::restart()
{
    m_history.clear();
    m_vars.clear(); // reset runtime vars
    QString start = StoryParser::findStartPassage(m_passages);
    if (!start.isEmpty())
        goToPassage(start);
    else {
        m_passageTitle->setText("No passages found");
        m_textArea->setPlainText("Open a .story file to begin playtesting.");
    }
}

void StoryPlaytest::goToPassage(const QString &name)
{
    if (!m_byName.contains(name)) {
        m_textArea->appendPlainText("\n[Error: passage '" + name + "' not found]");
        return;
    }
    m_current = name;
    m_history.append(name);
    if (m_history.size() > 20) m_history.removeFirst();

    renderPassage();
    updateHistoryDisplay();
    updateVarsDisplay();
    emit passageChanged(name);
}

void StoryPlaytest::renderPassage()
{
    const StoryPassage *p = m_byName[m_current];
    m_passageTitle->setText(":: " + p->name);

    // Process text (strip [[choice]] links from display, substitute vars)
    QString display = p->content;
    // Remove [[...]] from display text
    display.remove(QRegularExpression(R"(\[\[.*?\]\])"));
    display = processText(display.trimmed());
    m_textArea->setPlainText(display);

    // Clear old choice buttons
    QLayoutItem *item;
    while ((item = m_choicesLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (p->choices.isEmpty()) {
        bool isEnding = p->tags.contains("ending") || p->tags.contains("end");
        auto *endLbl = new QLabel(isEnding ? "✦ The End" : "[ Dead End — no choices ]");
        endLbl->setStyleSheet(isEnding
            ? "color: #f1fa8c; font-family: Consolas; font-size: 12px; font-style: italic; padding: 8px 0;"
            : "color: #ff5555; font-family: Consolas; font-size: 11px; padding: 8px 0;");
        m_choicesLayout->addWidget(endLbl);
    } else {
        for (int i = 0; i < p->choices.size(); ++i) {
            const StoryChoice &c = p->choices[i];
            auto *btn = new QPushButton(QString("▸ %1").arg(c.text));
            btn->setStyleSheet(
                "QPushButton { background: #161b22; color: #cdd6f4; border: 1px solid #30363d; "
                "border-radius: 4px; padding: 6px 12px; font-family: Consolas; font-size: 11px; "
                "text-align: left; }"
                "QPushButton:hover { background: #21262d; color: #8be9fd; border-color: #8be9fd; }");
            btn->setProperty("choiceIndex", i);
            connect(btn, &QPushButton::clicked, this, [this, i]() { onChoiceClicked(i); });
            m_choicesLayout->addWidget(btn);
        }
    }
}

QString StoryPlaytest::processText(const QString &raw)
{
    QString result = raw;
    // Substitute $varName with runtime value
    static QRegularExpression varRe(R"(\$(\w+))");
    auto it = varRe.globalMatch(raw);
    // Replace in reverse order to preserve positions
    QList<QPair<int,int>> replacements;
    while (it.hasNext()) {
        auto m = it.next();
        result.replace("$" + m.captured(1),
                        m_vars.value(m.captured(1), "$" + m.captured(1)));
    }
    return result;
}

void StoryPlaytest::onChoiceClicked(int index)
{
    if (!m_byName.contains(m_current)) return;
    const StoryPassage *p = m_byName[m_current];
    if (index < 0 || index >= p->choices.size()) return;
    goToPassage(p->choices[index].target);
}

void StoryPlaytest::onRestartClicked()
{
    restart();
}

void StoryPlaytest::updateVarsDisplay()
{
    if (m_vars.isEmpty()) { m_varsLabel->setText("—"); return; }
    QStringList lines;
    for (auto it = m_vars.begin(); it != m_vars.end(); ++it)
        lines.append("$" + it.key() + " = " + it.value());
    m_varsLabel->setText(lines.join('\n'));
}

void StoryPlaytest::updateHistoryDisplay()
{
    if (m_history.isEmpty()) { m_historyLabel->setText("—"); return; }
    // Show last 8 entries, most recent at bottom
    QStringList recent = m_history.mid(qMax(0, m_history.size() - 8));
    QStringList display;
    for (int i = 0; i < recent.size(); ++i) {
        bool isCurrent = (i == recent.size() - 1);
        display.append(isCurrent ? "▸ " + recent[i] : "  " + recent[i]);
    }
    m_historyLabel->setText(display.join('\n'));
}
