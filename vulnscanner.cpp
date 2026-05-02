#include "vulnscanner.h"
#include <QTextBlock>
#include <QTextDocument>

VulnScanner::VulnScanner(QObject *parent) : QObject(parent) {
    initRules();
}

void VulnScanner::initRules() {
    // General secrets
    generalRules.append({QRegularExpression("AKIA[0-9A-Z]{16}"), "SECRET", "Possible AWS Access Key ID"});
    generalRules.append({QRegularExpression("-----BEGIN (RSA |EC |OPENSSH )?PRIVATE KEY-----"), "SECRET", "Private Key block"});
    generalRules.append({QRegularExpression("ghp_[A-Za-z0-9]{36}"), "SECRET", "GitHub Personal Access Token"});
    generalRules.append({QRegularExpression("xox[baprs]-[0-9a-zA-Z]{10,48}"), "SECRET", "Slack API Token"});
    generalRules.append({QRegularExpression("(?i)(api_key|api_secret|password|passwd|secret|token)\\s*[=:]\\s*[\"']?[A-Za-z0-9+/=_-]{8,}"), "SECRET", "Hardcoded generic credential/token"});

    // C/C++ Dangerous Functions
    cppRules.append({QRegularExpression("\\b(gets|strcpy|strcat|sprintf|vsprintf)\\s*\\("), "DANGEROUS_FUNC", "Unsafe string manipulation (buffer overflow risk)"});
    cppRules.append({QRegularExpression("\\b(system|popen|execl|execle|execlp|execv|execve|execvp)\\s*\\("), "DANGEROUS_FUNC", "OS command execution (command injection risk)"});
    
    // Solidity Anti-patterns
    solidityRules.append({QRegularExpression("\\btx\\.origin\\b"), "SOLIDITY", "Use of tx.origin for authentication (Phishing risk)"});
    solidityRules.append({QRegularExpression("\\bdelegatecall\\b"), "SOLIDITY", "Unsafe delegatecall (State manipulation risk)"});
    solidityRules.append({QRegularExpression("\\b(selfdestruct|suicide)\\b"), "SOLIDITY", "Contract destruction function"});

    // Extended Solidity Anti-patterns
    solidityRules.append({QRegularExpression("\\bmsg\\.sender\\.call\\b"), "SOLIDITY", "Low-level .call() to msg.sender (reentrancy risk)"});
    solidityRules.append({QRegularExpression("\\bassembly\\s*\\{"), "SOLIDITY", "Inline assembly block"});
    solidityRules.append({QRegularExpression("\\bblock\\.timestamp\\b"), "SOLIDITY", "Timestamp dependence (miner manipulable)"});
    solidityRules.append({QRegularExpression("\\baddress\\(0\\)"), "SOLIDITY", "Potential zero-address check missing"});
    solidityRules.append({QRegularExpression("uint\\s+\\w+\\s*=\\s*\\w+\\s*/\\s*\\w+\\s*\\*"), "DIV_BEFORE_MUL", "Division before multiplication (precision loss)"});

    // ReDoS — catastrophic backtracking patterns (JS / Python files)
    // Nested quantifier: (group+)+ or (group*)* etc.
    redosRules.append({QRegularExpression(R"(\([^)]*[+*][^)]*\)[+*])"), "REDOS", "Nested quantifier in regex — catastrophic backtracking risk"});
    // Alternation with outer quantifier: (a|b)+ etc.
    redosRules.append({QRegularExpression(R"(\([^)]*\|[^)]*\)[+*])"), "REDOS", "Alternation with outer quantifier in regex — catastrophic backtracking risk"});
}

