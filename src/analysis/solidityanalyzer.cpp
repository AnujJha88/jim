#include "solidityanalyzer.h"
#include <QByteArray>
#include <QRegularExpression>
#include <QUrl>
#include <QStringList>
#include <QMap>

// ─── Internal helpers ────────────────────────────────────────────────────────

// Multiply a big-integer expressed as a decimal string by a small integer n.
static QString bigMulInt(const QString &num, qint64 n)
{
    if (n == 0) return QStringLiteral("0");
    QVector<int> digits;
    digits.reserve(num.size() + 4);
    for (QChar c : num) digits.append(c.digitValue());

    qint64 carry = 0;
    for (int i = digits.size() - 1; i >= 0; --i) {
        qint64 prod = static_cast<qint64>(digits[i]) * n + carry;
        digits[i]   = static_cast<int>(prod % 10);
        carry       = prod / 10;
    }
    while (carry > 0) {
        digits.prepend(static_cast<int>(carry % 10));
        carry /= 10;
    }

    QString result;
    result.reserve(digits.size());
    for (int d : digits) result += QChar(QLatin1Char('0' + d));
    return result;
}

// Insert thousands separators into an all-digit string.
static QString addThousandsSep(const QString &num)
{
    QString result;
    result.reserve(num.size() + num.size() / 3);
    int count = 0;
    for (int i = num.size() - 1; i >= 0; --i) {
        if (count > 0 && count % 3 == 0) result.prepend(QLatin1Char(','));
        result.prepend(num[i]);
        ++count;
    }
    return result;
}

// Return byte size for a Solidity type token (stripped of modifiers).
static int solidityTypeByteSize(const QString &type)
{
    if (type == QLatin1String("bool"))    return 1;
    if (type == QLatin1String("address")) return 20;

    if (type.startsWith(QLatin1String("uint"))) {
        QString bits = type.mid(4);
        if (bits.isEmpty()) return 32;          // uint = uint256
        int b = bits.toInt();
        return (b > 0) ? b / 8 : 32;
    }
    if (type.startsWith(QLatin1String("int"))) {
        QString bits = type.mid(3);
        if (bits.isEmpty()) return 32;          // int = int256
        int b = bits.toInt();
        return (b > 0) ? b / 8 : 32;
    }
    if (type.startsWith(QLatin1String("bytes"))) {
        QString n = type.mid(5);
        if (n.isEmpty()) return 32;             // dynamic bytes -> 32 (new slot)
        int sz = n.toInt();
        return (sz > 0 && sz <= 32) ? sz : 32;
    }
    // string, mapping(...), struct ... -> 32 (occupies own slot)
    return 32;
}

// True for types that always occupy a full dedicated slot (dynamic / reference).
static bool isDynamicType(const QString &type)
{
    if (type == QLatin1String("string") || type == QLatin1String("bytes")) return true;
    if (type.startsWith(QLatin1String("mapping"))) return true;
    if (type.startsWith(QLatin1String("struct")))  return true;
    return false;
}

// ─── parseStorageSlots ───────────────────────────────────────────────────────

