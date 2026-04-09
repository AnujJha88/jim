#include "codegraph.h"
#include <QVBoxLayout>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QPainter>
#include <QToolTip>
#include <QGraphicsSceneHoverEvent>
#include <cmath>
#include <chrono>

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────

static QColor colorForFile(const QString &name)
{
    QString ext = QFileInfo(name).suffix().toLower();
    if (ext == "h" || ext == "hpp" || ext == "hxx")      return QColor("#66fcf1"); // cyan  — C/C++ header
    if (ext == "cpp" || ext == "c" || ext == "cxx")      return QColor("#ff4f7b"); // pink  — C/C++ source
    if (ext == "py")                                      return QColor("#f1fa8c"); // yellow— Python
    if (ext == "js" || ext == "mjs" || ext == "cjs")     return QColor("#ffb86c"); // orange— JS
    if (ext == "ts")                                      return QColor("#8be9fd"); // blue  — TS
    if (ext == "jsx" || ext == "tsx")                     return QColor("#50fa7b"); // green — JSX/TSX
    if (ext == "go")                                      return QColor("#00acd7"); // cyan  — Go
    if (ext == "rs")                                      return QColor("#ff8c00"); // orange— Rust
    if (ext == "java" || ext == "kt")                     return QColor("#e85d04"); // red   — Java/Kotlin
    if (ext == "rb")                                      return QColor("#ff5555"); // red   — Ruby
    if (ext == "lua")                                     return QColor("#bd93f9"); // purple— Lua
    return QColor("#aaaaaa");
}

// Dirs to skip when scanning
static const QStringList kSkipDirs = {
    "node_modules", ".git", "build", "dist", "release", "debug",
    "__pycache__", ".venv", "venv", "env", "target", ".next",
    ".cache", "vendor", "third_party", "Pods", ".gradle"
};

static bool shouldSkip(const QString &absPath)
{
    for (const QString &d : kSkipDirs)
        if (absPath.contains('/' + d + '/'))
            return true;
    return false;
}

// ─────────────────────────────────────────────
//  CodeGraph
// ─────────────────────────────────────────────

CodeGraph::CodeGraph(const QString &workspacePath, const QString &currentFile, QWidget *parent)
    : QDialog(parent), m_workspacePath(workspacePath), m_currentFile(currentFile)
{
    setupUI();
    buildGraph();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CodeGraph::applyPhysics);
    m_timer->start(16);
}

CodeGraph::~CodeGraph() {}

void CodeGraph::setupUI()
{
    QString title = "Neural Code Graph";
    if (!m_currentFile.isEmpty())
        title += " — " + QFileInfo(m_currentFile).fileName();
    else if (!m_workspacePath.isEmpty())
        title += " — " + QDir(m_workspacePath).dirName();
    setWindowTitle(title);
    resize(1200, 800);
    setStyleSheet("background-color: #0b0c10;");

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor("#0b0c10"));

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
}

void CodeGraph::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    m_scene->setSceneRect(-width() / 2, -height() / 2, width(), height());
}

