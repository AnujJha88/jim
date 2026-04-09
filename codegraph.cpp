#include "codegraph.h"
#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QPainter>
#include <cmath>
#include <QColor>
#include <QDebug>
#include <QFileInfo>

CodeGraph::CodeGraph(const QString &workspacePath, QWidget *parent)
    : QDialog(parent), m_workspacePath(workspacePath)
{
    setupUI();
    buildGraph();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CodeGraph::applyPhysics);
    m_timer->start(16); // ~60fps
}

CodeGraph::~CodeGraph()
{
}

void CodeGraph::setupUI()
{
    this->setWindowTitle("Neural Code Graph");
    this->resize(1024, 768);
    this->setStyleSheet("background-color: #0b0c10;"); // Dark cyberpunk background

    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor("#0b0c10"));

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
}

void CodeGraph::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    m_scene->setSceneRect(-this->width()/2, -this->height()/2, this->width(), this->height());
}

void CodeGraph::buildGraph()
{
    // Find files
    QDir dir(m_workspacePath);
    QStringList filters;
    filters << "*.cpp" << "*.h" << "*.c" << "*.hpp";
    dir.setNameFilters(filters);
    QFileInfoList fileList = dir.entryInfoList(QDir::Files);

    // Create Nodes
    for(const QFileInfo &fInfo : fileList) {
        QString name = fInfo.fileName();
        GraphNode *node = new GraphNode(name);
        m_scene->addItem(node);
        m_nodes[name] = node;
        
        // Random initial position
        double x = (rand() % 400) - 200;
        double y = (rand() % 400) - 200;
        node->setPos(x, y);
    }

    // Add Edges
    for(const QFileInfo &fInfo : fileList) {
        QStringList includes;
        parseFileIncludes(fInfo.absoluteFilePath(), includes);
        
        QString sourceName = fInfo.fileName();
        GraphNode *sourceNode = m_nodes[sourceName];
        if(!sourceNode) continue;

        for(const QString &inc : includes) {
            if(m_nodes.contains(inc)) {
                GraphNode *destNode = m_nodes[inc];
                GraphEdge *edge = new GraphEdge(sourceNode, destNode);
                m_scene->addItem(edge);
                m_edges.append(edge);
            }
        }
    }
}

void CodeGraph::parseFileIncludes(const QString &filePath, QStringList &includes)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    QRegularExpression re("^\\s*#include\\s*\"([^\"]+)\"");
    while (!in.atEnd()) {
        QString line = in.readLine();
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            includes.append(match.captured(1));
        }
    }
}

void CodeGraph::applyPhysics()
{
    // Node Repulsion
    for (int i = 0; i < m_nodes.keys().size(); ++i) {
        GraphNode *n1 = m_nodes.value(m_nodes.keys()[i]);
        QVector2D force(0,0);
        
        for (int j = 0; j < m_nodes.keys().size(); ++j) {
            if (i == j) continue;
            GraphNode *n2 = m_nodes.value(m_nodes.keys()[j]);
            QVector2D d(n1->pos().x() - n2->pos().x(), n1->pos().y() - n2->pos().y());
            float distStrDist = d.lengthSquared();
            if (distStrDist > 0) {
                float f = kRepulsion / distStrDist;
                d.normalize();
                force += d * f;
            }
        }
        
        // Center Gravity attraction
        QVector2D centerDist(0 - n1->pos().x(), 0 - n1->pos().y());
        force += centerDist * 0.005f;

        n1->addForce(force);
    }

    // Edge Attraction
    for(GraphEdge *edge : m_edges) {
        GraphNode *n1 = edge->sourceNode();
        GraphNode *n2 = edge->destNode();
        
        QVector2D d(n2->pos().x() - n1->pos().x(), n2->pos().y() - n1->pos().y());
        float length = d.length();
        if(length > 0) {
            float f = (length - kSpringRestLen) * kSpringStiffness;
            d.normalize();
            n1->addForce(d * f);
            n2->addForce(d * -f);
        }
    }

    // Advance and Draw
    for (GraphNode *node : m_nodes) {
        node->advancePosition();
    }
    
    for (GraphEdge *edge : m_edges) {
        edge->adjust();
    }
}

// ---------------- GraphNode ----------------

GraphNode::GraphNode(const QString &name) : m_name(name)
{
    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    setZValue(1);
    setRect(-15, -15, 30, 30);
}

void GraphNode::addForce(const QVector2D &force)
{
    m_force += force;
}

void GraphNode::advancePosition()
{
    if (m_force.lengthSquared() < 0.001f && m_velocity.lengthSquared() < 0.001f) {
        m_force = QVector2D(0,0);
        return;
    }

    m_velocity += m_force;
    m_velocity *= 0.85f; // damping
    setPos(pos() + QPointF(m_velocity.x(), m_velocity.y()));
    m_force = QVector2D(0,0);
}

void GraphNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    // Cyberpunk Glow
    QColor color = m_name.endsWith(".h") || m_name.endsWith(".hpp") ? QColor("#66fcf1") : QColor("#ff003c");
    
    painter->setBrush(color);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(rect());
    
    painter->setPen(QColor("#c5c6c7"));
    painter->drawText(rect().adjusted(20, -10, 100, 10), Qt::AlignLeft | Qt::AlignVCenter, m_name);
}

// ---------------- GraphEdge ----------------

GraphEdge::GraphEdge(GraphNode *sourceNode, GraphNode *destNode)
    : source(sourceNode), dest(destNode)
{
    setAcceptedMouseButtons(Qt::NoButton);
    adjust();
}

GraphNode *GraphEdge::sourceNode() const { return source; }
GraphNode *GraphEdge::destNode() const { return dest; }

void GraphEdge::adjust()
{
    if (!source || !dest) return;
    QLineF line(mapFromItem(source, 0, 0), mapFromItem(dest, 0, 0));
    setLine(line);
}

void GraphEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    if(!source || !dest) return;
    QLineF line(source->pos(), dest->pos());
    if(qFuzzyCompare(line.length(), qreal(0.))) return;
    
    painter->setPen(QPen(QColor(100, 100, 100, 150), 2.0));
    painter->drawLine(line);
}