QVector<SolidityAnalyzer::StorageVar> SolidityAnalyzer::parseStorageSlots(const QString &code)
{
    QVector<StorageVar> vars;
    QStringList lines = code.split(QLatin1Char('\n'));

    // Matches:  <type>  [modifiers]  <name>  [= ...] ;
    // Type covers: uint/int variants, bool, address, bytesN/bytes, string,
    //              mapping(...), struct Foo
    QRegularExpression stateVarRe(
        QStringLiteral(
            "^\\s*"
            "(uint\\d*|int\\d*|bool|address|bytes\\d*|string|bytes"
            "|mapping\\s*\\([^)]*\\)"
            "|struct\\s+\\w+)"
            "\\s+"
            "(?:(?:public|private|internal|constant|immutable|override)\\s+)*"
            "(\\w+)"
            "\\s*(?:=.*)?;"
        ));

    int braceDepth = 0;
    int currentSlot   = 0;
    int currentOffset = 0; // bytes consumed in the current slot

    for (int lineNum = 0; lineNum < lines.size(); ++lineNum) {
        const QString &line = lines[lineNum];

        // Record depth *before* counting braces on this line.
        int depthBefore = braceDepth;
        for (QChar c : line) {
            if (c == QLatin1Char('{'))      ++braceDepth;
            else if (c == QLatin1Char('}')) --braceDepth;
        }

        // State variables live at depth 1 (inside contract, outside any function).
        if (depthBefore != 1) continue;

        QRegularExpressionMatch m = stateVarRe.match(line);
        if (!m.hasMatch()) continue;

        QString typeName = m.captured(1).trimmed();
        QString varName  = m.captured(2);

        int  size    = solidityTypeByteSize(typeName);
        bool dynamic = isDynamicType(typeName) || (size == 32);

        StorageVar var;
        var.name     = varName;
        var.typeName = typeName;
        var.byteSize = size;
        var.srcLine  = lineNum;
        var.isPacked = false;

        if (dynamic) {
            // Dynamic / 32-byte types always start on a fresh slot.
            if (currentOffset > 0) { ++currentSlot; currentOffset = 0; }
            var.slot       = currentSlot;
            var.byteOffset = 0;
            ++currentSlot;
            currentOffset = 0;
        } else {
            // Try greedy packing.
            if (currentOffset + size <= 32) {
                var.slot       = currentSlot;
                var.byteOffset = currentOffset;
                currentOffset += size;
                if (currentOffset == 32) { ++currentSlot; currentOffset = 0; }
            } else {
                // Doesn't fit → new slot.
                ++currentSlot;
                currentOffset = 0;
                var.slot       = currentSlot;
                var.byteOffset = 0;
                currentOffset  = size;
            }
        }
        vars.append(var);
    }

    // Mark isPacked: true when a slot is shared by more than one variable.
    QMap<int, int> slotCount;
    for (const StorageVar &v : vars) slotCount[v.slot]++;
    for (StorageVar &v : vars) {
        if (slotCount[v.slot] > 1) v.isPacked = true;
    }

    return vars;
}

// ─── detectReentrancy ────────────────────────────────────────────────────────

QVector<SolidityAnalyzer::ReentrancyFinding>
SolidityAnalyzer::detectReentrancy(const QString &code)
{
    QVector<ReentrancyFinding> findings;
    QStringList lines = code.split(QLatin1Char('\n'));

    // Patterns that indicate an external call / ETH transfer.
    const QStringList callPatterns = {
        QStringLiteral(".call{value:"),
        QStringLiteral(".call.value("),
        QStringLiteral("transfer("),
        QStringLiteral("send(")
    };

    // Simple state-change detector: assignments, compound assignments, mapping writes.
    QRegularExpression stateChangeRe(
        QStringLiteral("\\b\\w+\\s*(?:\\[.*\\]\\s*)?(?:\\+=|-=|\\*=|/=|=)(?!=)"
                       "|\\bmapping\\s*\\["));

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];

        QString matchedCall;
        for (const QString &pat : callPatterns) {
            if (line.contains(pat)) { matchedCall = pat; break; }
        }
        if (matchedCall.isEmpty()) continue;

        // Look ahead up to 20 lines for a state change.
        const int horizon = qMin(i + 20, lines.size() - 1);
        for (int j = i + 1; j <= horizon; ++j) {
            if (stateChangeRe.match(lines[j]).hasMatch()) {
                ReentrancyFinding f;
                f.externalCallLine    = i;
                f.vulnerableStateLine = j;
                f.callExpression      = matchedCall;
                f.description = QString(
                    "Reentrancy: external call (%1) on line %2 "
                    "followed by state change on line %3 — "
                    "apply Checks-Effects-Interactions pattern.")
                    .arg(matchedCall).arg(i + 1).arg(j + 1);
                findings.append(f);
                break; // One finding per external call is enough.
            }
        }
    }
    return findings;
}

// ─── detectStateShadowing ────────────────────────────────────────────────────

