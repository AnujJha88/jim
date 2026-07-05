#ifndef STORYPARSER_H
#define STORYPARSER_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QPair>

// ── Data Structures ───────────────────────────────────────────────────────────

struct StoryChoice {
    QString text;    // display text
    QString target;  // passage name to jump to
};

struct StoryPassage {
    QString name;
    QString content;       // raw text (no header line)
    QStringList tags;      // :: Name [tag1 tag2]
    QVector<StoryChoice> choices;
    int lineNumber = 0;    // 0-indexed line of the :: header
};

struct StoryVariable {
    QString name;
    QString defaultValue;
    QVector<int> setLines;
    QVector<int> readLines;
};

struct StoryLint {
    enum Severity { Warning, Error };
    Severity severity;
    int line;           // 0-indexed
    QString passageName;
    QString message;
};

// ── Parser ────────────────────────────────────────────────────────────────────

class StoryParser {
public:
    // Parse full .story text into passages
    static QVector<StoryPassage> parse(const QString &text);

    // Extract $variables from text
    static QMap<QString, StoryVariable> extractVariables(const QString &text);

    // Lint: unreachable passages, dead ends, undefined vars, etc.
    static QVector<StoryLint> lint(const QVector<StoryPassage> &passages,
                                   const QMap<QString, StoryVariable> &vars);

    // Stats
    struct Stats {
        int passageCount = 0;
        int wordCount = 0;
        int choiceCount = 0;
        int deadEndCount = 0;
        int unreachableCount = 0;
        double avgChoicesPerPassage = 0.0;
        int estimatedReadTimeSecs = 0;
        int longestPathLength = 0;
    };
    static Stats computeStats(const QVector<StoryPassage> &passages);

    // Find the start passage (first passage, or one named "Start"/"start")
    static QString findStartPassage(const QVector<StoryPassage> &passages);
};

#endif // STORYPARSER_H
