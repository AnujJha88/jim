#ifndef CODEGRAPH_H
#define CODEGRAPH_H

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QMap>
#include <QStringList>
#include <QVector2D>

class GraphNode;
class GraphEdge;

class CodeGraph : public QDialog
{
    Q_OBJECT
public:
    explicit CodeGraph(const QString &workspacePath, QWidget *parent = nullptr);
    ~CodeGraph();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void applyPhysics();

private:
    void buildGraph();
    void parseFileIncludes(const QString &filePath, QStringList &includes);
    void setupUI();

    QString m_workspacePath;
    QGraphicsScene *m_scene;
    QGraphicsView *m_view;
    QTimer *m_timer;

    QMap<QString, GraphNode*> m_nodes;
    QList<GraphEdge*> m_edges;

    // Physics parameters
    const double kRepulsion = 4000.0;
    const double kSpringRestLen = 100.0;
    const double kSpringStiffness = 0.05;
    const double kDamping = 0.85;
};

class GraphNode : public QGraphicsEllipseItem
{
public:
    GraphNode(const QString &name);

    QString name() const { return m_name; }
    void addForce(const QVector2D &force);
    void advancePosition();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QString m_name;
    QVector2D m_velocity;
    QVector2D m_force;
};

class GraphEdge : public QGraphicsLineItem
{
public:
    GraphEdge(GraphNode *sourceNode, GraphNode *destNode);

    GraphNode *sourceNode() const;
    GraphNode *destNode() const;
    void adjust();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    GraphNode *source, *dest;
};

#endif // CODEGRAPH_H