QVector<SolidityAnalyzer::ShadowingFinding>
SolidityAnalyzer::detectStateShadowing(const QString &code)
{
    QVector<ShadowingFinding> findings;
    QStringList lines = code.split(QLatin1Char('\n'));

    // Step 1 — collect state variable names (depth == 1).
    QRegularExpression stateVarRe(
        QStringLiteral(
            "^\\s*(uint\\d*|int\\d*|bool|address|bytes\\d*|string|mapping\\S*)"
            "\\s+(?:(?:public|private|internal|constant|immutable)\\s+)*(\\w+)"));

    QStringList stateNames;
    {
        int depth = 0;
        for (int i = 0; i < lines.size(); ++i) {
            int before = depth;
            for (QChar c : lines[i]) {
                if (c == QLatin1Char('{'))      ++depth;
                else if (c == QLatin1Char('}')) --depth;
            }
            if (before != 1) continue;
            QRegularExpressionMatch m = stateVarRe.match(lines[i]);
            if (m.hasMatch()) stateNames.append(m.captured(2));
        }
    }
    if (stateNames.isEmpty()) return findings;

    // Step 2 — look for local declarations with the same name (depth >= 2).
    QRegularExpression localVarRe(
        QStringLiteral(
            "^\\s*(uint\\d*|int\\d*|bool|address|bytes\\d*|string)"
            "\\s+(?:memory\\s+|storage\\s+|calldata\\s+)?(\\w+)\\s*(?:=.*)?;"));

    {
        int depth = 0;
        for (int i = 0; i < lines.size(); ++i) {
            int before = depth;
            for (QChar c : lines[i]) {
                if (c == QLatin1Char('{'))      ++depth;
                else if (c == QLatin1Char('}')) --depth;
            }
            if (before < 2) continue;

            QRegularExpressionMatch m = localVarRe.match(lines[i]);
            if (!m.hasMatch()) continue;
            QString localName = m.captured(2);
            if (stateNames.contains(localName)) {
                ShadowingFinding f;
                f.line       = i;
                f.localVar   = localName;
                f.shadowedIn = QStringLiteral("state");
                f.description = QString(
                    "Local variable '%1' shadows a contract state variable.")
                    .arg(localName);
                findings.append(f);
            }
        }
    }
    return findings;
}

// ─── checkERCInterface ───────────────────────────────────────────────────────

QStringList SolidityAnalyzer::checkERCInterface(const QString &code,
                                                const QString &standard)
{
    struct FnEntry { QString name; QString sig; };
    QVector<FnEntry> required;

    if (standard == QLatin1String("ERC20")) {
        required = {
            { QStringLiteral("transfer"),     QStringLiteral("transfer(address,uint256)")            },
            { QStringLiteral("transferFrom"), QStringLiteral("transferFrom(address,address,uint256)") },
            { QStringLiteral("approve"),      QStringLiteral("approve(address,uint256)")              },
            { QStringLiteral("allowance"),    QStringLiteral("allowance(address,address)")            },
            { QStringLiteral("balanceOf"),    QStringLiteral("balanceOf(address)")                    },
            { QStringLiteral("totalSupply"),  QStringLiteral("totalSupply()")                         },
        };
    } else if (standard == QLatin1String("ERC721")) {
        required = {
            { QStringLiteral("ownerOf"),            QStringLiteral("ownerOf(uint256)")                        },
            { QStringLiteral("transferFrom"),       QStringLiteral("transferFrom(address,address,uint256)")    },
            { QStringLiteral("safeTransferFrom"),   QStringLiteral("safeTransferFrom(address,address,uint256)")},
            { QStringLiteral("approve"),            QStringLiteral("approve(address,uint256)")                 },
            { QStringLiteral("setApprovalForAll"),  QStringLiteral("setApprovalForAll(address,bool)")          },
            { QStringLiteral("isApprovedForAll"),   QStringLiteral("isApprovedForAll(address,address)")        },
            { QStringLiteral("balanceOf"),          QStringLiteral("balanceOf(address)")                       },
        };
    } else if (standard == QLatin1String("ERC1155")) {
        required = {
            { QStringLiteral("balanceOf"),            QStringLiteral("balanceOf(address,uint256)")                                         },
            { QStringLiteral("balanceOfBatch"),       QStringLiteral("balanceOfBatch(address[],uint256[])")                                 },
            { QStringLiteral("safeTransferFrom"),     QStringLiteral("safeTransferFrom(address,address,uint256,uint256,bytes)")             },
            { QStringLiteral("safeBatchTransferFrom"),QStringLiteral("safeBatchTransferFrom(address,address,uint256[],uint256[],bytes)")    },
            { QStringLiteral("setApprovalForAll"),    QStringLiteral("setApprovalForAll(address,bool)")                                    },
            { QStringLiteral("isApprovedForAll"),     QStringLiteral("isApprovedForAll(address,address)")                                  },
        };
    }

    QStringList missing;
    for (const FnEntry &fn : required) {
        // Accept either a function definition or an external call of the same name.
        QRegularExpression fnRe(
            QString(QStringLiteral("\\bfunction\\s+%1\\s*\\(|\\b%1\\s*\\("))
            .arg(QRegularExpression::escape(fn.name)));
        if (!fnRe.match(code).hasMatch())
            missing.append(fn.sig);
    }
    return missing;
}