void CodeGraph::buildGraph()
{
    // Seed rand so layout isn't identical every open
    auto seed = static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    srand(seed);

    // ── Collect source files (recursive, capped at 400) ──────────────────────
    const QStringList exts = {
        "*.cpp","*.h","*.c","*.hpp","*.cxx","*.hxx",
        "*.py",
        "*.js","*.ts","*.jsx","*.tsx","*.mjs","*.cjs",
        "*.go",
        "*.rs",
        "*.java","*.kt",
        "*.rb",
        "*.lua",
    };

    QDirIterator it(m_workspacePath, exts, QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext() && count < 400) {
        it.next();
        QString abs = it.fileInfo().absoluteFilePath();
        if (shouldSkip(abs)) continue;

        QString rel  = QDir(m_workspacePath).relativeFilePath(abs);
        QString name = it.fileInfo().fileName();

        auto *node = new GraphNode(name, rel, abs);
        m_scene->addItem(node);
        m_nodes[abs] = node;

        if (abs == m_currentFile) {
            node->setHighlighted(true);
            node->setPos(0, 0); // center the current file
        } else {
            double spread = qMax(300.0, qMin(800.0, count * 4.0));
            node->setPos((rand() % (int)(spread * 2)) - spread,
                         (rand() % (int)(spread * 2)) - spread);
        }
        count++;
    }

    // ── Parse imports and wire edges ─────────────────────────────────────────
    for (auto nit = m_nodes.begin(); nit != m_nodes.end(); ++nit) {
        QStringList deps;
        parseImports(nit.key(), deps);

        for (const QString &dep : deps) {
            if (dep == nit.key()) continue;
            if (!m_nodes.contains(dep)) continue;

            auto *edge = new GraphEdge(nit.value(), m_nodes[dep]);
            m_scene->addItem(edge);
            m_edges.append(edge);
            nit.value()->addConnection();
            m_nodes[dep]->addConnection();
        }
    }

    // Scale node sizes now that connection counts are known
    for (GraphNode *node : m_nodes)
        node->updateSize();

    // If a current file was highlighted, centre the view on it
    if (!m_currentFile.isEmpty() && m_nodes.contains(m_currentFile))
        m_view->centerOn(m_nodes[m_currentFile]);
}

// ── Import parsing ────────────────────────────────────────────────────────────

