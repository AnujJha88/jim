#ifndef VULNSCANNER_H
#define VULNSCANNER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QRegularExpression>
#include "syntaxhighlighter.h" // for Language enum

class VulnScanner : public QObject {
    Q_OBJECT
public:
    struct Finding {
        int line;           // 0-indexed block number
        int colStart;
        int colEnd;
        int stateLine = -1; // for REENTRANCY: the line where the state change occurs
        // category values:
        //   "SECRET"          — hardcoded credentials / API tokens
        //   "DANGEROUS_FUNC"  — unsafe C/C++ stdlib calls
        //   "SOLIDITY"        — general Solidity anti-patterns
        //   "REENTRANCY"      — cross-function reentrancy via external call + state change
        //   "SHADOWING"       — local variable shadows a contract-level state variable
        //   "REDOS"           — catastrophic-backtracking regex pattern (JS / Python)
        //   "ERC_MISSING"     — ERC-20 / ERC-721 interface function(s) absent from contract
        //   "DIV_BEFORE_MUL"  — integer division before multiplication (precision loss)
        QString category;
        QString description;
    };

    explicit VulnScanner(QObject *parent = nullptr);
    QVector<Finding> scan(const QString &text, Language lang, const QString &fileName);

    // Higher-level Solidity scan: wraps scan() and adds reentrancy, shadowing,
    // ERC interface completeness, and division-before-multiplication checks.
    QVector<Finding> scanSolidity(const QString &text, const QString &fileName);

private:
    struct Rule {
        QRegularExpression regex;
        QString category;
        QString description;
    };
    QVector<Rule> generalRules;
    QVector<Rule> cppRules;
    QVector<Rule> solidityRules;
    QVector<Rule> redosRules;    // catastrophic-backtracking patterns (JS / Python)

    void initRules();
};

#endif // VULNSCANNER_H