// ─── buildGasTable / evmGasCost ──────────────────────────────────────────────

QMap<QString, int> SolidityAnalyzer::buildGasTable()
{
    QMap<QString, int> t;
    // Arithmetic
    t[QStringLiteral("ADD")]        = 3;
    t[QStringLiteral("MUL")]        = 5;
    t[QStringLiteral("SUB")]        = 3;
    t[QStringLiteral("DIV")]        = 5;
    t[QStringLiteral("SDIV")]       = 5;
    t[QStringLiteral("MOD")]        = 5;
    t[QStringLiteral("SMOD")]       = 5;
    t[QStringLiteral("ADDMOD")]     = 8;
    t[QStringLiteral("MULMOD")]     = 8;
    t[QStringLiteral("EXP")]        = 50;   // base cost; per-byte is extra
    t[QStringLiteral("SIGNEXTEND")] = 5;
    // Comparison / bitwise
    t[QStringLiteral("LT")]         = 3;
    t[QStringLiteral("GT")]         = 3;
    t[QStringLiteral("SLT")]        = 3;
    t[QStringLiteral("SGT")]        = 3;
    t[QStringLiteral("EQ")]         = 3;
    t[QStringLiteral("ISZERO")]     = 3;
    t[QStringLiteral("AND")]        = 3;
    t[QStringLiteral("OR")]         = 3;
    t[QStringLiteral("XOR")]        = 3;
    t[QStringLiteral("NOT")]        = 3;
    t[QStringLiteral("BYTE")]       = 3;
    t[QStringLiteral("SHL")]        = 3;
    t[QStringLiteral("SHR")]        = 3;
    t[QStringLiteral("SAR")]        = 3;
    // Memory
    t[QStringLiteral("MLOAD")]      = 3;
    t[QStringLiteral("MSTORE")]     = 3;
    t[QStringLiteral("MSTORE8")]    = 3;
    t[QStringLiteral("MSIZE")]      = 2;
    t[QStringLiteral("MCOPPY")]     = 3;    // EIP-5656
    // Storage (warm access — post EIP-2929 representative cost)
    t[QStringLiteral("SLOAD")]      = 100;
    t[QStringLiteral("SSTORE")]     = 100;
    t[QStringLiteral("TLOAD")]      = 100;  // EIP-1153 transient
    t[QStringLiteral("TSTORE")]     = 100;
    // Hashing
    t[QStringLiteral("SHA3")]       = 30;   // base cost; +6/word
    t[QStringLiteral("KECCAK256")]  = 30;
    // Environment / block
    t[QStringLiteral("ADDRESS")]    = 2;
    t[QStringLiteral("BALANCE")]    = 100;  // warm; cold = 2600
    t[QStringLiteral("ORIGIN")]     = 2;
    t[QStringLiteral("CALLER")]     = 2;
    t[QStringLiteral("CALLVALUE")]  = 2;
    t[QStringLiteral("CALLDATALOAD")]  = 3;
    t[QStringLiteral("CALLDATASIZE")]  = 2;
    t[QStringLiteral("CALLDATACOPY")] = 3;  // base; +3/word
    t[QStringLiteral("CODESIZE")]   = 2;
    t[QStringLiteral("CODECOPY")]   = 3;
    t[QStringLiteral("GASPRICE")]   = 2;
    t[QStringLiteral("EXTCODESIZE")]= 100;  // warm; cold = 2600
    t[QStringLiteral("EXTCODECOPY")]= 100;
    t[QStringLiteral("EXTCODEHASH")]= 100;
    t[QStringLiteral("RETURNDATASIZE")] = 2;
    t[QStringLiteral("RETURNDATACOPY")] = 3;
    t[QStringLiteral("BLOCKHASH")]  = 20;
    t[QStringLiteral("COINBASE")]   = 2;
    t[QStringLiteral("TIMESTAMP")]  = 2;
    t[QStringLiteral("NUMBER")]     = 2;
    t[QStringLiteral("DIFFICULTY")] = 2;
    t[QStringLiteral("PREVRANDAO")] = 2;
    t[QStringLiteral("GASLIMIT")]   = 2;
    t[QStringLiteral("CHAINID")]    = 2;
    t[QStringLiteral("SELFBALANCE")]= 5;
    t[QStringLiteral("BASEFEE")]    = 2;
    t[QStringLiteral("BLOBHASH")]   = 3;
    t[QStringLiteral("BLOBBASEFEE")]= 2;
    // Control flow
    t[QStringLiteral("JUMP")]       = 8;
    t[QStringLiteral("JUMPI")]      = 10;
    t[QStringLiteral("PC")]         = 2;
    t[QStringLiteral("GAS")]        = 2;
    t[QStringLiteral("JUMPDEST")]   = 1;
    t[QStringLiteral("STOP")]       = 0;
    t[QStringLiteral("RETURN")]     = 0;
    t[QStringLiteral("REVERT")]     = 0;
    t[QStringLiteral("INVALID")]    = 0;
    t[QStringLiteral("SELFDESTRUCT")]= 5000;
    // Calls
    t[QStringLiteral("CALL")]           = 2600;
    t[QStringLiteral("CALLCODE")]       = 2600;
    t[QStringLiteral("DELEGATECALL")]   = 2600;
    t[QStringLiteral("STATICCALL")]     = 2600;
    t[QStringLiteral("CREATE")]         = 32000;
    t[QStringLiteral("CREATE2")]        = 32000;
    // Stack / push / dup / swap
    t[QStringLiteral("POP")]    = 2;
    t[QStringLiteral("PUSH0")]  = 2;
    t[QStringLiteral("PUSH1")]  = 3;
    t[QStringLiteral("PUSH2")]  = 3;
    t[QStringLiteral("PUSH32")] = 3;
    t[QStringLiteral("DUP1")]   = 3;
    t[QStringLiteral("DUP2")]   = 3;
    t[QStringLiteral("SWAP1")]  = 3;
    t[QStringLiteral("SWAP2")]  = 3;
    // Logging
    t[QStringLiteral("LOG0")] = 375;
    t[QStringLiteral("LOG1")] = 750;
    t[QStringLiteral("LOG2")] = 1125;
    t[QStringLiteral("LOG3")] = 1500;
    t[QStringLiteral("LOG4")] = 1875;
    return t;
}