QVector<VulnScanner::Finding> VulnScanner::scan(const QString &text, Language lang, const QString &fileName) {
    QVector<Finding> findings;
    
    // We parse line-by-line to get accurate block numbers and columns.
    // Instead of instantiating a whole QTextDocument just for scanning,
    // we can split the text and scan line by line.
    
    QStringList lines = text.split('\n');
    bool isSolidity = fileName.endsWith(".sol", Qt::CaseInsensitive);

    for (int lineNum = 0; lineNum < lines.size(); ++lineNum) {
        const QString &line = lines[lineNum];
        if (line.trimmed().isEmpty()) continue;

        auto checkRules = [&](const QVector<Rule> &rules) {
            for (const Rule &rule : rules) {
                QRegularExpressionMatchIterator i = rule.regex.globalMatch(line);
                while (i.hasNext()) {
                    QRegularExpressionMatch match = i.next();
                    Finding f;
                    f.line = lineNum;
                    f.colStart = match.capturedStart();
                    f.colEnd = match.capturedEnd();
                    f.category = rule.category;
                    f.description = rule.description;
                    findings.append(f);
                }
            }
        };

        checkRules(generalRules);
        
        if (lang == Language::CPP) {
            checkRules(cppRules);
        }
        
        if (isSolidity) {
            checkRules(solidityRules);
        }

        if (lang == Language::JavaScript || lang == Language::Python) {
            checkRules(redosRules);
        }
    }

    return findings;
}

