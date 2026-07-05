#include "binaryinspector.h"

#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDateTime>
#include <QtEndian>
#include <QTableWidgetItem>
#include <QScrollBar>
#include <QFont>
#include <QPalette>
#include <cstring>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  Colour constants matching the Jim dark theme
// ─────────────────────────────────────────────────────────────────────────────
static const char *kBg        = "#1e1e1e";
static const char *kPanel     = "#252526";
static const char *kFg        = "#d4d4d4";
static const char *kBorder    = "#3c3c3c";
static const char *kAccent    = "#569cd6";
static const char *kOrange    = "#ce9178";
static const char *kGreen     = "#4ec9b0";
static const char *kGrey      = "#858585";
static const char *kYellow    = "#dcdcaa";
static const char *kPurple    = "#c586c0";
static const char *kSel       = "#094771";

// ─────────────────────────────────────────────────────────────────────────────
//  Construction / UI
// ─────────────────────────────────────────────────────────────────────────────
BinaryInspectorWidget::BinaryInspectorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void BinaryInspectorWidget::setupUI()
{
    setStyleSheet(QString("background-color: %1; color: %2;").arg(kBg).arg(kFg));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── top info bar ──────────────────────────────────────────────────────────
    auto *infoBar = new QWidget;
    infoBar->setStyleSheet(QString("background-color: %1; "
                                   "border-bottom: 1px solid %2;")
                               .arg(kPanel).arg(kBorder));
    auto *infoLayout = new QHBoxLayout(infoBar);
    infoLayout->setContentsMargins(12, 8, 12, 8);

    m_fileInfoLabel = new QLabel("No file loaded");
    m_fileInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_fileInfoLabel->setWordWrap(true);
    QFont infoFont("Consolas", 9);
    m_fileInfoLabel->setFont(infoFont);
    m_fileInfoLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(kFg));
    infoLayout->addWidget(m_fileInfoLabel);
    root->addWidget(infoBar);

    // ── tab widget ────────────────────────────────────────────────────────────
    m_tabs = new QTabWidget;
    m_tabs->setStyleSheet(QString(
        "QTabWidget::pane { border: none; background: %1; }"
        "QTabBar::tab { background: %2; color: %3; padding: 6px 16px; "
        "               border: none; border-right: 1px solid %4; }"
        "QTabBar::tab:selected { background: %1; color: #ffffff; "
        "                        border-bottom: 2px solid %5; }"
        "QTabBar::tab:hover { background: #2d2d30; }")
        .arg(kBg).arg(kPanel).arg(kGrey).arg(kBorder).arg(kAccent));

    // Headers tab
    m_headerTable = new QTableWidget(0, 3);
    m_headerTable->setHorizontalHeaderLabels({"Field", "Value", "Description"});
    styleTable(m_headerTable);
    m_tabs->addTab(m_headerTable, "Headers");

    // Sections tab
    m_sectionsTable = new QTableWidget(0, 5);
    m_sectionsTable->setHorizontalHeaderLabels(
        {"Name", "Virt. Addr", "File Offset", "Size", "Flags"});
    styleTable(m_sectionsTable);
    m_tabs->addTab(m_sectionsTable, "Sections");

    // Imports tab
    m_importsTable = new QTableWidget(0, 2);
    m_importsTable->setHorizontalHeaderLabels({"Library", "Symbol / Function"});
    styleTable(m_importsTable);
    m_tabs->addTab(m_importsTable, "Imports");

    // Strings tab
    m_stringsView = new QPlainTextEdit;
    m_stringsView->setReadOnly(true);
    m_stringsView->setFont(QFont("Consolas", 9));
    m_stringsView->setStyleSheet(
        QString("QPlainTextEdit { background: %1; color: %2; border: none; }"
                "QScrollBar:vertical { background: %3; width: 8px; }"
                "QScrollBar::handle:vertical { background: #555; border-radius: 4px; }")
            .arg(kBg).arg(kFg).arg(kPanel));
    m_tabs->addTab(m_stringsView, "Strings");

    root->addWidget(m_tabs, 1);
}

