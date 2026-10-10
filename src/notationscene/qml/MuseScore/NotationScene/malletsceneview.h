/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <QQuickPaintedItem>
#include <QVariantMap>
#include <QColor>
#include <QTransform>
#include <qqmlintegration.h>
namespace mu::notation {
class MalletSceneView : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantMap scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY sceneChanged)
    Q_PROPERTY(int hoveredPitch READ hoveredPitch NOTIFY hoveredPitchChanged)
    Q_PROPERTY(bool focusPlacement READ focusPlacement WRITE setFocusPlacement NOTIFY zoomChanged)
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
public:
    explicit MalletSceneView(QQuickItem* parent = nullptr);
    QVariantMap scene() const { return m_scene; }
    void setScene(const QVariantMap& value);
    QColor backgroundColor() const { return m_background; }
    void setBackgroundColor(const QColor& value) { m_background = value; update(); emit sceneChanged(); }
    int hoveredPitch() const { return m_hoveredPitch; }
    bool focusPlacement() const {return m_focusPlacement;}
    void setFocusPlacement(bool value) {if(m_focusPlacement==value)return;m_focusPlacement=value;update();emit zoomChanged();}
    double zoom() const { return m_zoom; }
    void setZoom(double value);
    void paint(QPainter* painter) override;
signals:
    void sceneChanged();
    void hoveredPitchChanged();
    void zoomChanged();
    void pitchClicked(int pitch);
    void malletSelected(int mallet);
    void strikePointDragged(int mallet, double fraction);
protected:
    bool event(QEvent*) override;
    void hoverMoveEvent(QHoverEvent*) override;
    void hoverLeaveEvent(QHoverEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private:
    int pitchAt(QPointF point) const;
    void setHovered(int pitch);
    QVariantMap m_scene;
    QColor m_background { "#414549" };
    QTransform m_transform;
    double m_zoom = 1;
    int m_hoveredPitch = -1;
    int m_dragged = -1;
    int m_draggedPitch = -1;
    QPointF m_hoverPosition { -1, -1 };
    bool m_keyboardHover = false;
    bool m_focusPlacement = true;
};
}
