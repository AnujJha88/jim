#include "cityscape_widget.h"
#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <QPainterPath>

CityscapeWidget::CityscapeWidget(QWidget *parent) 
    : QWidget(parent), scale(1.0f), panX(0), panY(0), hoveredIndex(-1) {
    setMouseTracking(true);
    setStyleSheet("background: #0d0d1a;");
}

void CityscapeWidget::loadDirectory(const QString &path) {
    buildings.clear();
    currentPath = path;
    int gx = 0, gy = 0;
    scanDir(path, gx, gy, 20); // 20 blocks wide city grid
    update();
}

void CityscapeWidget::scanDir(const QString &dirPath, int &gx, int &gy, int maxGx) {
    QDir dir(dirPath);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList list = dir.entryInfoList();
    
    for (const QFileInfo &info : list) {
        if (info.isDir()) {
            if (info.fileName() == ".git" || info.fileName() == "node_modules" || info.fileName() == "build") continue;
            scanDir(info.absoluteFilePath(), gx, gy, maxGx);
        } else {
            QString ext = info.suffix().toLower();
            if (ext == "cpp" || ext == "h" || ext == "js" || ext == "py" || ext == "sol" || ext == "css" || ext == "html" || ext == "rs" || ext == "go" || ext == "lean") {
                QFile f(info.absoluteFilePath());
                int loc = 0;
                if (f.open(QIODevice::ReadOnly)) {
                    QTextStream in(&f);
                    while (!in.atEnd()) {
                        in.readLine();
                        loc++;
                    }
                    f.close();
                }
                
                Building b;
                b.name = info.fileName();
                b.path = info.absoluteFilePath();
                b.linesOfCode = loc;
                b.height = qMin(300.0f, qMax(10.0f, loc * 0.2f));
                b.gridX = gx;
                b.gridY = gy;
                
                if (ext == "cpp") b.color = QColor(0, 120, 215);
                else if (ext == "h") b.color = QColor(160, 0, 200);
                else if (ext == "js") b.color = QColor(240, 219, 79);
                else if (ext == "py") b.color = QColor(53, 114, 165);
                else if (ext == "sol") b.color = QColor(80, 80, 80);
                else if (ext == "rs") b.color = QColor(222, 165, 132);
                else b.color = QColor(0, 255, 120); // Neon green default
                
                buildings.append(b);
                
                gx++;
                if (gx >= maxGx) {
                    gx = 0;
                    gy++;
                }
            }
        }
    }
}

void CityscapeWidget::drawIsometricCube(QPainter &p, float x, float y, float w, float h, float d, const QColor &color) {
    // Top face
    QPainterPath top;
    top.moveTo(x, y - h);
    top.lineTo(x + w, y - h - w * 0.5f);
    top.lineTo(x + w + d, y - h - w * 0.5f + d * 0.5f);
    top.lineTo(x + d, y - h + d * 0.5f);
    top.closeSubpath();
    p.setBrush(color.lighter(130));
    p.drawPath(top);
    
    // Left face
    QPainterPath left;
    left.moveTo(x, y);
    left.lineTo(x + d, y + d * 0.5f);
    left.lineTo(x + d, y + d * 0.5f - h);
    left.lineTo(x, y - h);
    left.closeSubpath();
    p.setBrush(color);
    p.drawPath(left);
    
    // Right face
    QPainterPath right;
    right.moveTo(x + d, y + d * 0.5f);
    right.lineTo(x + w + d, y - w * 0.5f + d * 0.5f);
    right.lineTo(x + w + d, y - w * 0.5f + d * 0.5f - h);
    right.lineTo(x + d, y + d * 0.5f - h);
    right.closeSubpath();
    p.setBrush(color.darker(150));
    p.drawPath(right);
}

void CityscapeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(13, 13, 26)); // Cyberpunk night sky
    
    // Draw neon grid
    p.setPen(QColor(0, 255, 200, 40));
    for(int i=-2000; i<2000; i+=100) {
        float isoX1 = (i - -2000) * 1.0f * scale + panX;
        float isoY1 = (i + -2000) * 0.5f * scale + panY;
        float isoX2 = (i - 2000) * 1.0f * scale + panX;
        float isoY2 = (i + 2000) * 0.5f * scale + panY;
        p.drawLine(isoX1, isoY1, isoX2, isoY2);
        
        float isoX3 = (-2000 - i) * 1.0f * scale + panX;
        float isoY3 = (-2000 + i) * 0.5f * scale + panY;
        float isoX4 = (2000 - i) * 1.0f * scale + panX;
        float isoY4 = (2000 + i) * 0.5f * scale + panY;
        p.drawLine(isoX3, isoY3, isoX4, isoY4);
    }
    
    p.setPen(QColor(0, 0, 0, 100)); // Wireframe outline
    
    for (int i = 0; i < buildings.size(); i++) {
        const Building &b = buildings[i];
        
        // Convert grid to isometric
        float isoX = (b.gridX * 60 - b.gridY * 60) * scale + panX + width() / 2;
        float isoY = (b.gridX * 60 + b.gridY * 60) * 0.5f * scale + panY + height() / 3;
        
        QColor buildingCol = b.color;
        if (i == hoveredIndex) {
            buildingCol = buildingCol.lighter(150);
            p.setPen(QColor(255, 255, 255));
        } else {
            p.setPen(QColor(0, 0, 0, 150));
        }
        
        drawIsometricCube(p, isoX, isoY, 40 * scale, b.height * scale, 40 * scale, buildingCol);
        
        if (i == hoveredIndex) {
            // Draw floating label
            p.setPen(QColor(0, 255, 255));
            p.setFont(QFont("Consolas", 10, QFont::Bold));
            p.drawText(isoX, isoY - b.height * scale - 20, b.name);
        }
    }
}

void CityscapeWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        panX += event->pos().x() - lastMousePos.x();
        panY += event->pos().y() - lastMousePos.y();
        lastMousePos = event->pos();
        update();
        return;
    }
    
    lastMousePos = event->pos();
    hoveredIndex = -1;
    
    // Simple raycast / hit test
    for (int i = buildings.size() - 1; i >= 0; i--) { // Check front-most first
        const Building &b = buildings[i];
        float isoX = (b.gridX * 60 - b.gridY * 60) * scale + panX + width() / 2;
        float isoY = (b.gridX * 60 + b.gridY * 60) * 0.5f * scale + panY + height() / 3;
        
        QRectF bounds(isoX, isoY - b.height * scale, 80 * scale, b.height * scale + 40 * scale);
        if (bounds.contains(event->pos())) {
            hoveredIndex = i;
            QToolTip::showText(event->globalPosition().toPoint(), QString("<div style='background:#0d0d1a;border:1px solid #00ff88;padding:5px;font-family:Consolas;color:#fff;'><b>%1</b><br><span style='color:#00ff88;'>Lines: %2</span></div>").arg(b.name).arg(b.linesOfCode), this);
            break;
        }
    }
    update();
}

void CityscapeWidget::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0) scale *= 1.1f;
    else scale *= 0.9f;
    update();
}

void CityscapeWidget::mousePressEvent(QMouseEvent *event) {
    lastMousePos = event->pos();
}
