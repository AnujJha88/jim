#include "storyexporter.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// ── HTML Export ───────────────────────────────────────────────────────────────

QString StoryExporter::toHTML(const QVector<StoryPassage> &passages, const QString &title)
{
    QString start = StoryParser::findStartPassage(passages);

    // Build passage JS objects
    QStringList passageJS;
    for (const auto &p : passages) {
        // Escape content for JS string
        QString content = p.content;
        content.replace('\\', "\\\\").replace('"', "\\\"").replace('\n', "\\n");
        QString name = p.name;
        name.replace('"', "\\\"");

        QStringList choicesJS;
        for (const auto &c : p.choices) {
            QString ct = c.text; ct.replace('"', "\\\"");
            QString tg = c.target; tg.replace('"', "\\\"");
            choicesJS.append(QString("{text:\"%1\",target:\"%2\"}").arg(ct, tg));
        }
        passageJS.append(QString("{name:\"%1\",content:\"%2\",choices:[%3]}")
                         .arg(name, content, choicesJS.join(',')));
    }

    return QString(R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>%1</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { background: #0d1117; color: #e6edf3; font-family: Georgia, serif;
         display: flex; justify-content: center; align-items: flex-start;
         min-height: 100vh; padding: 40px 20px; }
  #story { max-width: 680px; width: 100%; }
  #passage-title { color: #8be9fd; font-family: monospace; font-size: 13px;
                   letter-spacing: 2px; margin-bottom: 16px; opacity: 0.7; }
  #passage-text { font-size: 16px; line-height: 1.8; color: #e6edf3;
                  margin-bottom: 28px; white-space: pre-wrap; }
  #choices { display: flex; flex-direction: column; gap: 8px; }
  .choice-btn { background: #161b22; color: #cdd6f4; border: 1px solid #30363d;
                border-radius: 6px; padding: 10px 16px; font-family: Georgia, serif;
                font-size: 14px; cursor: pointer; text-align: left; transition: all 0.15s; }
  .choice-btn:hover { background: #21262d; color: #8be9fd; border-color: #8be9fd; }
  #trail { margin-top: 32px; font-family: monospace; font-size: 11px;
           color: #6272a4; border-top: 1px solid #21262d; padding-top: 12px; }
  .ending { color: #f1fa8c; font-style: italic; font-size: 18px; text-align: center;
            padding: 24px 0; }
  .dead-end { color: #ff5555; font-family: monospace; font-size: 12px; }
  @keyframes fadeIn { from { opacity: 0; transform: translateY(8px); }
                      to   { opacity: 1; transform: translateY(0); } }
  #story { animation: fadeIn 0.3s ease; }
</style>
</head>
<body>
<div id="story">
  <div id="passage-title"></div>
  <div id="passage-text"></div>
  <div id="choices"></div>
  <div id="trail"></div>
</div>
<script>
const passages = [%2];
const byName = {};
passages.forEach(p => byName[p.name] = p);
let history = [];

function go(name) {
  const p = byName[name];
  if (!p) { document.getElementById('passage-text').textContent = '[Error: passage "' + name + '" not found]'; return; }
  history.push(name);
  document.getElementById('passage-title').textContent = ':: ' + p.name;
  // Strip [[...]] from display
  const text = p.content.replace(/\[\[.*?\]\]/g, '').trim();
  document.getElementById('passage-text').textContent = text;
  const choicesEl = document.getElementById('choices');
  choicesEl.innerHTML = '';
  if (p.choices.length === 0) {
    const el = document.createElement('div');
    el.className = p.name.toLowerCase().includes('end') ? 'ending' : 'dead-end';
    el.textContent = p.name.toLowerCase().includes('end') ? '✦ The End' : '[ Dead End ]';
    choicesEl.appendChild(el);
  } else {
    p.choices.forEach(c => {
      const btn = document.createElement('button');
      btn.className = 'choice-btn';
      btn.textContent = '▸ ' + c.text;
      btn.onclick = () => go(c.target);
      choicesEl.appendChild(btn);
    });
  }
  document.getElementById('trail').textContent = 'Trail: ' + history.slice(-6).join(' → ');
  window.scrollTo(0, 0);
}

go('%3');
</script>
</body>
</html>)html").arg(title, passageJS.join(','), start);
}

// ── JSON Export ───────────────────────────────────────────────────────────────

QString StoryExporter::toJSON(const QVector<StoryPassage> &passages)
{
    QJsonArray arr;
    for (const auto &p : passages) {
        QJsonObject obj;
        obj["name"]       = p.name;
        obj["content"]    = p.content;
        obj["lineNumber"] = p.lineNumber;
        QJsonArray tags;
        for (const auto &t : p.tags) tags.append(t);
        obj["tags"] = tags;
        QJsonArray choices;
        for (const auto &c : p.choices) {
            QJsonObject co;
            co["text"]   = c.text;
            co["target"] = c.target;
            choices.append(co);
        }
        obj["choices"] = choices;
        arr.append(obj);
    }
    QJsonObject root;
    root["start"]    = StoryParser::findStartPassage(passages);
    root["passages"] = arr;
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

// ── Ink Export ────────────────────────────────────────────────────────────────

QString StoryExporter::toInk(const QVector<StoryPassage> &passages)
{
    QStringList out;
    QString start = StoryParser::findStartPassage(passages);

    // Ink format: knots are === Name ===, choices are * [text] -> target
    for (const auto &p : passages) {
        if (p.name == start)
            out.append("// Start");
        out.append("=== " + QString(p.name).replace(' ', '_') + " ===");

        // Content without [[...]]
        QString content = p.content;
        content.remove(QRegularExpression(R"(\[\[.*?\]\])"));
        out.append(content.trimmed());
        out.append("");

        for (const auto &c : p.choices) {
            out.append("* [" + c.text + "] -> " + QString(c.target).replace(' ', '_'));
        }
        if (p.choices.isEmpty()) {
            bool isEnding = p.tags.contains("ending") || p.tags.contains("end");
            out.append(isEnding ? "-> END" : "-> END // dead end");
        }
        out.append("");
    }
    return out.join('\n');
}

// ── Markdown Export ───────────────────────────────────────────────────────────

QString StoryExporter::toMarkdown(const QVector<StoryPassage> &passages)
{
    QStringList out;
    out.append("# Story\n");

    for (const auto &p : passages) {
        out.append("## " + p.name);
        if (!p.tags.isEmpty())
            out.append("*Tags: " + p.tags.join(", ") + "*");
        out.append("");

        QString content = p.content;
        content.remove(QRegularExpression(R"(\[\[.*?\]\])"));
        out.append(content.trimmed());
        out.append("");

        if (!p.choices.isEmpty()) {
            out.append("**Choices:**");
            for (const auto &c : p.choices)
                out.append("- [" + c.text + "](#" + c.target.toLower().replace(' ', '-') + ")");
            out.append("");
        }
        out.append("---\n");
    }
    return out.join('\n');
}
