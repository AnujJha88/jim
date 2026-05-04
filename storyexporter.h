#ifndef STORYEXPORTER_H
#define STORYEXPORTER_H

#include <QString>
#include "storyparser.h"

class StoryExporter {
public:
    // Export to self-contained HTML (Twine-compatible style)
    static QString toHTML(const QVector<StoryPassage> &passages, const QString &title = "Story");

    // Export to structured JSON (for game engines)
    static QString toJSON(const QVector<StoryPassage> &passages);

    // Export to Ink format (.ink)
    static QString toInk(const QVector<StoryPassage> &passages);

    // Export to flat Markdown (linear reading version)
    static QString toMarkdown(const QVector<StoryPassage> &passages);
};

#endif // STORYEXPORTER_H
