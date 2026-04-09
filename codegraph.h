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
    explicit CodeGraph(const QString &workspacePath,
                       const QString &currentFile = QString(),
                       QWidget *parent = nullptr);
    ~CodeGraph();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void applyPhysics();

private:
    void setupUI();
    void buildGraph();
    void parseImports(const QString &filePath, QStringList &deps);
    QString resolveImport(const QString &import, const QString &fromDir, const QString &fileExt);

    QString          m_workspacePath;
    QString          m_currentFile;
    QGraphicsScene  *m_scene;
    QGraphicsView   *m_view;
    QTimer          *m_timer;

    QMap<QString, GraphNode*> m_nodes; // keyed by absolute path
    QList<GraphEdge*>         m_edges;

    const double kRepulsion      = 7000.0;
    const double kSpringRestLen  = 130.0;
    const double kSpringStiffness = 0.04;
};

class GraphNode : public QGraphicsEllipseItem
{
public:
    GraphNode(const QString &name, const QString &relPath, const QString &absPath);

    QString name()    const { return m_name; }
    QString absPath() const { return m_absPath; }

    void addForce(const QVector2D &force);
    void advancePosition();
    void setHighlighted(bool h) { m_highlighted = h; }
    void addConnection()        { m_connections++; }
    void updateSize();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QString   m_name;
    QString   m_relPath;
    QString   m_absPath;
    bool      m_highlighted = false;
    int       m_connections = 0;
    QVector2D m_velocity;
    QVector2D m_force;
};

class GraphEdge : public QGraphicsLineItem
{
public:
    GraphEdge(GraphNode *src, GraphNode *dst);

    GraphNode *sourceNode() const;
    GraphNode *destNode()   const;
    void adjust();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    GraphNode *source, *dest;
};

#endif // CODEGRAPH_H