// ---------------------------------------------------------------------------
// scanSolidity — higher-level analysis that wraps scan() and adds:
//   * reentrancy detection  (external-call followed by state mutation)
//   * state-variable shadowing
//   * ERC-20 / ERC-721 interface completeness
//   * division-before-multiplication (expression-level)
// ---------------------------------------------------------------------------
QVector<VulnScanner::Finding> VulnScanner::scanSolidity(const QString &text, const QString &fileName)
{
    // Start with the base scan (uses solidityRules, generalRules, etc.)
    QVector<Finding> findings = scan(text, Language::PlainText, fileName);

    QStringList lines = text.split('\n');
    const int lineCount = lines.size();

    // -----------------------------------------------------------------------
    // 1. Reentrancy detection
    //    Look for external call patterns; if a state-change assignment appears
    //    within the next 20 lines, flag it.
    // -----------------------------------------------------------------------
    QRegularExpression extCallRe(R"(\.call\s*\{\s*value:|\bcall\.value\s*\(|\b\.send\s*\(|\b\.transfer\s*\()");
    // State-change: simple assignment or compound assignment (excludes == / != / <= / >=)
    QRegularExpression stateChangeRe(R"((?<![=!<>])=[^=]|\+=[^=]|-=[^=])");

    for (int i = 0; i < lineCount; ++i) {
        if (extCallRe.match(lines[i]).hasMatch()) {
            // Scan ahead up to 20 lines for a state-change
            int lookAhead = qMin(i + 20, lineCount - 1);
            for (int j = i + 1; j <= lookAhead; ++j) {
                if (stateChangeRe.match(lines[j]).hasMatch()) {
                    Finding f;
                    f.line     = i;
                    f.colStart = extCallRe.match(lines[i]).capturedStart();
                    f.colEnd   = extCallRe.match(lines[i]).capturedEnd();
                    f.category    = "REENTRANCY";
                    f.description = "External call followed by state change on line "
                                    + QString::number(j + 1) + " (classic reentrancy pattern)";
                    findings.append(f);
                    break; // one finding per call site
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // 2. State-variable shadowing
    //    Collect identifiers declared at contract depth (lines 0-1 of brace
    //    nesting), then flag any local variable with the same name.
    // -----------------------------------------------------------------------
    QSet<QString> stateVarNames;
    {
        // Very light-weight heuristic: match top-level variable declarations
        // (uint/int/bool/address/bytes/mapping) before the first function body.
        QRegularExpression stateVarRe(
            R"(^\s*(?:uint\d*|int\d*|bool|address(?:\s+payable)?|bytes\d*|string|mapping\b)[^;(]*\b(\w+)\s*;)");
        int braceDepth = 0;
        bool inContract = false;
        for (int i = 0; i < lineCount; ++i) {
            const QString &l = lines[i];
            if (l.contains(QRegularExpression("\\bcontract\\b"))) inContract = true;
            if (!inContract) continue;
            braceDepth += l.count('{') - l.count('}');
            // Only depth 1 = inside contract, but not inside a function
            if (braceDepth == 1) {
                auto m = stateVarRe.match(l);
                if (m.hasMatch())
                    stateVarNames.insert(m.captured(1));
            }
        }
    }

    if (!stateVarNames.isEmpty()) {
        // Now find local declarations that shadow a state var
        QRegularExpression localVarRe(
            R"(^\s*(?:uint\d*|int\d*|bool|address(?:\s+payable)?|bytes\d*|string)\s+(\w+)\s*[=;])");
        int braceDepth = 0;
        bool inContract = false;
        for (int i = 0; i < lineCount; ++i) {
            const QString &l = lines[i];
            if (l.contains(QRegularExpression("\\bcontract\\b"))) inContract = true;
            if (!inContract) continue;
            braceDepth += l.count('{') - l.count('}');
            if (braceDepth >= 2) { // inside a function
                auto m = localVarRe.match(l);
                if (m.hasMatch() && stateVarNames.contains(m.captured(1))) {
                    Finding f;
                    f.line     = i;
                    f.colStart = m.capturedStart(1);
                    f.colEnd   = m.capturedEnd(1);
                    f.category    = "SHADOWING";
                    f.description = "Local variable '" + m.captured(1) + "' shadows a contract state variable";
                    findings.append(f);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // 3. ERC interface completeness
    // -----------------------------------------------------------------------
    struct ErcSpec {
        QRegularExpression contractRe;
        QStringList required;
        QString name;
    };
    QVector<ErcSpec> ercSpecs = {
        {
            QRegularExpression(R"(\bis\s+I?ERC20\b)"),
            { "transfer(", "transferFrom(", "approve(", "allowance(", "balanceOf(", "totalSupply(" },
            "ERC20"
        },
        {
            QRegularExpression(R"(\bis\s+I?ERC721\b)"),
            { "ownerOf(", "safeTransferFrom(", "approve(", "setApprovalForAll(" },
            "ERC721"
        }
    };

    for (const ErcSpec &spec : ercSpecs) {
        // Find the contract declaration line
        int contractLine = -1;
        for (int i = 0; i < lineCount; ++i) {
            if (spec.contractRe.match(lines[i]).hasMatch()) {
                contractLine = i;
                break;
            }
        }
        if (contractLine < 0) continue;

        // Check which required function signatures are present in the whole file
        QStringList missing;
        for (const QString &sig : spec.required) {
            bool found = false;
            for (const QString &l : lines) {
                if (l.contains(sig)) { found = true; break; }
            }
            if (!found) missing.append(sig);
        }

        if (!missing.isEmpty()) {
            Finding f;
            f.line     = contractLine;
            f.colStart = 0;
            f.colEnd   = lines[contractLine].length();
            f.category    = "ERC_MISSING";
            f.description = spec.name + " interface incomplete — missing: " + missing.join(", ");
            findings.append(f);
        }
    }

    // -----------------------------------------------------------------------
    // 4. Division before multiplication (expression-level)
    // -----------------------------------------------------------------------
    QRegularExpression divBeforeMulRe(R"(\w+\s*/\s*\w+\s*\*\s*\w+)");
    for (int i = 0; i < lineCount; ++i) {
        QRegularExpressionMatchIterator it = divBeforeMulRe.globalMatch(lines[i]);
        while (it.hasNext()) {
            auto m = it.next();
            Finding f;
            f.line     = i;
            f.colStart = m.capturedStart();
            f.colEnd   = m.capturedEnd();
            f.category    = "DIV_BEFORE_MUL";
            f.description = "Division before multiplication — possible integer precision loss";
            findings.append(f);
        }
    }

    return findings;
}
