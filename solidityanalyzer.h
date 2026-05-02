#ifndef SOLIDITYANALYZER_H
#define SOLIDITYANALYZER_H
#include <QString>
#include <QVector>
#include <QStringList>
#include <QMap>
class SolidityAnalyzer {
public:
    struct StorageVar {
        QString name;
        QString typeName;
        int slot;        // 32-byte slot number (0-indexed)
        int byteOffset;  // byte offset within slot (0-31)
        int byteSize;    // size in bytes
        bool isPacked;   // packed with another var in same slot
        int srcLine;     // source line number (0-indexed), -1 if unknown
    };
    struct ReentrancyFinding {
        int externalCallLine;    // 0-indexed line number
        int vulnerableStateLine; // 0-indexed, -1 if not found
        QString callExpression;  // e.g. ".call{value:"
        QString description;
    };
    struct ShadowingFinding {
        int line;            // 0-indexed
        QString localVar;
        QString shadowedIn;  // "state" or "inherited"
        QString description;
    };

    // Parse Solidity state variable declarations, compute slot layout
    static QVector<StorageVar> parseStorageSlots(const QString &code);

    // Detect reentrancy: external calls followed by state changes
    static QVector<ReentrancyFinding> detectReentrancy(const QString &code);

    // Detect state variable shadowing by local variables
    static QVector<ShadowingFinding> detectStateShadowing(const QString &code);

    // Check ERC interface compliance. standard = "ERC20" | "ERC721" | "ERC1155"
    // Returns list of missing required function signatures
    static QStringList checkERCInterface(const QString &code, const QString &standard);

    // Returns EVM gas cost for an opcode name (e.g. "SLOAD", "MSTORE", "KECCAK256")
    // Returns -1 if unknown. Case-insensitive.
    static int evmGasCost(const QString &opcode);

    // Expand numeric literal token to human-readable form
    // "1e18" -> "1,000,000,000,000,000,000"
    // "10**18" -> "1,000,000,000,000,000,000"
    // Returns empty string if not recognized
    static QString expandNumericLiteral(const QString &token);

    // Try to decode a token as Base64, Hex, or URL-encoded string
    // Returns decoded string, or empty if not decodable
    static QString tryDecode(const QString &token);

    // Returns EVM gas tooltip HTML for an opcode at the given position
    static QString evmOpcodeTooltipHtml(const QString &opcode);

private:
    static QMap<QString,int> buildGasTable();
};
#endif
