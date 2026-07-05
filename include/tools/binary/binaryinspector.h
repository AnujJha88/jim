#ifndef BINARYINSPECTOR_H
#define BINARYINSPECTOR_H

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QByteArray>
#include <QString>
#include <QVector>

// ─────────────────────────────────────────────────────────────────────────────
//  BinaryInspectorWidget
//  Parses ELF and PE binaries natively (no external tools required) and
//  presents headers, sections, imports and extracted strings in a tabbed view.
// ─────────────────────────────────────────────────────────────────────────────
class BinaryInspectorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BinaryInspectorWidget(QWidget *parent = nullptr);

    // Load a file from disk; returns false if the file could not be read.
    bool    loadFile(const QString &filePath);
    QString getFilePath() const { return m_filePath; }

private:
    // ── state ─────────────────────────────────────────────────────────────────
    QString m_filePath;

    // RVA → file-offset mapping (used by PE import parser)
    struct SectionMapping {
        quint32 virtualAddress;
        quint32 virtualSize;
        quint32 rawOffset;
        quint32 rawSize;
    };
    QVector<SectionMapping> m_peSections;   // populated during parsePE()

    // ── widgets ───────────────────────────────────────────────────────────────
    QLabel          *m_fileInfoLabel    = nullptr;
    QTabWidget      *m_tabs             = nullptr;
    QTableWidget    *m_headerTable      = nullptr;
    QTableWidget    *m_sectionsTable    = nullptr;
    QTableWidget    *m_importsTable     = nullptr;
    QPlainTextEdit  *m_stringsView      = nullptr;

    // ── setup ─────────────────────────────────────────────────────────────────
    void setupUI();

    // ── analysis entry points ─────────────────────────────────────────────────
    void analyzeFile(const QByteArray &data);

    // Returns true when the magic bytes indicate ELF / PE
    bool parseELF(const QByteArray &data);
    bool parsePE (const QByteArray &data);

    // Always called regardless of format
    void extractStrings(const QByteArray &data);

    // ── ELF helpers ───────────────────────────────────────────────────────────
    void parseELFSections  (const QByteArray &data, bool is64, bool le,
                             int shoff, int shentsize, int shnum, int shstrndx);
    void parseELFDynSymbols(const QByteArray &data, bool is64, bool le);

    // ── PE helpers ────────────────────────────────────────────────────────────
    void parsePESections(const QByteArray &data, int firstSectionOff,
                         int numSections);
    void parsePEImports (const QByteArray &data, quint32 importRVA,
                         bool pe32plus);

    int  rvaToOffset(quint32 rva) const;   // uses m_peSections

    // ── table helpers ─────────────────────────────────────────────────────────
    // Append one row to m_headerTable
    void addHeaderRow(const QString &field,
                      const QString &value,
                      const QString &desc = QString());

    // Append one row to m_sectionsTable
    void addSectionRow(const QString &name,
                       const QString &vaddr,
                       const QString &offset,
                       const QString &size,
                       const QString &flags);

    // Append one row to m_importsTable
    void addImportRow(const QString &library,
                      const QString &symbol);

    // ── misc helpers ──────────────────────────────────────────────────────────
    static QString formatSize   (qint64 bytes);
    static QString computeMD5   (const QByteArray &data);

    // Style helpers
    void styleTable  (QTableWidget *t);
    void clearTables ();
};

#endif // BINARYINSPECTOR_H