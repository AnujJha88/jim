#ifndef STORYPLAYTEST_H
#define STORYPLAYTEST_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMap>
#include "storyparser.h"

// ── StoryPlaytest ─────────────────────────────────────────────────────────────
// Terminal-style interactive playtest panel.
// Renders current passage text, lists choices as clickable buttons,
// tracks visited passages and variable state.

class StoryPlaytest : public QWidget {
    Q_OBJECT
public:
    explicit StoryPlaytest(QWidget *parent = nullptr);

    void loadStory(const QString &storyText);
    void restart();

signals:
    void passageChanged(const QString &passageName); // sync with graph

private slots:
    void onChoiceClicked(int index);
    void onRestartClicked();

private:
    void goToPassage(const QString &name);
    void renderPassage();
    QString processText(const QString &raw); // substitute $vars

    QVector<StoryPassage>          m_passages;
    QMap<QString, const StoryPassage*> m_byName;
    QMap<QString, QString>         m_vars;     // runtime variable state
    QString                        m_current;
    QStringList                    m_history;  // breadcrumb trail

    // UI
    QLabel         *m_passageTitle;
    QPlainTextEdit *m_textArea;
    QWidget        *m_choicesWidget;
    QVBoxLayout    *m_choicesLayout;
    QLabel         *m_historyLabel;
    QPushButton    *m_restartBtn;
    QLabel         *m_varsLabel;

    void setupUI();
    void updateVarsDisplay();
    void updateHistoryDisplay();
};

#endif // STORYPLAYTEST_H