int SolidityAnalyzer::evmGasCost(const QString &opcode)
{
    static const QMap<QString, int> gasTable = buildGasTable();
    auto it = gasTable.constFind(opcode.toUpper());
    return (it != gasTable.constEnd()) ? it.value() : -1;
}

// ─── expandNumericLiteral ────────────────────────────────────────────────────

QString SolidityAnalyzer::expandNumericLiteral(const QString &token)
{
    // e-notation: 1e18, 2e6, etc.
    QRegularExpression eRe(QStringLiteral("^(\\d+)[eE](\\d+)$"));
    // power notation: 10**18, 2**256, etc.
    QRegularExpression powRe(QStringLiteral("^(\\d+)\\*\\*(\\d+)$"));

    QRegularExpressionMatch m = eRe.match(token);
    if (m.hasMatch()) {
        qint64 base    = m.captured(1).toLongLong();
        int    exp     = m.captured(2).toInt();
        // Clamp to a sane maximum to avoid absurdly long strings.
        if (exp > 100) return QString();
        // base * 10^exp  ==  base string followed by exp zeros.
        QString result = QString::number(base) + QString(exp, QLatin1Char('0'));
        return addThousandsSep(result);
    }

    m = powRe.match(token);
    if (m.hasMatch()) {
        qint64 base = m.captured(1).toLongLong();
        int    exp  = m.captured(2).toInt();
        if (exp > 512 || base < 0) return QString();
        // Compute base^exp via repeated big-integer multiplication.
        QString result = QStringLiteral("1");
        for (int i = 0; i < exp; ++i)
            result = bigMulInt(result, base);
        return addThousandsSep(result);
    }

    return QString();
}

// ─── tryDecode ───────────────────────────────────────────────────────────────

