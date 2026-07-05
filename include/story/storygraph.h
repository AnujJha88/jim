#ifndef STORYGRAPH_H
#define STORYGRAPH_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QTimer>
#include <QMap>
#include <QVector2D>
#include "storyparser.h"

class StoryGraphNode;
class StoryGraphEdge;

// ── StoryGraph ────────────────────────────────────────────────────────────────
// Dockable force-directed graph of story passages.
// Reuses the same physics engine as CodeGraph.

class StoryGraph : public QWidget {
    Q_OBJECT
public:
    explicit StoryGraph(QWidget *parent = nullptr);

    void refresh(const QString &storyText);
    void setCurrentPassage(const QString &name);

signals:
    void passageClicked(const QString &passageName);

private slots:
    void applyPhysics();

private:
    void buildGraph(const QVector<StoryPassage> &passages);
    void clearGraph();

    QGraphicsScene *m_scene;
    QGraphicsView  *m_view;
    QTimer         *m_timer;

    QMap<QString, StoryGraphNode*> m_nodes;
    QList<StoryGraphEdge*>         m_edges;

    const double kRepulsion       = 6000.0;
    const double kSpringRestLen   = 120.0;
    const double kSpringStiffness = 0.035;
};

// ── StoryGraphNode ────────────────────────────────────────────────────────────

class StoryGraphNode : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
public:
    enum NodeType { Normal, Start, DeadEnd, Ending, Unreachable };

    StoryGraphNode(const QString &name, NodeType type, int choiceCount);

    QString name() const { return m_name; }
    void addForce(const QVector2D &f) { m_force += f; }
    void advancePosition();
    void setActive(bool a);

signals:
    void clicked(const QString &name);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QString   m_name;
    NodeType  m_type;
    int       m_choiceCount;
    bool      m_active = false;
    QVector2D m_velocity;
    QVector2D m_force;
};

// ── StoryGraphEdge ────────────────────────────────────────────────────────────

class StoryGraphEdge : public QGraphicsLineItem {
public:
    StoryGraphEdge(StoryGraphNode *src, StoryGraphNode *dst, const QString &label);
    void adjust();
    StoryGraphNode *src() const { return m_src; }
    StoryGraphNode *dst() const { return m_dst; }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

private:
    StoryGraphNode *m_src, *m_dst;
    QString m_label;
};

#endif // STORYGRAPH_H
