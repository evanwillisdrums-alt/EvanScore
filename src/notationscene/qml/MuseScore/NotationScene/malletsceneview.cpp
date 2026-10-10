/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "malletsceneview.h"
#include <QPainter>
#include <QPainterPath>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <algorithm>
#include <cmath>
using namespace mu::notation;
static QPointF location(const QVariantMap& p) { return {p.value("x").toDouble(), p.value("y").toDouble()}; }
MalletSceneView::MalletSceneView(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton); setAcceptHoverEvents(true); setAntialiasing(true);
    setFlag(ItemIsFocusScope); setActiveFocusOnTab(true);
}
void MalletSceneView::setScene(const QVariantMap& value) { if (value == m_scene) return; m_scene = value; update(); emit sceneChanged(); }
void MalletSceneView::setZoom(double value) { value = std::clamp(value, 1.0, 4.0); if (std::abs(value - m_zoom) < 0.001) return; m_zoom = value; update(); emit zoomChanged(); }
void MalletSceneView::paint(QPainter* p) {
    p->setRenderHint(QPainter::Antialiasing); p->fillRect(boundingRect(), m_background);
    const auto bars = m_scene.value("bars").toList();
    if (bars.empty()) return;
    const auto pose = m_scene.value("pose").toMap();
    const double keyboardWidth = m_scene.value("keyboardWidth").toDouble();
    const double front = m_scene.value("keyboardFront").toDouble();
    QPointF body = location(pose.value("body").toMap());
    if (body.isNull()) body = {keyboardWidth / 2, front + 43};
    const QColor skin(m_scene.value("skin").toString()), hair(m_scene.value("hair").toString());
    if (m_scene.value("sideView").toBool()) {
        // A front elevation, deliberately without mallet movement.
        const double scale = std::min((width() - 32) / std::max(1.0, keyboardWidth), (height() - 24) / 100.0);
        p->translate((width() - keyboardWidth * scale) / 2, (height() - 85 * scale) / 2); p->scale(scale, scale);
        p->setPen(Qt::NoPen); p->setBrush(QColor("#262b31")); p->drawRoundedRect(QRectF(0, 25, keyboardWidth, 7), 2, 2);
        for (const auto& value : bars) {
            const auto bar = value.toMap();
            if (bar.value("accidental").toBool()) continue;
            const double x = bar.value("x").toDouble(), w = bar.value("width").toDouble();
            p->setBrush(m_scene.value("metal").toBool() ? QColor("#b8c1c7") : QColor("#b97850")); p->drawRoundedRect(QRectF(x, 22, w, 3), 0.8, 0.8);
            p->setBrush(QColor("#ac9168")); p->drawRoundedRect(QRectF(x + w * .2, 32, w * .6, bar.value("length").toDouble() * .65), 1, 1);
        }
        p->setPen(QPen(QColor("#28313a"), 4, Qt::SolidLine, Qt::RoundCap)); p->drawLine(QPointF(7, 29), QPointF(7, 93)); p->drawLine(QPointF(keyboardWidth-7, 29), QPointF(keyboardWidth-7, 93));
        p->setPen(Qt::NoPen); p->setBrush(QColor("#516389")); p->drawRoundedRect(QRectF(body.x()-17, 6, 34, 28), 8, 8);
        p->setBrush(skin); p->drawEllipse(QPointF(body.x(), -2), 8, 10); p->setBrush(hair); p->drawChord(QRectF(body.x()-9, -13, 18, 18), 0, 180*16);
        p->setPen(QPen(skin, 5, Qt::SolidLine, Qt::RoundCap)); p->drawLine(QPointF(body.x()-15, 13), QPointF(body.x()-23, 25)); p->drawLine(QPointF(body.x()+15, 13), QPointF(body.x()+23, 25));
        return;
    }
    const double fullHeight = front + 77;
    const double scale = std::max(.01, std::min((width()-32) / (keyboardWidth+10), (height()-22) / fullHeight)) * m_zoom;
    const double center = m_zoom > 1 ? body.x() : keyboardWidth / 2;
    m_transform = QTransform(); m_transform.translate(width()/2 - center*scale, height()/2 - fullHeight*scale/2); m_transform.scale(scale, scale);
    p->setTransform(m_transform);
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0, 0, 0, 35)); p->drawRoundedRect(QRectF(-4, 58, keyboardWidth+10, front-52), 3, 3);
    p->setBrush(QColor("#303338")); p->drawRoundedRect(QRectF(-3, 58, keyboardWidth+6, 7), 2, 2); p->drawRoundedRect(QRectF(-3, front-4, keyboardWidth+6, 6), 2, 2);
    const auto activePitches = pose.value("pitches").toList();
    const auto heldPitches = m_scene.value("heldPitches").toList();
    const auto mallets = pose.value("mallets").toList();
    const QColor colors[] {QColor("#5da9f4"), QColor("#72d0cf"), QColor("#f3c560"), QColor("#ed8a9e")};
    for (int row = 0; row < 2; ++row) for (const auto& value : bars) {
        const auto bar = value.toMap(); if (bar.value("accidental").toBool() != bool(row)) continue;
        const int pitch = bar.value("pitch").toInt();
        const QRectF rect(bar.value("x").toDouble(), bar.value("y").toDouble(), bar.value("width").toDouble(), bar.value("length").toDouble());
        p->setPen(Qt::NoPen); p->setBrush(QColor(0,0,0,50)); p->drawRoundedRect(rect.translated(.6, 1), .8, .8);
        QLinearGradient material(rect.topLeft(), rect.topRight());
        const QColor wood = m_scene.value("metal").toBool() ? QColor("#b2bec8") : row ? QColor("#d19970") : QColor("#b87952");
        material.setColorAt(0, wood.lighter(113)); material.setColorAt(.65, wood); material.setColorAt(1, wood.darker(114));
        p->setBrush(material); p->drawRoundedRect(rect, .7, .7);
        const auto found = std::find(activePitches.begin(), activePitches.end(), QVariant(pitch));
        if (found != activePitches.end()) {
            const int index = std::distance(activePitches.begin(), found);
            const int id = index < mallets.size() ? mallets[index].toInt() - 1 : -1;
            const QColor color = id >= 0 && id < 4 ? colors[id] : QColor("#e77878");
            p->setBrush(QColor(color.red(), color.green(), color.blue(), 70)); p->setPen(QPen(color, 1.4)); p->drawRoundedRect(rect, .7, .7);
        } else if (heldPitches.contains(pitch)) { p->setBrush(QColor(255,255,255,45)); p->setPen(QPen(QColor("#d9e2eb"), .6, Qt::DashLine)); p->drawRoundedRect(rect, .7, .7); }
        if (pitch == m_hoveredPitch) { p->setBrush(Qt::NoBrush); p->setPen(QPen(QColor("#f6f7f8"), 1)); p->drawRoundedRect(rect, .7, .7); }
        p->setPen(QColor("#f9eee5")); QFont font; font.setPixelSize(std::max(3, int(9/scale))); p->setFont(font);
        if (pitch % 12 == 0 || (found != activePitches.end() && rect.width()*scale > 12))
            p->drawText(QRectF(rect.x()-1, rect.bottom()-7, rect.width()+2, 6), Qt::AlignCenter, bar.value("label").toString());
        p->setPen(QPen(wood.darker(130), .2)); p->drawLine(QPointF(rect.x()+rect.width()*.3, rect.y()+3), QPointF(rect.x()+rect.width()*.3, rect.bottom()-9));
    }
    // Body stays below the instrument; mallet targets are painted last.
    p->setPen(Qt::NoPen); p->setBrush(QColor(0,0,0,30)); p->drawEllipse(body + QPointF(3,6), 27, 20);
    p->setBrush(QColor("#53658c")); p->drawEllipse(body + QPointF(0,5), 25, 17);
    const auto wrists = pose.value("wrists").toList(); const auto heads = pose.value("heads").toList();
    bool anyHead = false; for (const auto& h : heads) anyHead |= h.toMap().value("active").toBool();
    for (int hand = 0; hand < 2; ++hand) {
        const double sign = hand ? 1 : -1;
        const QPointF shoulder = body + QPointF(sign * 21, -1);
        const QPointF wrist = anyHead && hand < wrists.size() ? location(wrists[hand].toMap()) : body + QPointF(sign * 21, -28);
        const QPointF elbow((shoulder.x()+wrist.x())*.5+sign*7, (shoulder.y()+wrist.y())*.5+5);
        QPainterPath arm; arm.moveTo(shoulder); arm.lineTo(elbow); arm.lineTo(wrist);
        p->setPen(QPen(QColor(0,0,0,35), 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p->drawPath(arm.translated(1,1));
        p->setPen(QPen(skin, 5.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p->drawPath(arm);
        p->setBrush(skin); p->setPen(Qt::NoPen); p->drawEllipse(wrist, 3.2, 4);
    }
    p->setBrush(skin); p->drawEllipse(body + QPointF(0,-7), 11, 12);
    p->setBrush(hair); p->drawEllipse(body + QPointF(0,-10), 11, 12); p->drawEllipse(body + QPointF(0,-1), 6, 7);
    for (int m = 0; m < heads.size() && m < 4; ++m) {
        const auto head = heads[m].toMap(); if (!head.value("active").toBool()) continue;
        const auto target = location(head); const auto wrist = location(wrists[m/2].toMap());
        p->setPen(QPen(QColor("#aa694b"), 1.4, Qt::SolidLine, Qt::RoundCap)); p->drawLine(wrist, target);
        p->setPen(QPen(QColor("#3a4458"), .5)); p->setBrush(colors[m]); p->drawEllipse(target, 2.2, 2.2);
        p->setPen(QColor("#ffffff")); QFont font; font.setPixelSize(4); font.setBold(true); p->setFont(font);
        p->drawText(QRectF(target.x()-3, target.y()-3, 6, 6), Qt::AlignCenter, QString::number(m+1));
    }
}
int MalletSceneView::pitchAt(QPointF point) const {
    if (m_scene.value("sideView").toBool()) return -1;
    const QPointF pos = m_transform.inverted().map(point); const auto bars = m_scene.value("bars").toList();
    for (int row = 1; row >= 0; --row) for (int i = bars.size()-1; i >= 0; --i) {
        const auto bar = bars[i].toMap();
        if (bar.value("accidental").toBool() != bool(row)) continue;
        if (QRectF(bar.value("x").toDouble(), bar.value("y").toDouble(), bar.value("width").toDouble(), bar.value("length").toDouble()).contains(pos)) return bar.value("pitch").toInt();
    }
    return -1;
}
void MalletSceneView::setHovered(int pitch) { if (pitch == m_hoveredPitch) return; m_hoveredPitch = pitch; update(); emit hoveredPitchChanged(); }
void MalletSceneView::hoverMoveEvent(QHoverEvent* event) { setHovered(pitchAt(event->position())); }
void MalletSceneView::hoverLeaveEvent(QHoverEvent*) { setHovered(-1); }
void MalletSceneView::mousePressEvent(QMouseEvent* event) {
    forceActiveFocus();
    if (m_scene.value("sideView").toBool()) { event->accept(); return; }
    const auto pose = m_scene.value("pose").toMap(); const auto heads = pose.value("heads").toList();
    for (const auto& h : heads) {
        const auto head = h.toMap();
        if (head.value("active").toBool() && QLineF(event->position(), m_transform.map(location(head))).length() < 9) {
            m_dragged = head.value("id").toInt();
            const auto ids = pose.value("mallets").toList(); const auto pitches = pose.value("pitches").toList();
            const int index = ids.indexOf(m_dragged); m_draggedPitch = index >= 0 ? pitches[index].toInt() : -1;
            event->accept(); return;
        }
    }
    const int pitch = pitchAt(event->position()); if (pitch >= 0) emit pitchClicked(pitch); event->accept();
}
void MalletSceneView::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragged < 0) return;
    const auto position = m_transform.inverted().map(event->position());
    for (const auto& value : m_scene.value("bars").toList()) {
        const auto bar = value.toMap();
        if (bar.value("pitch").toInt() == m_draggedPitch) {
            emit strikePointDragged(m_dragged, (position.y() - bar.value("y").toDouble()) / bar.value("length").toDouble()); break;
        }
    }
    event->accept();
}
void MalletSceneView::mouseReleaseEvent(QMouseEvent* event) { m_dragged = -1; m_draggedPitch = -1; event->accept(); }
void MalletSceneView::wheelEvent(QWheelEvent* event) { setZoom(m_zoom + (event->angleDelta().y() > 0 ? .2 : -.2)); event->accept(); }
void MalletSceneView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        int next = m_hoveredPitch < 0 ? m_scene.value("low").toInt() : m_hoveredPitch + (event->key() == Qt::Key_Left ? -1 : 1);
        setHovered(std::clamp(next, m_scene.value("low").toInt(), m_scene.value("high").toInt())); event->accept();
    } else if ((event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) && m_hoveredPitch >= 0) { emit pitchClicked(m_hoveredPitch); event->accept(); }
    else QQuickPaintedItem::keyPressEvent(event);
}