void CodeGraph::parseImports(const QString &filePath, QStringList &deps)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    QString content = in.readAll();

    QFileInfo fi(filePath);
    QString ext    = fi.suffix().toLower();
    QString dir    = fi.absoluteDir().absolutePath();
    QStringList candidates;

    if (ext == "cpp" || ext == "c" || ext == "h" || ext == "hpp" || ext == "cxx" || ext == "hxx") {
        // #include "foo.h"  (local only — skip <system>)
        QRegularExpression re(R"re(#include\s*"([^"]+)")re");
        auto m = re.globalMatch(content);
        while (m.hasNext()) candidates << m.next().captured(1);

    } else if (ext == "py") {
        // from .pkg.mod import X  /  from pkg.mod import X  /  import pkg.mod
        QRegularExpression re1(R"(from\s+(\.[\w.]*)\s+import)");   // relative
        QRegularExpression re2(R"(from\s+([\w][\w.]*)\s+import)"); // absolute
        QRegularExpression re3(R"(^\s*import\s+([\w.]+))", QRegularExpression::MultilineOption);
        for (auto &re : {re1, re2, re3}) {
            auto m = re.globalMatch(content);
            while (m.hasNext()) candidates << m.next().captured(1);
        }

    } else if (ext == "js" || ext == "mjs" || ext == "cjs" ||
               ext == "ts" || ext == "jsx" || ext == "tsx") {
        // import ... from '...' / require('...')
        QRegularExpression re(R"((?:from|require)\s*\(?\s*['"]([^'"]+)['"]\s*\)?)");
        auto m = re.globalMatch(content);
        while (m.hasNext()) candidates << m.next().captured(1);

    } else if (ext == "go") {
        // import "pkg/sub" or import ( "pkg/sub" )
        QRegularExpression re(R"re(import\s+(?:\(([^)]+)\)|"([^"]+)"))re");
        auto m = re.globalMatch(content);
        while (m.hasNext()) {
            auto match    = m.next();
            QString block = match.captured(1);
            if (block.isEmpty()) {
                candidates << match.captured(2);
            } else {
                QRegularExpression inner(R"re("([^"]+)")re");
                auto im = inner.globalMatch(block);
                while (im.hasNext()) candidates << im.next().captured(1);
            }
        }

    } else if (ext == "rs") {
        // mod foo;  (resolves to foo.rs or foo/mod.rs)
        QRegularExpression re(R"(^\s*mod\s+(\w+)\s*;)", QRegularExpression::MultilineOption);
        auto m = re.globalMatch(content);
        while (m.hasNext()) candidates << m.next().captured(1);

    } else if (ext == "java" || ext == "kt") {
        // import com.example.Foo;
        QRegularExpression re(R"(import\s+([\w.]+)\s*;)");
        auto m = re.globalMatch(content);
        while (m.hasNext()) candidates << m.next().captured(1);

    } else if (ext == "lua") {
        // require("mod") or require('mod')
        QRegularExpression re(R"(require\s*\(?\s*['"]([^'"]+)['"]\s*\)?)");
        auto m = re.globalMatch(content);
        while (m.hasNext()) candidates << m.next().captured(1);
    }

    for (const QString &cand : candidates) {
        QString resolved = resolveImport(cand, dir, ext);
        if (!resolved.isEmpty() && !deps.contains(resolved))
            deps << resolved;
    }
}

QString CodeGraph::resolveImport(const QString &import, const QString &fromDir, const QString &ext)
{
    // ── C / C++ ──────────────────────────────────────────────────────────────
    if (ext == "cpp" || ext == "c" || ext == "h" || ext == "hpp" || ext == "cxx" || ext == "hxx") {
        for (const QString &base : {fromDir, m_workspacePath}) {
            QString p = QDir(base).absoluteFilePath(import);
            if (QFileInfo::exists(p)) return p;
        }
    }

    // ── Python ───────────────────────────────────────────────────────────────
    if (ext == "py") {
        QString cleaned = import;
        int dots = 0;
        while (cleaned.startsWith('.')) { cleaned = cleaned.mid(1); dots++; }

        QString base = fromDir;
        for (int i = 1; i < dots; i++) base = QFileInfo(base).absolutePath();

        QString rel = cleaned.replace('.', '/');
        for (const QString &b : {base, m_workspacePath}) {
            QString p = QDir(b).absoluteFilePath(rel + ".py");
            if (QFileInfo::exists(p)) return p;
            p = QDir(b).absoluteFilePath(rel + "/__init__.py");
            if (QFileInfo::exists(p)) return p;
        }
    }

    // ── JS / TS ───────────────────────────────────────────────────────────────
    if (ext == "js" || ext == "mjs" || ext == "cjs" || ext == "ts" || ext == "jsx" || ext == "tsx") {
        if (!import.startsWith('.')) return {}; // skip package imports
        const QStringList tryExts = {".ts",".tsx",".js",".jsx",".mjs"};
        for (const QString &e : tryExts) {
            QString p = QDir(fromDir).absoluteFilePath(import + e);
            if (QFileInfo::exists(p)) return p;
            p = QDir(fromDir).absoluteFilePath(import + "/index" + e);
            if (QFileInfo::exists(p)) return p;
        }
        // exact path (already has extension)
        QString p = QDir(fromDir).absoluteFilePath(import);
        if (QFileInfo::exists(p)) return p;
    }

    // ── Rust mod ─────────────────────────────────────────────────────────────
    if (ext == "rs") {
        QString p = QDir(fromDir).absoluteFilePath(import + ".rs");
        if (QFileInfo::exists(p)) return p;
        p = QDir(fromDir).absoluteFilePath(import + "/mod.rs");
        if (QFileInfo::exists(p)) return p;
    }

    // ── Lua ──────────────────────────────────────────────────────────────────
    if (ext == "lua") {
        QString rel = QString(import).replace('.', '/');
        QString p   = QDir(fromDir).absoluteFilePath(rel + ".lua");
        if (QFileInfo::exists(p)) return p;
        p = QDir(m_workspacePath).absoluteFilePath(rel + ".lua");
        if (QFileInfo::exists(p)) return p;
    }

    return {};
}

// ── Physics ───────────────────────────────────────────────────────────────────

void CodeGraph::applyPhysics()
{
    QList<QString> keys = m_nodes.keys();
    int n = keys.size();

    for (int i = 0; i < n; ++i) {
        GraphNode *n1 = m_nodes.value(keys[i]);
        QVector2D force(0, 0);

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            GraphNode *n2 = m_nodes.value(keys[j]);
            QVector2D d(n1->pos().x() - n2->pos().x(), n1->pos().y() - n2->pos().y());
            float dist2 = d.lengthSquared();
            if (dist2 > 0) {
                d.normalize();
                force += d * float(kRepulsion / dist2);
            }
        }

        // Gentle pull toward centre
        force += QVector2D(-n1->pos().x(), -n1->pos().y()) * 0.004f;

        n1->addForce(force);
    }

    for (GraphEdge *edge : m_edges) {
        GraphNode *a = edge->sourceNode();
        GraphNode *b = edge->destNode();
        QVector2D d(b->pos().x() - a->pos().x(), b->pos().y() - a->pos().y());
        float len = d.length();
        if (len > 0) {
            float f = (len - float(kSpringRestLen)) * float(kSpringStiffness);
            d.normalize();
            a->addForce(d *  f);
            b->addForce(d * -f);
        }
    }

    for (GraphNode *node : m_nodes) node->advancePosition();
    for (GraphEdge *edge  : m_edges) edge->adjust();
}

// ─────────────────────────────────────────────
//  GraphNode
// ─────────────────────────────────────────────

GraphNode::GraphNode(const QString &name, const QString &relPath, const QString &absPath)
    : m_name(name), m_relPath(relPath), m_absPath(absPath)
{
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setCacheMode(DeviceCoordinateCache);
    setZValue(1);
    setRect(-15, -15, 30, 30);
    setToolTip(relPath);
}

void GraphNode::updateSize()
{
    float r = qBound(12.0f, 12.0f + m_connections * 2.5f, 32.0f);
    setRect(-r, -r, r * 2, r * 2);
}

void GraphNode::addForce(const QVector2D &force) { m_force += force; }

void GraphNode::advancePosition()
{
    if (m_force.lengthSquared() < 0.001f && m_velocity.lengthSquared() < 0.001f) {
        m_force = QVector2D(0, 0);
        return;
    }
    m_velocity += m_force;
    m_velocity *= 0.85f;
    setPos(pos() + QPointF(m_velocity.x(), m_velocity.y()));
    m_force = QVector2D(0, 0);
}

void GraphNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QColor col = colorForFile(m_name);
    QRectF r   = rect();

    if (m_highlighted) {
        // Outer glow ring for the active file
        painter->setPen(QPen(QColor(255, 220, 50, 180), 3));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(r.adjusted(-4, -4, 4, 4));
        col = col.lighter(140);
    }

    // Soft glow behind the dot
    QRadialGradient glow(r.center(), r.width() * 0.9);
    glow.setColorAt(0,   QColor(col.red(), col.green(), col.blue(), 120));
    glow.setColorAt(1,   QColor(col.red(), col.green(), col.blue(), 0));
    painter->setPen(Qt::NoPen);
    painter->setBrush(glow);
    painter->drawEllipse(r.adjusted(-6, -6, 6, 6));

    // Main dot
    painter->setBrush(col);
    painter->drawEllipse(r);

    // Label
    painter->setPen(QColor("#c5c6c7"));
    QFont f = painter->font();
    f.setPointSize(m_highlighted ? 9 : 8);
    if (m_highlighted) f.setBold(true);
    painter->setFont(f);
    painter->drawText(QRectF(r.right() + 4, r.top() - 4, 180, r.height() + 8),
                      Qt::AlignLeft | Qt::AlignVCenter, m_name);
}

// ─────────────────────────────────────────────
//  GraphEdge
// ─────────────────────────────────────────────

GraphEdge::GraphEdge(GraphNode *src, GraphNode *dst) : source(src), dest(dst)
{
    setAcceptedMouseButtons(Qt::NoButton);
    setZValue(0);
    adjust();
}

GraphNode *GraphEdge::sourceNode() const { return source; }
GraphNode *GraphEdge::destNode()   const { return dest; }

void GraphEdge::adjust()
{
    if (!source || !dest) return;
    setLine(QLineF(mapFromItem(source, 0, 0), mapFromItem(dest, 0, 0)));
}

void GraphEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!source || !dest) return;
    QLineF line(source->pos(), dest->pos());
    if (qFuzzyCompare(line.length(), 0.0)) return;

    painter->setPen(QPen(QColor(80, 80, 120, 140), 1.5));
    painter->drawLine(line);
}
