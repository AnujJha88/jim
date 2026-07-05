#include "disassembler.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFont>
#include <QFontMetrics>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QScrollBar>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QListWidgetItem>
#include <QColor>
#include <QPalette>
#include <QFrame>
#include <QSizePolicy>

// =============================================================================
// AsmSyntaxHighlighter
// =============================================================================

AsmSyntaxHighlighter::AsmSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupRules();
}

void AsmSyntaxHighlighter::setupRules()
{
    // ----- Directives  (.text, .data, .bss, .section, …)  -----
    // Must come before labels so ".Lsym:" doesn't steal the colour.
    {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral(R"(^\s*\.[A-Za-z][A-Za-z0-9_]*)"));
        r.format.setForeground(QColor(0xc5, 0x86, 0xc0));   // purple
        m_rules.append(r);
    }

    // ----- Function / section labels  (addr <name>:  or  .Lsym:) -----
    {
        HighlightRule r;
        r.pattern = QRegularExpression(
            QStringLiteral(R"(^[0-9a-fA-F]+\s+<[^>]+>:)"));
        r.format.setForeground(QColor(0xdc, 0xdc, 0xaa));   // yellow
        r.format.setFontWeight(QFont::Bold);
        m_rules.append(r);
    }
    {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral(R"(^\.[A-Za-z_][A-Za-z0-9_.]*:)"));
        r.format.setForeground(QColor(0xdc, 0xdc, 0xaa));
        r.format.setFontWeight(QFont::Bold);
        m_rules.append(r);
    }

    // ----- Hex addresses / offsets  (0x…  or  bare leading hex column) -----
    {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral(R"(\b0x[0-9a-fA-F]+\b)"));
        r.format.setForeground(QColor(0xce, 0x91, 0x78));   // orange
        m_rules.append(r);
    }
    // bare leading address column (e.g. "401000:")
    {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral(R"(^\s*[0-9a-fA-F]+:)"));
        r.format.setForeground(QColor(0xce, 0x91, 0x78));
        m_rules.append(r);
    }

    // ----- Decimal / plain numbers -----
    {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral(R"(\b[0-9]+\b)"));
        r.format.setForeground(QColor(0xb5, 0xce, 0xa8));   // light orange/green
        m_rules.append(r);
    }

    // ----- Registers -----
    {
        HighlightRule r;
        r.pattern = QRegularExpression(
            QStringLiteral(
                R"(\b(r[a-d]x|e[a-d]x|[a-d]x|[a-d][hl])"
                R"(|r[0-9]{1,2}[dwb]?)"
                R"(|[re]?[sbi]p|[re]?[sd]i|[re]?ip|[re]?flags)"
                R"(|[xyz]mm[0-9]+)"
                R"(|st[0-9]*|cr[0-9]+|dr[0-9]+)"
                R"(|cs|ds|es|fs|gs|ss)\b)"),
            QRegularExpression::CaseInsensitiveOption);
        r.format.setForeground(QColor(0x4e, 0xc9, 0xb0));   // teal/green
        m_rules.append(r);
    }

    // ----- Mnemonics -----
    // Build one big alternation of known mnemonics.
    {
        static const char *mnemonics[] = {
            "mov","movzx","movsx","movsxd","movaps","movups","movss","movsd",
            "vmovaps","vmovups",
            "push","pop","pusha","popa","pushf","popf",
            "call","ret","retn","retf",
            "jmp","je","jne","jz","jnz","jl","jle","jg","jge",
            "ja","jb","jae","jbe","jo","jno","js","jns","jp","jnp","jcxz","jecxz","jrcxz",
            "add","sub","mul","div","imul","idiv",
            "and","or","xor","not","neg",
            "shl","shr","sar","ror","rol","rcl","rcr",
            "lea","cmp","test","nop",
            "int","syscall","sysret","sysenter","sysexit",
            "inc","dec",
            "cbw","cwd","cdq","cqo",
            "movsb","movsw","movsd","movsq",
            "stosb","stosw","stosd","stosq",
            "lodsb","lodsw","lodsd","lodsq",
            "cmpsb","cmpsw","cmpsd","cmpsq",
            "scasb","scasw","scasd","scasq",
            "rep","repe","repz","repne","repnz",
            "lock","xchg","bswap",
            "bt","bts","btr","btc",
            "lzcnt","tzcnt","popcnt",
            "cmova","cmovae","cmovb","cmovbe","cmovc","cmove","cmovg","cmovge",
            "cmovl","cmovle","cmovna","cmovnae","cmovnb","cmovnbe","cmovnc",
            "cmovne","cmovng","cmovnge","cmovnl","cmovnle","cmovno","cmovnp",
            "cmovns","cmovnz","cmovo","cmovp","cmovpe","cmovpo","cmovs","cmovz",
            "seta","setae","setb","setbe","setc","sete","setg","setge",
            "setl","setle","setna","setnae","setnb","setnbe","setnc",
            "setne","setng","setnge","setnl","setnle","setno","setnp",
            "setns","setnz","seto","setp","setpe","setpo","sets","setz",
            "fld","fst","fstp","fldz","fld1","fldpi",
            "fadd","faddp","fsub","fsubp","fsubr","fsubrp",

            "fmul","fmulp","fdiv","fdivp","fdivr","fdivrp",
            "fcom","fcomp","fcompp","fcomi","fcomip","fucom","fucomp","fucompp",
            "fxch","ffree","finit","fninit","fwait","fnop",
            "vaddps","vaddpd","vaddss","vaddsd",
            "vmulps","vmulpd","vmulss","vmulsd",
            "vsubps","vsubpd","vsubss","vsubsd",
            "vdivps","vdivpd","vdivss","vdivsd",
            "vzeroupper","vzeroall",
            "pxor","por","pand","pandn","pcmpeqb","pcmpeqw","pcmpeqd",
            "punpcklbw","punpcklwd","punpckldq","punpcklqdq",
            "punpckhbw","punpckhwd","punpckhdq","punpckhqdq",
            "pshufd","pshufhw","pshuflw","shufps","shufpd",
            "enter","leave",
            "hlt","cli","sti","clc","stc","cld","std",
            "cpuid","rdtsc","rdtscp","pause","mfence","lfence","sfence",
            nullptr
        };

        QString alt;
        for (int i = 0; mnemonics[i]; ++i) {
            if (i) alt += QLatin1Char('|');
            alt += QRegularExpression::escape(QString::fromLatin1(mnemonics[i]));
        }
        HighlightRule r;
        r.pattern = QRegularExpression(
            QStringLiteral(R"(\b(?:)") + alt + QStringLiteral(R"()\b)"),
            QRegularExpression::CaseInsensitiveOption);
        r.format.setForeground(QColor(0x56, 0x9c, 0xd6));   // blue
        m_rules.append(r);
    }

    // ----- Comments  (# … or ; … to end of line) -----
    {
        HighlightRule r;
        r.pattern = QRegularExpression(QStringLiteral(R"([#;].*$)"));
        r.format.setForeground(QColor(0x6a, 0x99, 0x55));   // grey-green
        m_rules.append(r);
    }
}

void AsmSyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), rule.format);
        }
    }
}

// =============================================================================
// DisassemblerWidget
// =============================================================================

DisassemblerWidget::DisassemblerWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

// -----------------------------------------------------------------------------
// setupUI
// -----------------------------------------------------------------------------

void DisassemblerWidget::setupUI()
{
    // ---- fonts ----------------------------------------------------------------
    QFont monoSmall;
    monoSmall.setFamily(QStringLiteral("Consolas"));
    monoSmall.setPointSize(9);
    monoSmall.setFixedPitch(true);

    QFont monoMain;
    monoMain.setFamily(QStringLiteral("Consolas"));
    monoMain.setPointSize(10);
    monoMain.setFixedPitch(true);

    // ---- colours --------------------------------------------------------------
    const QString bgDark       = QStringLiteral("#1e1e1e");
    const QString bgPanel      = QStringLiteral("#252526");
    const QString fgDefault    = QStringLiteral("#d4d4d4");
    const QString borderColor  = QStringLiteral("#3c3c3c");
    const QString accent       = QStringLiteral("#0e639c");
    const QString selBg        = QStringLiteral("#094771");
    const QString greyText     = QStringLiteral("#858585");

    // ---- root layout ----------------------------------------------------------
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    setStyleSheet(QString("background-color: %1; color: %2;").arg(bgDark, fgDefault));

    // ---- top bar --------------------------------------------------------------
    QWidget *topBar = new QWidget(this);
    topBar->setFixedHeight(32);
    topBar->setStyleSheet(QString(
        "background-color: %1;"
        "border-bottom: 1px solid %2;")
        .arg(bgPanel, borderColor));

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 0, 8, 0);
    topLayout->setSpacing(10);

    QLabel *iconLabel = new QLabel(QStringLiteral("⚙  Disassembler"), topBar);
    iconLabel->setStyleSheet(QString(
        "color: %1; font-weight: bold; font-size: 11px;").arg(fgDefault));

    m_fileLabel = new QLabel(QStringLiteral("No file"), topBar);
    m_fileLabel->setStyleSheet(QString(
        "color: %1; font-weight: bold; font-size: 11px;").arg(fgDefault));

    QWidget *spacer = new QWidget(topBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_toolLabel = new QLabel(QStringLiteral(""), topBar);
    m_toolLabel->setStyleSheet(QString(
        "color: %1; font-size: 10px;").arg(greyText));

    m_statusLabel = new QLabel(QStringLiteral("Ready"), topBar);
    m_statusLabel->setStyleSheet(QString(
        "color: %1; font-size: 10px;").arg(greyText));
    m_statusLabel->setMinimumWidth(160);
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    topLayout->addWidget(iconLabel);
    topLayout->addWidget(m_fileLabel);
    topLayout->addWidget(spacer);
    topLayout->addWidget(m_toolLabel);
    topLayout->addWidget(m_statusLabel);

    // ---- splitter -------------------------------------------------------------
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(QString(
        "QSplitter::handle { background: %1; }").arg(borderColor));

    // ---- left panel: function list -------------------------------------------
    QWidget *leftPanel = new QWidget(splitter);
    leftPanel->setStyleSheet(QString(
        "background-color: %1;").arg(bgPanel));

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    QLabel *funcHeader = new QLabel(QStringLiteral("  Functions"), leftPanel);
    funcHeader->setFixedHeight(24);
    funcHeader->setStyleSheet(QString(
        "background-color: %1;"
        "color: %2;"
        "font-size: 10px;"
        "font-weight: bold;"
        "border-bottom: 1px solid %3;"
        "padding-left: 4px;")
        .arg(bgPanel, greyText, borderColor));
    funcHeader->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_functionList = new QListWidget(leftPanel);
    m_functionList->setFont(monoSmall);
    m_functionList->setStyleSheet(QString(
        "QListWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  padding: 1px 6px;"
        "  border: none;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: %3;"
        "  color: %2;"
        "}"
        "QListWidget::item:hover:!selected {"
        "  background-color: #2a2d2e;"
        "}"
        "QScrollBar:vertical {"
        "  background: %1;"
        "  width: 8px;"
        "  border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #424242;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}")
        .arg(bgPanel, fgDefault, selBg));

    leftLayout->addWidget(funcHeader);
    leftLayout->addWidget(m_functionList, 1);

    // ---- right panel: asm view -----------------------------------------------
    m_asmView = new QPlainTextEdit(splitter);
    m_asmView->setReadOnly(true);
    m_asmView->setFont(monoMain);
    m_asmView->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_asmView->setStyleSheet(QString(
        "QPlainTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  selection-background-color: %3;"
        "}"
        "QScrollBar:vertical {"
        "  background: %1;"
        "  width: 10px;"
        "  border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #424242;"
        "  border-radius: 5px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "  background: %1;"
        "  height: 10px;"
        "  border: none;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #424242;"
        "  border-radius: 5px;"
        "  min-width: 20px;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
        "}")
        .arg(bgDark, fgDefault, selBg));

    // attach highlighter
    m_highlighter = new AsmSyntaxHighlighter(m_asmView->document());

    // ---- assemble splitter ----------------------------------------------------
    splitter->addWidget(leftPanel);
    splitter->addWidget(m_asmView);
    splitter->setSizes({200, 600});
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    // ---- wire into root -------------------------------------------------------
    rootLayout->addWidget(topBar);
    rootLayout->addWidget(splitter, 1);

    // ---- signals --------------------------------------------------------------
    connect(m_functionList, &QListWidget::currentRowChanged,
            this, &DisassemblerWidget::onFunctionSelected);
}