void BinaryInspectorWidget::styleTable(QTableWidget *t)
{
    t->setStyleSheet(QString(
        "QTableWidget { background: %1; color: %2; border: none; "
        "               gridline-color: %3; alternate-background-color: %4; }"
        "QTableWidget::item { padding: 3px 8px; }"
        "QTableWidget::item:selected { background: %5; color: #ffffff; }"
        "QHeaderView::section { background: %4; color: %6; padding: 4px 8px; "
        "                       border: none; border-right: 1px solid %3; "
        "                       border-bottom: 1px solid %3; font-weight: bold; }"
        "QScrollBar:vertical { background: %4; width: 8px; }"
        "QScrollBar::handle:vertical { background: #555; border-radius: 4px; }"
        "QScrollBar:horizontal { background: %4; height: 8px; }"
        "QScrollBar::handle:horizontal { background: #555; border-radius: 4px; }")
        .arg(kBg).arg(kFg).arg(kBorder).arg(kPanel).arg(kSel).arg(kGrey));

    t->setAlternatingRowColors(true);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setShowGrid(false);
    t->verticalHeader()->hide();
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────
bool BinaryInspectorWidget::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    m_filePath = filePath;

    // Refuse files larger than 256 MB to stay responsive
    if (file.size() > 256LL * 1024 * 1024) {
        m_fileInfoLabel->setText(
            QString("File too large to inspect inline (%1).")
                .arg(formatSize(file.size())));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    clearTables();
    analyzeFile(data);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Top-level analysis
// ─────────────────────────────────────────────────────────────────────────────
void BinaryInspectorWidget::analyzeFile(const QByteArray &data)
{
    QFileInfo fi(m_filePath);
    QString md5 = computeMD5(data);

    bool isELF = (data.size() >= 4 &&
                  (quint8)data[0] == 0x7F &&
                  data[1] == 'E' && data[2] == 'L' && data[3] == 'F');

    bool isMZ  = (data.size() >= 2 &&
                  (quint8)data[0] == 'M' && (quint8)data[1] == 'Z');
    bool isPE  = false;
    if (isMZ && data.size() >= 0x40) {
        quint32 peOff;
        memcpy(&peOff, data.constData() + 0x3C, 4);
        peOff = qFromLittleEndian(peOff);
        isPE = (peOff + 4 <= (quint32)data.size() &&
                data[peOff] == 'P' && data[peOff+1] == 'E' &&
                data[peOff+2] == 0  && data[peOff+3] == 0);
    }

    QString formatStr = "Unknown / Raw Binary";
    if (isELF) formatStr = "ELF";
    if (isPE)  formatStr = "PE (Portable Executable)";

    // Basic file info
    m_fileInfoLabel->setText(
        QString("  File: %1    Size: %2    Format: %3    MD5: %4")
            .arg(fi.fileName())
            .arg(formatSize(data.size()))
            .arg(formatStr)
            .arg(md5));

    if      (isELF) parseELF(data);
    else if (isPE)  parsePE (data);
    else {
        addHeaderRow("Format",   "Unknown",           "Not a recognised ELF or PE binary");
        addHeaderRow("Size",     formatSize(data.size()), "Total file size");
        addHeaderRow("MD5",      md5,                 "");
    }

    extractStrings(data);
}

// ─────────────────────────────────────────────────────────────────────────────
//  ELF Parser
// ─────────────────────────────────────────────────────────────────────────────
bool BinaryInspectorWidget::parseELF(const QByteArray &data)
{
    if (data.size() < 64) return false;

    bool le   = ((quint8)data[5] == 1);   // EI_DATA: 1=LE, 2=BE
    bool is64 = ((quint8)data[4] == 2);   // EI_CLASS: 1=32-bit, 2=64-bit

    // Endian-aware readers
    auto u16 = [&](int off) -> quint16 {
        if (off + 2 > data.size()) return 0;
        quint16 v; memcpy(&v, data.constData() + off, 2);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };
    auto u32 = [&](int off) -> quint32 {
        if (off + 4 > data.size()) return 0;
        quint32 v; memcpy(&v, data.constData() + off, 4);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };
    auto u64 = [&](int off) -> quint64 {
        if (off + 8 > data.size()) return 0;
        quint64 v; memcpy(&v, data.constData() + off, 8);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };

    // ── ELF header fields ─────────────────────────────────────────────────────
    quint8  osabi    = (quint8)data[7];
    quint16 e_type   = u16(16);
    quint16 e_mach   = u16(18);
    quint32 e_ver    = u32(20);
    quint64 e_entry  = is64 ? u64(24) : u32(24);
    quint64 e_phoff  = is64 ? u64(32) : u32(28);
    quint64 e_shoff  = is64 ? u64(40) : u32(32);
    quint32 e_flags  = is64 ? u32(48) : u32(36);
    quint16 e_ehsize = is64 ? u16(52) : u16(40);
    quint16 e_phesz  = is64 ? u16(54) : u16(42);
    quint16 e_phnum  = is64 ? u16(56) : u16(44);
    quint16 e_shesz  = is64 ? u16(58) : u16(46);
    quint16 e_shnum  = is64 ? u16(60) : u16(48);
    quint16 e_ssnx   = is64 ? u16(62) : u16(50);

    // String mappings
    auto typeName = [](quint16 t) -> QString {
        switch (t) {
        case 0: return "ET_NONE (None)";
        case 1: return "ET_REL (Relocatable)";
        case 2: return "ET_EXEC (Executable)";
        case 3: return "ET_DYN (Shared object)";
        case 4: return "ET_CORE (Core)";
        default: return QString("0x%1").arg(t, 4, 16, QChar('0'));
        }
    };
    auto machName = [](quint16 m) -> QString {
        switch (m) {
        case 0x00: return "None";
        case 0x02: return "SPARC";
        case 0x03: return "x86";
        case 0x08: return "MIPS";
        case 0x14: return "PowerPC";
        case 0x16: return "PowerPC 64";
        case 0x28: return "ARM (32-bit)";
        case 0x2A: return "SuperH";
        case 0x32: return "IA-64";
        case 0x3E: return "x86-64 (AMD64)";
        case 0xB7: return "AArch64 (ARM64)";
        case 0xF3: return "RISC-V";
        case 0xF7: return "BPF";
        default:   return QString("0x%1").arg(m, 4, 16, QChar('0'));
        }
    };
    auto osabiName = [](quint8 a) -> QString {
        switch (a) {
        case 0:  return "System V / None";
        case 1:  return "HP-UX";
        case 2:  return "NetBSD";
        case 3:  return "Linux";
        case 6:  return "Solaris";
        case 7:  return "AIX";
        case 8:  return "IRIX";
        case 9:  return "FreeBSD";
        case 12: return "OpenBSD";
        case 64: return "ARM EABI";
        case 97: return "ARM";
        case 255:return "Standalone";
        default: return QString("0x%1").arg(a, 2, 16, QChar('0'));
        }
    };

    addHeaderRow("Class",        is64 ? "ELF64" : "ELF32",     "Address size");
    addHeaderRow("Endianness",   le ? "Little Endian" : "Big Endian", "Data encoding");
    addHeaderRow("OS/ABI",       osabiName(osabi),              "Target OS/ABI");
    addHeaderRow("Type",         typeName(e_type),              "Object file type");
    addHeaderRow("Architecture", machName(e_mach),              "Target ISA");
    addHeaderRow("Version",      QString::number(e_ver),        "ELF specification version");
    addHeaderRow("Entry Point",  QString("0x%1").arg(e_entry, 0, 16), "Virtual address of entry");
    addHeaderRow("PH Offset",    QString("0x%1").arg(e_phoff, 0, 16), "Program header table offset");
    addHeaderRow("SH Offset",    QString("0x%1").arg(e_shoff, 0, 16), "Section header table offset");
    addHeaderRow("Flags",        QString("0x%1").arg(e_flags, 8, 16, QChar('0')), "Processor-specific flags");
    addHeaderRow("EH Size",      QString::number(e_ehsize) + " bytes", "ELF header size");
    addHeaderRow("PH Entry Size",QString::number(e_phesz)  + " bytes", "Program header entry size");
    addHeaderRow("PH Count",     QString::number(e_phnum),      "Number of program headers");
    addHeaderRow("SH Entry Size",QString::number(e_shesz)  + " bytes", "Section header entry size");
    addHeaderRow("SH Count",     QString::number(e_shnum),      "Number of section headers");
    addHeaderRow("SH Strtab Idx",QString::number(e_ssnx),       "Section name string table index");

    // Update info bar with richer detail
    m_fileInfoLabel->setText(
        m_fileInfoLabel->text().replace("ELF",
            QString("ELF  ·  %1  ·  %2  ·  %3")
                .arg(is64 ? "64-bit" : "32-bit")
                .arg(machName(e_mach))
                .arg(typeName(e_type))));

    // Parse sections
    if (e_shoff > 0 && e_shesz > 0 && e_shnum > 0)
        parseELFSections(data, is64, le,
                         (int)e_shoff, (int)e_shesz, (int)e_shnum, (int)e_ssnx);

    // Parse dynamic symbols for imports panel
    parseELFDynSymbols(data, is64, le);

    return true;
}

void BinaryInspectorWidget::parseELFSections(const QByteArray &data,
                                              bool is64, bool le,
                                              int shoff, int shentsize,
                                              int shnum, int shstrndx)
{
    auto u32 = [&](int off) -> quint32 {
        if (off + 4 > data.size()) return 0;
        quint32 v; memcpy(&v, data.constData() + off, 4);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };
    auto u64 = [&](int off) -> quint64 {
        if (off + 8 > data.size()) return 0;
        quint64 v; memcpy(&v, data.constData() + off, 8);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };

    // Locate the section-name string table
    QByteArray strtab;
    if (shstrndx < shnum) {
        int sOff = shoff + shstrndx * shentsize;
        quint64 strRawOff  = is64 ? u64(sOff + 24) : u32(sOff + 16);
        quint64 strRawSize = is64 ? u64(sOff + 32) : u32(sOff + 20);
        if (strRawOff > 0 && strRawOff + strRawSize <= (quint64)data.size())
            strtab = data.mid((int)strRawOff, (int)strRawSize);
    }

    auto getName = [&](quint32 idx) -> QString {
        if (strtab.isEmpty() || idx >= (quint32)strtab.size()) return "";
        return QString::fromLatin1(strtab.constData() + idx);
    };

    auto flagStr = [](quint64 f) -> QString {
        QString s;
        if (f & 0x01) s += "W";   // SHF_WRITE
        if (f & 0x02) s += "A";   // SHF_ALLOC
        if (f & 0x04) s += "X";   // SHF_EXECINSTR
        if (f & 0x10) s += "M";   // SHF_MERGE
        if (f & 0x20) s += "S";   // SHF_STRINGS
        return s.isEmpty() ? "-" : s;
    };

    for (int i = 0; i < shnum; ++i) {
        int sOff = shoff + i * shentsize;
        if (sOff + shentsize > data.size()) break;

        quint32 sh_name    = u32(sOff);
        quint64 sh_flags   = is64 ? u64(sOff + 8)  : u32(sOff + 8);
        quint64 sh_addr    = is64 ? u64(sOff + 16) : u32(sOff + 12);
        quint64 sh_offset  = is64 ? u64(sOff + 24) : u32(sOff + 16);
        quint64 sh_size    = is64 ? u64(sOff + 32) : u32(sOff + 20);

        QString name = getName(sh_name);
        if (name.isEmpty() && i == 0) name = "(null)";

        addSectionRow(name,
                      QString("0x%1").arg(sh_addr,   0, 16),
                      QString("0x%1").arg(sh_offset, 0, 16),
                      formatSize((qint64)sh_size),
                      flagStr(sh_flags));
    }
}

void BinaryInspectorWidget::parseELFDynSymbols(const QByteArray &data,
                                                bool is64, bool le)
{
    // We look for a .dynstr section (type SHT_STRTAB associated with .dynsym)
    // Simplified: scan for null-separated printable strings in the .dynstr region
    // by finding the SHT_DYNSYM section and following sh_link to .dynstr.
    auto u16 = [&](int off) -> quint16 {
        if (off + 2 > data.size()) return 0;
        quint16 v; memcpy(&v, data.constData() + off, 2);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };
    auto u32 = [&](int off) -> quint32 {
        if (off + 4 > data.size()) return 0;
        quint32 v; memcpy(&v, data.constData() + off, 4);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };
    auto u64 = [&](int off) -> quint64 {
        if (off + 8 > data.size()) return 0;
        quint64 v; memcpy(&v, data.constData() + off, 8);
        return le ? qFromLittleEndian(v) : qFromBigEndian(v);
    };

    int shoff     = (int)(is64 ? u64(40) : u32(32));
    int shentsize = (int)(is64 ? u16(58) : u16(46));
    int shnum     = (int)(is64 ? u16(60) : u16(48));
    if (shoff == 0 || shentsize == 0 || shnum == 0) return;

    // Find SHT_DYNSYM (type == 11)
    int dynsymOff   = -1;
    int dynsymSize  = 0;
    int dynsymLink  = 0;
    int dynsymEntSz = is64 ? 24 : 16;

    for (int i = 0; i < shnum; ++i) {
        int sOff = shoff + i * shentsize;
        if (sOff + shentsize > data.size()) break;
        quint32 sh_type = u32(sOff + 4);
        if (sh_type == 11) { // SHT_DYNSYM
            dynsymOff  = (int)(is64 ? u64(sOff + 24) : u32(sOff + 16));
            dynsymSize = (int)(is64 ? u64(sOff + 32) : u32(sOff + 20));
            dynsymLink = (int) u32(sOff + (is64 ? 40 : 24));
            quint64 esz = is64 ? u64(sOff + 56) : u32(sOff + 36);
            if (esz > 0) dynsymEntSz = (int)esz;
            break;
        }
    }
    if (dynsymOff <= 0) return;

    // Find linked string table section
    QByteArray dynstr;
    if (dynsymLink > 0 && dynsymLink < shnum) {
        int sOff = shoff + dynsymLink * shentsize;
        int strOff  = (int)(is64 ? u64(sOff + 24) : u32(sOff + 16));
        int strSize = (int)(is64 ? u64(sOff + 32) : u32(sOff + 20));
        if (strOff > 0 && strOff + strSize <= data.size())
            dynstr = data.mid(strOff, strSize);
    }
    if (dynstr.isEmpty()) return;

    int numSyms = dynsymSize / dynsymEntSz;
    QString lastLib = "(ELF dynamic)";
    int added = 0;
    const int kMaxImports = 500;

    for (int i = 1; i < numSyms && added < kMaxImports; ++i) {
        int symOff = dynsymOff + i * dynsymEntSz;
        if (symOff + dynsymEntSz > data.size()) break;

        quint32 st_name = u32(symOff);
        if (st_name == 0 || st_name >= (quint32)dynstr.size()) continue;

        QString symName = QString::fromLatin1(dynstr.constData() + st_name);
        if (symName.isEmpty()) continue;

        addImportRow(lastLib, symName);
        ++added;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PE Parser
// ─────────────────────────────────────────────────────────────────────────────
bool BinaryInspectorWidget::parsePE(const QByteArray &data)
{
    if (data.size() < 0x40) return false;

    // All PE values are little-endian
    auto u8  = [&](int off) -> quint8  { return (quint8)data[off]; };
    auto u16 = [&](int off) -> quint16 {
        if (off + 2 > data.size()) return 0;
        quint16 v; memcpy(&v, data.constData() + off, 2);
        return qFromLittleEndian(v);
    };
    auto u32 = [&](int off) -> quint32 {
        if (off + 4 > data.size()) return 0;
        quint32 v; memcpy(&v, data.constData() + off, 4);
        return qFromLittleEndian(v);
    };
    auto u64 = [&](int off) -> quint64 {
        if (off + 8 > data.size()) return 0;
        quint64 v; memcpy(&v, data.constData() + off, 8);
        return qFromLittleEndian(v);
    };

    quint32 peOff = u32(0x3C);
    if (peOff + 24 > (quint32)data.size()) return false;

    // ── COFF header (immediately after PE signature) ───────────────────────
    int coffBase = (int)peOff + 4;
    quint16 machine      = u16(coffBase);
    quint16 numSections  = u16(coffBase + 2);
    quint32 timeStamp    = u32(coffBase + 4);
    quint16 optHdrSize   = u16(coffBase + 16);
    quint16 characts     = u16(coffBase + 18);

    auto machineName = [](quint16 m) -> QString {
        switch (m) {
        case 0x0000: return "Any";
        case 0x014C: return "x86 (i386)";
        case 0x0200: return "IA-64 (Itanium)";
        case 0x8664: return "x86-64 (AMD64)";
        case 0x01C0: return "ARM (little endian)";
        case 0x01C4: return "ARM Thumb-2";
        case 0xAA64: return "ARM64 (AArch64)";
        case 0x5032: return "RISC-V 32-bit";
        case 0x5064: return "RISC-V 64-bit";
        default:     return QString("0x%1").arg(m, 4, 16, QChar('0'));
        }
    };

    auto buildCharsStr = [](quint16 c) -> QString {
        QStringList flags;
        if (c & 0x0002) flags << "Executable";
        if (c & 0x0020) flags << "No relocations";
        if (c & 0x0100) flags << "32-bit";
        if (c & 0x0200) flags << "No debug";
        if (c & 0x1000) flags << "System";
        if (c & 0x2000) flags << "DLL";
        if (c & 0x4000) flags << "Uniprocessor";
        return flags.isEmpty() ? "0x" + QString::number(c, 16) : flags.join(" | ");
    };

    QDateTime ts = QDateTime::fromSecsSinceEpoch(timeStamp, Qt::UTC);

    addHeaderRow("Signature",        "PE\\0\\0",              "Portable Executable magic");
    addHeaderRow("Machine",          machineName(machine),     "Target CPU architecture");
    addHeaderRow("Number of Sections",  QString::number(numSections),       "Count of section headers");
    addHeaderRow("Timestamp",           ts.toString("yyyy-MM-dd hh:mm:ss UTC"), "Link/compile time");
    addHeaderRow("Opt. Header Size",    QString::number(optHdrSize) + " bytes", "Optional header size");
    addHeaderRow("Characteristics",     buildCharsStr(characts),             "File attribute flags");

    // ── Optional header ────────────────────────────────────────────────────
    int optBase = coffBase + 20;   // optional header starts here
    if (optHdrSize >= 2 && optBase + 2 <= data.size()) {
        quint16 magic     = u16(optBase);
        bool    pe32plus  = (magic == 0x020B);

        auto subsysName = [](quint16 s) -> QString {
            switch (s) {
            case 1:  return "Native";
            case 2:  return "Windows GUI";
            case 3:  return "Windows CUI (console)";
            case 5:  return "OS/2 CUI";
            case 7:  return "POSIX CUI";
            case 9:  return "Windows CE GUI";
            case 10: return "EFI Application";
            case 11: return "EFI Boot Service Driver";
            case 12: return "EFI Runtime Driver";
            case 14: return "Xbox";
            case 16: return "Windows Boot Application";
            default: return QString("0x%1").arg(s, 4, 16, QChar('0'));
            }
        };

        quint32 aoe      = u32(optBase + 16);
        quint64 imgBase  = pe32plus ? u64(optBase + 24) : u32(optBase + 28);
        quint32 secAlign = u32(optBase + 32);
        quint32 filAlign = u32(optBase + 36);
        quint16 majorOS  = u16(optBase + 40);
        quint16 minorOS  = u16(optBase + 42);
        quint32 sizeImg  = u32(optBase + 56);
        quint16 subsys   = u16(optBase + 68);

        addHeaderRow("Optional Magic",     pe32plus ? "PE32+ (64-bit)" : "PE32 (32-bit)", "Optional header type");
        addHeaderRow("Entry Point (RVA)",  QString("0x%1").arg(aoe, 8, 16, QChar('0')),   "Address of entry point");
        addHeaderRow("Image Base",         QString("0x%1").arg(imgBase, 0, 16),            "Preferred load address");
        addHeaderRow("Section Alignment",  QString("0x%1").arg(secAlign, 0, 16),           "In-memory section alignment");
        addHeaderRow("File Alignment",     QString("0x%1").arg(filAlign, 0, 16),           "On-disk section alignment");
        addHeaderRow("OS Version",         QString("%1.%2").arg(majorOS).arg(minorOS),      "Minimum OS version");
        addHeaderRow("Size of Image",      formatSize(sizeImg),                             "Total image size in memory");
        addHeaderRow("Subsystem",          subsysName(subsys),                              "Target Windows subsystem");

        // Update info bar
        m_fileInfoLabel->setText(
            m_fileInfoLabel->text().replace("PE (Portable Executable)",
                QString("PE  ·  %1  ·  %2  ·  %3")
                    .arg(pe32plus ? "64-bit" : "32-bit")
                    .arg(machineName(machine))
                    .arg((characts & 0x2000) ? "DLL" : "Executable")));

        // ── Data directory: locate Import Table RVA ────────────────────────
        // DataDirectory[1] = Import Table
        // Offset is from beginning of Optional Header.
        // PE32  (32-bit): DataDirectory starts at offset 96
        // PE32+ (64-bit): DataDirectory starts at offset 112
        int ddBase = optBase + (pe32plus ? 112 : 96);
        quint32 importRVA  = (ddBase + 8 <= data.size()) ? u32(ddBase + 8)  : 0;

        // ── Section headers ────────────────────────────────────────────────
        int firstSecOff = optBase + optHdrSize;
        parsePESections(data, firstSecOff, (int)numSections);

        // ── Imports ────────────────────────────────────────────────────────
        if (importRVA != 0)
            parsePEImports(data, importRVA, pe32plus);
    }

    return true;
}

void BinaryInspectorWidget::parsePESections(const QByteArray &data,
                                             int firstSectionOff,
                                             int numSections)
{
    auto u32 = [&](int off) -> quint32 {
        if (off + 4 > data.size()) return 0;
        quint32 v; memcpy(&v, data.constData() + off, 4);
        return qFromLittleEndian(v);
    };

    auto secFlags = [](quint32 c) -> QString {
        QStringList f;
        if (c & 0x00000020) f << "CODE";
        if (c & 0x00000040) f << "IDATA";
        if (c & 0x00000080) f << "UDATA";
        if (c & 0x20000000) f << "X";
        if (c & 0x40000000) f << "R";
        if (c & 0x80000000) f << "W";
        return f.isEmpty() ? "-" : f.join("|");
    };

    m_peSections.clear();

    for (int i = 0; i < numSections; ++i) {
        int sOff = firstSectionOff + i * 40;
        if (sOff + 40 > data.size()) break;

        // Name: 8 bytes, null-padded
        char nameBuf[9] = {};
        memcpy(nameBuf, data.constData() + sOff, 8);
        QString name = QString::fromLatin1(nameBuf).trimmed();

        quint32 virtualSize    = u32(sOff + 8);
        quint32 virtualAddress = u32(sOff + 12);
        quint32 rawSize        = u32(sOff + 16);
        quint32 rawOffset      = u32(sOff + 20);
        quint32 characteristics = u32(sOff + 36);

        // Store for RVA→offset mapping
        m_peSections.append({ virtualAddress, virtualSize, rawOffset, rawSize });

        addSectionRow(name,
                      QString("0x%1").arg(virtualAddress, 8, 16, QChar('0')),
                      QString("0x%1").arg(rawOffset,      8, 16, QChar('0')),
                      formatSize(rawSize),
                      secFlags(characteristics));
    }
}

void BinaryInspectorWidget::parsePEImports(const QByteArray &data,
                                            quint32 importRVA,
                                            bool pe32plus)
{
    auto u32 = [&](int off) -> quint32 {
        if (off < 0 || off + 4 > data.size()) return 0;
        quint32 v; memcpy(&v, data.constData() + off, 4);
        return qFromLittleEndian(v);
    };
    auto u64 = [&](int off) -> quint64 {
        if (off < 0 || off + 8 > data.size()) return 0;
        quint64 v; memcpy(&v, data.constData() + off, 8);
        return qFromLittleEndian(v);
    };
    auto readCStr = [&](int off) -> QString {
        if (off < 0 || off >= data.size()) return QString();
        int end = off;
        while (end < data.size() && data[end] != 0) ++end;
        return QString::fromLatin1(data.constData() + off, end - off);
    };

    int descOff = rvaToOffset(importRVA);
    if (descOff < 0) return;

    const int kMaxDlls    = 64;
    const int kMaxSymsPerDll = 200;
    int dllCount = 0;

    // Walk IMAGE_IMPORT_DESCRIPTOR array (20 bytes each, ends with all-zero entry)
    while (descOff + 20 <= data.size() && dllCount < kMaxDlls) {
        quint32 oft      = u32(descOff);       // OriginalFirstThunk
        quint32 nameRVA  = u32(descOff + 12);  // Name (RVA)
        quint32 ft       = u32(descOff + 16);  // FirstThunk

        // Null terminator entry
        if (oft == 0 && nameRVA == 0 && ft == 0) break;

        descOff += 20;
        ++dllCount;

        if (nameRVA == 0) continue;
        int nameOff = rvaToOffset(nameRVA);
        if (nameOff < 0) continue;
        QString dllName = readCStr(nameOff);
        if (dllName.isEmpty()) continue;

        // Walk thunk array (use OFT if available, else FT)
        quint32 thunkRVA = (oft != 0) ? oft : ft;
        if (thunkRVA == 0) { addImportRow(dllName, "(no symbols)"); continue; }

        int thunkOff = rvaToOffset(thunkRVA);
        if (thunkOff < 0) { addImportRow(dllName, "(unresolvable thunks)"); continue; }

        int entrySize = pe32plus ? 8 : 4;
        int symCount = 0;
        while (thunkOff + entrySize <= data.size() && symCount < kMaxSymsPerDll) {
            quint64 thunkVal = pe32plus ? u64(thunkOff) : u32(thunkOff);
            thunkOff += entrySize;

            if (thunkVal == 0) break;

            bool importByOrdinal = pe32plus ? (thunkVal & 0x8000000000000000ULL) 
                                            : (thunkVal & 0x80000000);

            if (importByOrdinal) {
                // Import by ordinal
                addImportRow(dllName, QString("Ordinal #%1").arg(thunkVal & 0xFFFF));
            } else {
                // Import by name: thunkVal is RVA to IMAGE_IMPORT_BY_NAME
                int ibnOff = rvaToOffset((quint32)thunkVal);
                if (ibnOff >= 0 && ibnOff + 2 < data.size()) {
                    // Skip the 2-byte Hint field
                    QString sym = readCStr(ibnOff + 2);
                    if (!sym.isEmpty())
                        addImportRow(dllName, sym);
                }
            }
            ++symCount;
        }
        if (symCount == 0)
            addImportRow(dllName, "(no named imports)");
    }
}

int BinaryInspectorWidget::rvaToOffset(quint32 rva) const
{
    for (const auto &sec : m_peSections) {
        if (rva >= sec.virtualAddress &&
            rva <  sec.virtualAddress + qMax(sec.virtualSize, sec.rawSize)) {
            int off = (int)(sec.rawOffset + (rva - sec.virtualAddress));
            return off;
        }
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  String Extractor
// ─────────────────────────────────────────────────────────────────────────────
void BinaryInspectorWidget::extractStrings(const QByteArray &data)
{
    const int kMinLen   = 5;
    const int kMaxStrs  = 2000;
    QStringList results;

    int i = 0;
    while (i < data.size() && results.size() < kMaxStrs) {
        // Try ASCII run
        if ((quint8)data[i] >= 0x20 && (quint8)data[i] <= 0x7E) {
            int start = i;
            while (i < data.size() &&
                   (quint8)data[i] >= 0x20 && (quint8)data[i] <= 0x7E)
                ++i;
            int len = i - start;
            if (len >= kMinLen) {
                results << QString("0x%1  %2")
                               .arg((quint32)start, 8, 16, QChar('0'))
                               .arg(QString::fromLatin1(data.constData() + start, len));
            }
            continue;
        }

        // Try UTF-16LE run (letter, 0x00, letter, 0x00, …)
        if (i + 1 < data.size() &&
            (quint8)data[i] >= 0x20 && (quint8)data[i] <= 0x7E &&
            data[i + 1] == 0x00) {
            int start = i;
            QString utf16;
            while (i + 1 < data.size() &&
                   (quint8)data[i] >= 0x20 && (quint8)data[i] <= 0x7E &&
                   data[i + 1] == 0x00) {
                utf16 += QChar(data[i]);
                i += 2;
            }
            if (utf16.length() >= kMinLen) {
                results << QString("0x%1  [UTF-16] %2")
                               .arg((quint32)start, 8, 16, QChar('0'))
                               .arg(utf16);
            }
            continue;
        }

        ++i;
    }

    m_stringsView->setPlainText(results.join('\n'));
    m_tabs->setTabText(3, QString("Strings (%1)").arg(results.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Table / row helpers
// ─────────────────────────────────────────────────────────────────────────────
void BinaryInspectorWidget::addHeaderRow(const QString &field,
                                          const QString &value,
                                          const QString &desc)
{
    int row = m_headerTable->rowCount();
    m_headerTable->insertRow(row);

    auto makeItem = [](const QString &text, const QString &color,
                       bool bold = false) -> QTableWidgetItem * {
        auto *item = new QTableWidgetItem(text);
        item->setForeground(QColor(color));
        if (bold) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    };

    // Decide value colour: hex → orange, plain number → white, flag/name → teal
    QString valColor = kFg;
    if (value.startsWith("0x") || value.startsWith("0X"))
        valColor = kOrange;
    else if (value[0].isDigit() || (value.size() > 1 && value[0] == '-' && value[1].isDigit()))
        valColor = kYellow;
    else
        valColor = kGreen;

    m_headerTable->setItem(row, 0, makeItem(field, kFg,   true));
    m_headerTable->setItem(row, 1, makeItem(value, valColor));
    m_headerTable->setItem(row, 2, makeItem(desc,  kGrey));
    m_headerTable->setRowHeight(row, 22);
}

void BinaryInspectorWidget::addSectionRow(const QString &name,
                                           const QString &vaddr,
                                           const QString &offset,
                                           const QString &size,
                                           const QString &flags)
{
    int row = m_sectionsTable->rowCount();
    m_sectionsTable->insertRow(row);

    auto makeItem = [](const QString &text, const QString &color) -> QTableWidgetItem * {
        auto *item = new QTableWidgetItem(text);
        item->setForeground(QColor(color));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    };

    m_sectionsTable->setItem(row, 0, makeItem(name,   kYellow));
    m_sectionsTable->setItem(row, 1, makeItem(vaddr,  kOrange));
    m_sectionsTable->setItem(row, 2, makeItem(offset, kOrange));
    m_sectionsTable->setItem(row, 3, makeItem(size,   kFg));
    m_sectionsTable->setItem(row, 4, makeItem(flags,  kGreen));
    m_sectionsTable->setRowHeight(row, 22);
}

void BinaryInspectorWidget::addImportRow(const QString &library,
                                          const QString &symbol)
{
    int row = m_importsTable->rowCount();
    m_importsTable->insertRow(row);

    auto makeItem = [](const QString &text, const QString &color) -> QTableWidgetItem * {
        auto *item = new QTableWidgetItem(text);
        item->setForeground(QColor(color));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        return item;
    };

    m_importsTable->setItem(row, 0, makeItem(library, kAccent));
    m_importsTable->setItem(row, 1, makeItem(symbol,  kFg));
    m_importsTable->setRowHeight(row, 22);
}

void BinaryInspectorWidget::clearTables()
{
    m_headerTable  ->setRowCount(0);
    m_sectionsTable->setRowCount(0);
    m_importsTable ->setRowCount(0);
    m_stringsView  ->clear();
    m_tabs->setTabText(3, "Strings");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Static helpers
// ─────────────────────────────────────────────────────────────────────────────
QString BinaryInspectorWidget::formatSize(qint64 bytes)
{
    if (bytes < 1024)
        return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QString("%1 KB (%2 B)").arg(bytes / 1024).arg(bytes);
    if (bytes < 1024LL * 1024 * 1024)
        return QString("%1 MB (%2 B)").arg(bytes / (1024 * 1024)).arg(bytes);
    return QString("%1 GB").arg(bytes / (1024LL * 1024 * 1024));
}

QString BinaryInspectorWidget::computeMD5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}