QString SolidityAnalyzer::tryDecode(const QString &token)
{
    // ── Hex ──────────────────────────────────────────────────────────────────
    if (token.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
        QString hexPart = token.mid(2);
        QRegularExpression hexChars(QStringLiteral("^[0-9a-fA-F]+$"));
        if (hexChars.match(hexPart).hasMatch() && hexPart.size() % 2 == 0) {
            QByteArray bytes = QByteArray::fromHex(hexPart.toLatin1());
            QString ascii;
            ascii.reserve(bytes.size() * 3);
            for (unsigned char c : bytes) {
                if (c >= 32 && c < 127)
                    ascii += QChar(static_cast<char>(c));
                else
                    ascii += QString(QStringLiteral("\\x%1"))
                                 .arg(c, 2, 16, QLatin1Char('0'));
            }
            return QStringLiteral("hex: ") + ascii;
        }
    }

    // ── Base64 ───────────────────────────────────────────────────────────────
    {
        QRegularExpression b64Re(QStringLiteral("^[A-Za-z0-9+/]+=*$"));
        // Require at least 4 chars and a length that is a multiple of 4
        if (token.size() >= 4 && token.size() % 4 == 0
                && b64Re.match(token).hasMatch()) {
            QByteArray decoded = QByteArray::fromBase64(token.toLatin1());
            if (!decoded.isEmpty()) {
                bool printable = true;
                for (char c : decoded) {
                    if (static_cast<unsigned char>(c) < 32
                            || static_cast<unsigned char>(c) >= 127) {
                        printable = false;
                        break;
                    }
                }
                if (printable)
                    return QStringLiteral("base64: ") + QString::fromLatin1(decoded);
            }
        }
    }

    // ── URL percent-encoding ─────────────────────────────────────────────────
    if (token.contains(QLatin1Char('%'))) {
        QString decoded = QUrl::fromPercentEncoding(token.toUtf8());
        if (decoded != token && !decoded.isEmpty())
            return QStringLiteral("url: ") + decoded;
    }

    return QString();
}

// ─── evmOpcodeTooltipHtml ────────────────────────────────────────────────────

