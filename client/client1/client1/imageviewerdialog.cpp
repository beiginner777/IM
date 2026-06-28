#include "ImageViewerDialog.h"

ZoomPanGraphicsView::ZoomPanGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setFrameShape(QFrame::NoFrame);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);

    // 背景透明（由外层 dialog 负责遮罩）
    setStyleSheet("background: transparent;");
}

void ZoomPanGraphicsView::setPixmapItem(QGraphicsPixmapItem* item)
{
    m_item = item;
    resetToFit();
}

void ZoomPanGraphicsView::resetToFit()
{
    if (!m_item) return;

    m_fitMode = true;
    resetTransform();

    // 让图片适配视口（保留比例）
    const QRectF itemRect = m_item->boundingRect();
    if (itemRect.isEmpty()) return;

    fitInView(m_item, Qt::KeepAspectRatio);

    // fitInView 会设置 transform，但为了后续 scale 限制更稳定，记录当前 scale 作为基准也可以
}

void ZoomPanGraphicsView::resetTo100()
{
    if (!m_item) return;

    m_fitMode = false;
    resetTransform();

    // 把图片中心放到视口中心
    centerOn(m_item);
}

void ZoomPanGraphicsView::resizeEvent(QResizeEvent* e)
{
    QGraphicsView::resizeEvent(e);
    if (m_fitMode) {
        resetToFit();
    }
}

void ZoomPanGraphicsView::wheelEvent(QWheelEvent* e)
{
    if (!m_item) return;

    // 常见体验：滚轮上滑放大，下滑缩小
    const QPoint numDegrees = e->angleDelta() / 8;
    if (numDegrees.isNull()) return;

    // 每次滚动一个“档位”的缩放倍率
    const qreal factor = (numDegrees.y() > 0) ? 1.15 : (1.0 / 1.15);

    // 一旦用户滚轮缩放，就不再处于 fit 模式
    m_fitMode = false;

    zoomAt(e->position().toPoint(), factor);
    e->accept();
}

void ZoomPanGraphicsView::zoomAt(const QPoint& viewPos, qreal factor)
{
    // 限制整体缩放范围：通过当前 transform 的 m11 判断
    const qreal current = transform().m11();
    qreal next = current * factor;
    if (next < m_minScale) factor = m_minScale / current;
    if (next > m_maxScale) factor = m_maxScale / current;

    const QPointF scenePosBefore = mapToScene(viewPos);

    scale(factor, factor);

    const QPointF scenePosAfter = mapToScene(viewPos);
    const QPointF delta = scenePosAfter - scenePosBefore;

    // 把视图平移回来，让鼠标所在点保持“缩放前后同一位置”
    translate(delta.x(), delta.y());
}

void ZoomPanGraphicsView::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMousePos = e->pos();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    QGraphicsView::mousePressEvent(e);
}

void ZoomPanGraphicsView::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging) {
        const QPoint delta = e->pos() - m_lastMousePos;
        m_lastMousePos = e->pos();

        // 注意：这里用 translate 的符号与直觉一致（拖动内容跟着鼠标走）
        translate(delta.x(), delta.y());
        e->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(e);
}

void ZoomPanGraphicsView::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        unsetCursor();
        e->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(e);
}

void ZoomPanGraphicsView::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        // 双击切换：fit <-> 100%
        if (m_fitMode) resetTo100();
        else resetToFit();

        e->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(e);
}

// -------------------- ImageViewerDialog --------------------

ImageViewerDialog::ImageViewerDialog(const QPixmap& pm, QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setModal(true);

    // 全屏铺满所在屏幕（比 showFullScreen 更可控）f
    QScreen* screen = nullptr;
    if (parent && parent->windowHandle() && parent->windowHandle()->screen())
        screen = parent->windowHandle()->screen();
    if (!screen) screen = QGuiApplication::primaryScreen();

    setGeometry(screen->availableGeometry());

    // 遮罩 + 内容布局
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 让整个 dialog 背景半透明黑（微信感觉）
    setStyleSheet("background: rgba(0,0,0,180);");

    m_scene = new QGraphicsScene(this);
    m_item = new QGraphicsPixmapItem(pm);
    m_item->setTransformationMode(Qt::SmoothTransformation);
    m_scene->addItem(m_item);

    m_view = new ZoomPanGraphicsView(this);
    m_view->setScene(m_scene);
    m_view->setPixmapItem(m_item);

    // 把场景范围设得大一点，避免拖拽时卡边
    m_scene->setSceneRect(m_item->boundingRect().adjusted(-5000, -5000, 5000, 5000));

    rootLayout->addWidget(m_view);
}

void ImageViewerDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    QDialog::keyPressEvent(e);
}

void ImageViewerDialog::mousePressEvent(QMouseEvent* e)
{
    // 点击空白处关闭（view 会吃掉大部分事件；如果你点到 view 空白也想关，
    // 可以在 view 的 viewport 上装 eventFilter，这里先给最简单版本）
    if (e->button() == Qt::LeftButton) {
        // 若希望“点任意处关闭”，直接 close() 即可；但会影响拖拽体验
        // 这里保持更像微信：主要通过 ESC / 右上角按钮（你也可以加按钮）
    }
    QDialog::mousePressEvent(e);
}