// -----------------------------------------------------------------------------
// loadFile
// -----------------------------------------------------------------------------

bool DisassemblerWidget::loadFile(const QString &filePath)
{
    m_filePath = filePath;

    QFileInfo fi(filePath);
    m_fileLabel->setText(fi.fileName());
    m_fileLabel->setToolTip(filePath);

    m_asmView->setPlainText(QStringLiteral("Disassembling…"));
    m_functionList->clear();
    m_functions.clear();

    m_statusLabel->setStyleSheet(QStringLiteral("color: #858585; font-size: 10px;"));
    m_statusLabel->setText(QStringLiteral("Disassembling…"));
    m_toolLabel->setText(QString());

    // kill any running process
    if (m_process) {
        m_process->disconnect();
        m_process->kill();
        m_process->waitForFinished(500);
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_accumulatedOutput.clear();

    // Try objdump first, then llvm-objdump
    const QStringList args = {
        QStringLiteral("-d"),
        QStringLiteral("-C"),
        QStringLiteral("--no-show-raw-insn"),
        filePath
    };

    if (tryDisassemble(QStringLiteral("objdump"), args)) {
        m_currentTool = QStringLiteral("objdump");
        return true;
    }
    if (tryDisassemble(QStringLiteral("llvm-objdump"), args)) {
        m_currentTool = QStringLiteral("llvm-objdump");
        return true;
    }

    m_statusLabel->setStyleSheet(QStringLiteral("color: #f44747; font-size: 10px;"));
    m_statusLabel->setText(QStringLiteral("✗ No disassembler found (objdump / llvm-objdump)"));
    m_asmView->setPlainText(
        QStringLiteral("Error: Neither 'objdump' nor 'llvm-objdump' was found on PATH.\n"
                       "Please install binutils or LLVM and ensure the tool is accessible."));
    return false;
}

QString DisassemblerWidget::getFilePath() const
{
    return m_filePath;
}

// -----------------------------------------------------------------------------
// tryDisassemble
// -----------------------------------------------------------------------------

bool DisassemblerWidget::tryDisassemble(const QString &tool, const QStringList &args)
{
    QProcess *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    // accumulate output incrementally
    connect(proc, &QProcess::readyReadStandardOutput,
            this, &DisassemblerWidget::onProcessReadyRead);

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DisassemblerWidget::onProcessFinished);

    proc->start(tool, args);
    if (!proc->waitForStarted(3000)) {
        proc->disconnect();
        proc->deleteLater();
        return false;
    }

    m_process = proc;
    return true;
}