QString SolidityAnalyzer::evmOpcodeTooltipHtml(const QString &opcode)
{
    // Brief human-readable descriptions keyed by uppercase opcode name.
    static const QMap<QString, QString> desc = {
        { QStringLiteral("ADD"),         QStringLiteral("Pop a, b → push a+b (mod 2^256)")                              },
        { QStringLiteral("MUL"),         QStringLiteral("Pop a, b → push a*b (mod 2^256)")                              },
        { QStringLiteral("SUB"),         QStringLiteral("Pop a, b → push a-b (mod 2^256)")                              },
        { QStringLiteral("DIV"),         QStringLiteral("Unsigned integer division. Returns 0 if divisor is 0.")         },
        { QStringLiteral("SDIV"),        QStringLiteral("Signed integer division (two's complement).")                   },
        { QStringLiteral("MOD"),         QStringLiteral("Unsigned modulo. Returns 0 if modulus is 0.")                   },
        { QStringLiteral("SMOD"),        QStringLiteral("Signed modulo (two's complement).")                             },
        { QStringLiteral("ADDMOD"),      QStringLiteral("Pop a, b, N → push (a+b) mod N. 0 if N=0.")                   },
        { QStringLiteral("MULMOD"),      QStringLiteral("Pop a, b, N → push (a*b) mod N. 0 if N=0.")                   },
        { QStringLiteral("EXP"),         QStringLiteral("Pop base, exp → push base^exp. Cost: 50 + 50 per byte of exp.")},
        { QStringLiteral("SIGNEXTEND"),  QStringLiteral("Sign-extend x from (b+1)*8 bits to 256 bits.")                 },
        { QStringLiteral("LT"),          QStringLiteral("Pop a, b → push 1 if a < b (unsigned), else 0.")               },
        { QStringLiteral("GT"),          QStringLiteral("Pop a, b → push 1 if a > b (unsigned), else 0.")               },
        { QStringLiteral("SLT"),         QStringLiteral("Signed less-than comparison.")                                  },
        { QStringLiteral("SGT"),         QStringLiteral("Signed greater-than comparison.")                               },
        { QStringLiteral("EQ"),          QStringLiteral("Equality comparison. Pushes 1 if equal, 0 otherwise.")          },
        { QStringLiteral("ISZERO"),      QStringLiteral("Pushes 1 if top of stack is zero, 0 otherwise.")                },
        { QStringLiteral("AND"),         QStringLiteral("Bitwise AND of top two stack items.")                           },
        { QStringLiteral("OR"),          QStringLiteral("Bitwise OR of top two stack items.")                            },
        { QStringLiteral("XOR"),         QStringLiteral("Bitwise XOR of top two stack items.")                           },
        { QStringLiteral("NOT"),         QStringLiteral("Bitwise NOT (one's complement) of top stack item.")             },
        { QStringLiteral("BYTE"),        QStringLiteral("Get byte i of value x (big-endian, byte 0 = MSB).")             },
        { QStringLiteral("SHL"),         QStringLiteral("Logical left shift by shift positions.")                        },
        { QStringLiteral("SHR"),         QStringLiteral("Logical right shift by shift positions.")                       },
        { QStringLiteral("SAR"),         QStringLiteral("Arithmetic (signed) right shift.")                              },
        { QStringLiteral("KECCAK256"),   QStringLiteral("Keccak-256 hash of memory[offset..offset+size). Base 30 gas + 6/word.") },
        { QStringLiteral("SHA3"),        QStringLiteral("Alias for KECCAK256.")                                          },
        { QStringLiteral("ADDRESS"),     QStringLiteral("Push the address of the currently executing account.")          },
        { QStringLiteral("BALANCE"),     QStringLiteral("Get ETH balance of address. Warm: 100 gas, Cold: 2600 gas.")    },
        { QStringLiteral("ORIGIN"),      QStringLiteral("Push the address of the original transaction sender (tx.origin).") },
        { QStringLiteral("CALLER"),      QStringLiteral("Push the caller address (msg.sender).")                         },
        { QStringLiteral("CALLVALUE"),   QStringLiteral("Push msg.value in wei.")                                        },
        { QStringLiteral("CALLDATALOAD"),QStringLiteral("Load 32-byte word from calldata at offset.")                   },
        { QStringLiteral("CALLDATASIZE"),QStringLiteral("Push size of calldata in bytes.")                               },
        { QStringLiteral("CALLDATACOPY"),QStringLiteral("Copy calldata to memory. 3 gas + 3 per 32-byte word.")         },
        { QStringLiteral("CODESIZE"),    QStringLiteral("Push size of the currently executing code.")                    },
        { QStringLiteral("CODECOPY"),    QStringLiteral("Copy code to memory. 3 gas base + 3 per word.")                 },
        { QStringLiteral("GASPRICE"),    QStringLiteral("Push the gas price of the transaction (in wei/gas).")           },
        { QStringLiteral("EXTCODESIZE"), QStringLiteral("Get code size of external account. Warm: 100, Cold: 2600.")     },
        { QStringLiteral("EXTCODECOPY"), QStringLiteral("Copy external account code to memory. Warm: 100, Cold: 2600.") },
        { QStringLiteral("EXTCODEHASH"), QStringLiteral("Keccak-256 hash of external account's code. Warm/Cold pricing.")},
        { QStringLiteral("RETURNDATASIZE"),QStringLiteral("Push the size of the return data from the last call.")       },
        { QStringLiteral("RETURNDATACOPY"),QStringLiteral("Copy return data into memory.")                              },
        { QStringLiteral("BLOCKHASH"),   QStringLiteral("Hash of one of the 256 most recent complete blocks.")           },
        { QStringLiteral("COINBASE"),    QStringLiteral("Push current block's beneficiary (miner/validator) address.")   },
        { QStringLiteral("TIMESTAMP"),   QStringLiteral("Push current block timestamp as Unix epoch seconds.")           },
        { QStringLiteral("NUMBER"),      QStringLiteral("Push current block number.")                                    },
        { QStringLiteral("DIFFICULTY"),  QStringLiteral("Push block difficulty / prevrandao (post-Merge).")              },
        { QStringLiteral("PREVRANDAO"),  QStringLiteral("Post-Merge: push beacon chain randomness (PREVRANDAO).")        },
        { QStringLiteral("GASLIMIT"),    QStringLiteral("Push the block's gas limit.")                                   },
        { QStringLiteral("CHAINID"),     QStringLiteral("Push the chain ID (EIP-1344).")                                 },
        { QStringLiteral("SELFBALANCE"), QStringLiteral("Push balance of the executing contract (cheaper than BALANCE).") },
        { QStringLiteral("BASEFEE"),     QStringLiteral("Push current block base fee per gas (EIP-1559).")               },
        { QStringLiteral("MLOAD"),       QStringLiteral("Load 32-byte word from memory at offset.")                      },
        { QStringLiteral("MSTORE"),      QStringLiteral("Store 32-byte word to memory at offset.")                       },
        { QStringLiteral("MSTORE8"),     QStringLiteral("Store a single byte to memory at offset.")                      },
        { QStringLiteral("MSIZE"),       QStringLiteral("Push the size of active memory in bytes.")                      },
        { QStringLiteral("SLOAD"),       QStringLiteral("Load word from storage. Warm: 100 gas, Cold: 2100 gas.")        },
        { QStringLiteral("SSTORE"),      QStringLiteral("Save word to storage. New slot: 20000 gas, Modify: 5000, Warm: 100.") },
        { QStringLiteral("TLOAD"),       QStringLiteral("Load from transient storage (EIP-1153). 100 gas.")              },
        { QStringLiteral("TSTORE"),      QStringLiteral("Write to transient storage (EIP-1153). 100 gas.")               },
        { QStringLiteral("JUMP"),        QStringLiteral("Unconditional jump to destination JUMPDEST.")                   },
        { QStringLiteral("JUMPI"),       QStringLiteral("Conditional jump: branch if condition != 0.")                   },
        { QStringLiteral("PC"),          QStringLiteral("Push the program counter before this instruction.")             },
        { QStringLiteral("GAS"),         QStringLiteral("Push remaining gas (after cost of this instruction).")          },
        { QStringLiteral("JUMPDEST"),    QStringLiteral("Mark valid jump destination. Costs 1 gas.")                     },
        { QStringLiteral("STOP"),        QStringLiteral("Halt execution successfully. 0 gas.")                           },
        { QStringLiteral("RETURN"),      QStringLiteral("Return memory[offset..offset+size) and halt. 0 gas.")           },
        { QStringLiteral("REVERT"),      QStringLiteral("Revert with reason data. State changes undone. 0 gas.")         },
        { QStringLiteral("INVALID"),     QStringLiteral("Designated invalid instruction; consumes all gas.")             },
        { QStringLiteral("SELFDESTRUCT"),QStringLiteral("Destroy contract and send balance to target. 5000+ gas.")       },
        { QStringLiteral("CALL"),        QStringLiteral("Call into another account. Warm: 2600 gas base + value/new account costs.") },
        { QStringLiteral("DELEGATECALL"),QStringLiteral("Like CALL but preserves caller's msg.sender and msg.value. 2600 gas.") },
        { QStringLiteral("STATICCALL"),  QStringLiteral("Like CALL but disallows state modifications. 2600 gas.")        },
        { QStringLiteral("CALLCODE"),    QStringLiteral("Deprecated. Like DELEGATECALL but updates msg.sender. 2600 gas.")},
        { QStringLiteral("CREATE"),      QStringLiteral("Create new contract from code. 32000 gas.")                     },
        { QStringLiteral("CREATE2"),     QStringLiteral("Create contract at deterministic address. 32000 gas.")           },
        { QStringLiteral("POP"),         QStringLiteral("Discard top stack item. 2 gas.")                                },
        { QStringLiteral("LOG0"),        QStringLiteral("Emit a log with no topics. 375 gas + data cost.")               },
        { QStringLiteral("LOG1"),        QStringLiteral("Emit a log with 1 topic. 750 gas + data cost.")                 },
        { QStringLiteral("LOG2"),        QStringLiteral("Emit a log with 2 topics. 1125 gas + data cost.")               },
        { QStringLiteral("LOG3"),        QStringLiteral("Emit a log with 3 topics. 1500 gas + data cost.")               },
        { QStringLiteral("LOG4"),        QStringLiteral("Emit a log with 4 topics. 1875 gas + data cost.")               },
    };

    QString upper = opcode.toUpper();
    int gas = evmGasCost(upper);
    if (gas < 0) return QString();   // Unknown opcode — no tooltip.

    QString description = desc.value(upper, QStringLiteral("EVM opcode."));
    QString gasStr = (gas == 0)
        ? QStringLiteral("<font color='#aaaaaa'>0</font>")
        : QString(QStringLiteral("<font color='#88ff88'>%1</font>")).arg(gas);

    return QString(
        QStringLiteral(
            "<b style='font-size:11pt'>%1</b>"
            "&nbsp;&nbsp;<small>gas:</small>&nbsp;%2"
            "<hr style='margin:4px 0'/>"
            "<span style='color:#cccccc'>%3</span>"
        ))
        .arg(upper, gasStr, description);
}
