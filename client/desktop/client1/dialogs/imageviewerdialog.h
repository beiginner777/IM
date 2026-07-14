#ifndef ZOOMPANGRAPHICSVIEW_H
#define ZOOMPANGRAPHICSVIEW_H

#include "core/global.h"

class ZoomPanGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ZoomPanGraphicsView(QWidget* parent = nullptr);

    void setPixmapItem(QGraphicsPixmapItem* item);
    void resetToFit();      // 适配窗口
    void resetTo100();      // 100%
    bool isFitMode() const { return m_fitMode; }

protected:
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    void zoomAt(const QPoint& viewPos, qreal factor);

private:
    QGraphicsPixmapItem* m_item = nullptr;
    // QPointer<QGraphicsPixmapItem> m_item;
    bool m_dragging = false;
    QPoint m_lastMousePos;
    bool m_fitMode = true;

    qreal m_minScale = 0.05;
    qreal m_maxScale = 8.0;
};

class ImageViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImageViewerDialog(const QPixmap& pm, QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    ZoomPanGraphicsView* m_view = nullptr;
    QGraphicsScene* m_scene = nullptr;
    QGraphicsPixmapItem* m_item = nullptr;
};

#endif