// -----------------------------------------------------------------------------
// onProcessReadyRead
// -----------------------------------------------------------------------------

void DisassemblerWidget::onProcessReadyRead()
{
    if (!m_process) return;
    const QByteArray chunk = m_process->readAllStandardOutput();
    m_accumulatedOutput.append(QString::fromLocal8Bit(chunk));
}

// -----------------------------------------------------------------------------
// onProcessFinished
// -----------------------------------------------------------------------------

void DisassemblerWidget::onProcessFinished(int exitCode, QProcess::ExitStatus /*exitStatus*/)
{
    // drain any remaining bytes
    if (m_process) {
        const QByteArray leftover = m_process->readAllStandardOutput();
        if (!leftover.isEmpty())
            m_accumulatedOutput.append(QString::fromLocal8Bit(leftover));

        m_process->deleteLater();
        m_process = nullptr;
    }

    if (exitCode != 0 && m_accumulatedOutput.trimmed().isEmpty()) {
        m_statusLabel->setStyleSheet(QStringLiteral("color: #f44747; font-size: 10px;"));
        m_statusLabel->setText(
            QStringLiteral("✗ Disassembler exited with code %1").arg(exitCode));
        m_asmView->setPlainText(
            QStringLiteral("Disassembler process failed (exit code %1).\n"
                           "The file may not be a valid executable or object file.")
            .arg(exitCode));
        return;
    }

    parseAndDisplay(m_accumulatedOutput);
}

// -----------------------------------------------------------------------------
// parseAndDisplay
// -----------------------------------------------------------------------------

void DisassemblerWidget::parseAndDisplay(const QString &output)
{
    // --- fill text view -------------------------------------------------------
    m_asmView->setPlainText(output);

    // --- parse function labels ------------------------------------------------
    // Matches lines like:
    //   0000000000401130 <main>:
    //   08048420 <__libc_start_main@plt>:
    static const QRegularExpression labelRe(
        QStringLiteral(R"(^([0-9a-fA-F]+)\s+<([^>]+)>:)"),
        QRegularExpression::MultilineOption);

    m_functions.clear();
    m_functionList->clear();

    const QStringList lines = output.split(QLatin1Char('\n'));
    const int lineCount = lines.size();

    for (int i = 0; i < lineCount; ++i) {
        QRegularExpressionMatch m = labelRe.match(lines.at(i));
        if (m.hasMatch()) {
            FunctionEntry entry;
            entry.address    = m.captured(1);
            entry.name       = m.captured(2);
            entry.lineNumber = i;
            m_functions.append(entry);

            // Display in list:  address  functionName
            const QString display =
                QStringLiteral("%1  %2")
                .arg(entry.address.rightJustified(16, QLatin1Char('0')))
                .arg(entry.name);

            QListWidgetItem *item = new QListWidgetItem(display, m_functionList);
            item->setToolTip(entry.name);
            // subtle grey for the address prefix — achieved via a per-item font
            // colour tweak (we keep it simple: same colour, address just there)
            Q_UNUSED(item)
        }
    }

    // --- update labels --------------------------------------------------------
    const int n = m_functions.size();
    m_statusLabel->setStyleSheet(QStringLiteral("color: #4ec9b0; font-size: 10px;"));
    m_statusLabel->setText(
        n > 0
            ? QStringLiteral("✓ %1 function%2 found").arg(n).arg(n == 1 ? "" : "s")
            : QStringLiteral("✓ Done (no function labels detected)"));

    m_toolLabel->setStyleSheet(QStringLiteral("color: #858585; font-size: 10px;"));
    m_toolLabel->setText(QStringLiteral("via %1").arg(m_currentTool));
}

// -----------------------------------------------------------------------------
// onFunctionSelected
// -----------------------------------------------------------------------------

void DisassemblerWidget::onFunctionSelected(int row)
{
    if (row < 0 || row >= m_functions.size()) return;

    const FunctionEntry &entry = m_functions.at(row);
    const int targetLine = entry.lineNumber;

    QTextDocument *doc = m_asmView->document();
    if (targetLine < 0 || targetLine >= doc->blockCount()) return;

    QTextBlock block = doc->findBlockByNumber(targetLine);
    if (!block.isValid()) return;

    // move cursor to the block
    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::StartOfBlock);
    m_asmView->setTextCursor(cursor);
    m_asmView->ensureCursorVisible();

    // brief highlight via extra selections
    QTextEdit::ExtraSelection sel;
    sel.cursor = cursor;
    sel.cursor.select(QTextCursor::LineUnderCursor);
    sel.format.setBackground(QColor(0x09, 0x47, 0x71));   // accent highlight
    sel.format.setProperty(QTextFormat::FullWidthSelection, true);

    m_asmView->setExtraSelections({sel});
}