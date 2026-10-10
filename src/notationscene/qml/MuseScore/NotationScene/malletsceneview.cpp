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
    if (body.isNull()) body = {keyboardWidth / 2, front + 28};
    const QColor skin(m_scene.value("skin").toString()), hair(m_scene.value("hair").toString());
    // Compact mode crops unused register, rather than shrinking the whole
    // player. The overview preserves the complete range. All physical points
    // use the same uniform scale/inverse transform as hit testing.
    const auto active=pose.value("pitches").toList();
    double left=-5,right=keyboardWidth+5,top=0,bottom=std::max(front+55,body.y()+24);
    if(m_focusPlacement && !active.empty()) {
        left=body.x()-30;right=body.x()+30;top=front;bottom=body.y()+24;
        for(const auto& value:bars) {const auto bar=value.toMap();if(!active.contains(bar.value("pitch")))continue;
            left=std::min(left,bar.value("x").toDouble()-12);right=std::max(right,bar.value("x").toDouble()+bar.value("width").toDouble()+12);
            top=std::min(top,bar.value("y").toDouble()-6);
        }
        for(const auto& value:pose.value("wrists").toList()) {const auto w=location(value.toMap());left=std::min(left,w.x()-10);right=std::max(right,w.x()+10);bottom=std::max(bottom,w.y()+10);}
    }
    const double overview=20;
    const double scale=std::max(.01,std::min((width()-20)/std::max(1.0,right-left),(height()-overview-10)/std::max(1.0,bottom-top)))*m_zoom;
    const double center=m_zoom>1?body.x():(left+right)/2;
    m_transform=QTransform();m_transform.translate(width()/2-center*scale,overview+(height()-overview)/2-(top+bottom)*scale/2);m_transform.scale(scale,scale);
    // Range overview is deliberately a compact navigator, not a second
    // physical scene. The detailed scene below retains true proportions.
    const double overviewScale=(width()-24)/std::max(1.0,keyboardWidth);
    p->setPen(Qt::NoPen);
    for(const auto& value:bars) {const auto bar=value.toMap();const bool selected=active.contains(bar.value("pitch"));
        p->setBrush(selected?QColor("#78bafa"):bar.value("accidental").toBool()?QColor("#bb987b"):QColor("#8b7667"));
        p->drawRoundedRect(QRectF(12+bar.value("x").toDouble()*overviewScale,bar.value("accidental").toBool()?3:10,
            std::max(1.0,bar.value("width").toDouble()*overviewScale),6),1,1);
    }
    if(m_focusPlacement && !active.empty()) {p->setPen(QPen(QColor("#b9cbd9"),1));p->setBrush(Qt::NoBrush);p->drawRoundedRect(QRectF(12+std::max(0.0,left)*overviewScale,1,(std::min(keyboardWidth,right)-std::max(0.0,left))*overviewScale,17),2,2);}
    p->save();p->setClipRect(QRectF(0,overview,width(),height()-overview));
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
            const QColor color = pose.value("uncertain").toBool() ? QColor("#afb6be") : id >= 0 && id < 4 ? colors[id] : QColor("#e77878");
            p->setBrush(QColor(color.red(), color.green(), color.blue(), 70)); p->setPen(QPen(color, 1.4)); p->drawRoundedRect(rect, .7, .7);
            p->setPen(Qt::NoPen);p->setBrush(QColor(120,196,150,45));p->drawRect(QRectF(rect.x(),rect.y()+rect.height()*.35,rect.width(),rect.height()*.30));
            p->setBrush(QColor(230,180,95,45));p->drawRect(QRectF(rect.x(),rect.y()+rect.height()*.90,rect.width(),rect.height()*.10));
            p->setPen(QPen(QColor(230,210,155,140),.5,Qt::DashLine));
            for(double fraction:{.224,.776}) p->drawLine(QPointF(rect.x(),rect.y()+rect.height()*fraction),QPointF(rect.right(),rect.y()+rect.height()*fraction));
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
    const auto anchors=pose.value("anchors").toList();
    bool anyHead = false; for (const auto& h : heads) anyHead |= h.toMap().value("active").toBool();
    for (int hand = 0; hand < 2; ++hand) {
        const double sign = hand ? 1 : -1;
        const QPointF shoulder = body + QPointF(sign * 21, -1);
        const QPointF wrist = anyHead && hand < wrists.size() ? location(wrists[hand].toMap()) : body + QPointF(sign * 21, -28);
        const QPointF elbow((shoulder.x()+wrist.x())*.5+sign*7, (shoulder.y()+wrist.y())*.5+5);
        QPainterPath arm; arm.moveTo(shoulder); arm.lineTo(elbow); arm.lineTo(wrist);
        p->setPen(QPen(QColor(0,0,0,35), 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p->drawPath(arm.translated(1,1));
        p->setPen(QPen(skin, 5.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p->drawPath(arm);
        p->setPen(Qt::NoPen);
    }
    p->setBrush(skin); p->drawEllipse(body + QPointF(0,-7), 11, 12);
    p->setBrush(hair); p->drawEllipse(body + QPointF(0,-10), 11, 12); p->drawEllipse(body + QPointF(0,-1), 6, 7);
    // Shafts extend beyond their distinct holding points. Painting palms and
    // wrapped fingers afterward places the shaft through, not on top of, the hand.
    for(int m=0;m<heads.size() && m<4;++m) {
        const auto head=heads[m].toMap();if(!head.value("active").toBool() || m/2>=wrists.size())continue;
        const auto target=location(head);const auto hold=m<anchors.size()?location(anchors[m].toMap()):location(wrists[m/2].toMap())+QPointF(m%2?2:-2,0);
        const auto vector=hold-target;const double length=std::max(1.0,std::hypot(vector.x(),vector.y()));const auto tail=hold+vector*(6/length);
        p->setPen(QPen(QColor("#c2a474"),1.25,Qt::SolidLine,Qt::RoundCap));p->drawLine(tail,target);
    }
    for(int hand=0;hand<2;++hand) {
        const double sign=hand?1:-1;const auto wrist=anyHead && hand<wrists.size()?location(wrists[hand].toMap()):body+QPointF(sign*21,-28);
        p->setPen(Qt::NoPen);p->setBrush(skin);p->drawRoundedRect(QRectF(wrist.x()-3,wrist.y()-1,6,6),2,2);
        const int inner=hand?2:1,outer=hand?3:0;
        const auto innerHold=inner<anchors.size()?location(anchors[inner].toMap()):wrist+QPointF(-sign*2,-1);
        const auto outerHold=outer<anchors.size()?location(anchors[outer].toMap()):wrist+QPointF(sign*2,1);
        p->setPen(QPen(skin.lighter(108),1.5,Qt::SolidLine,Qt::RoundCap));
        // Thumb/index independently secure the inner mallet; ring/little
        // wrap the outer shaft. Middle finger separates the holding regions.
        p->drawLine(innerHold+QPointF(-sign*1.5,1.5),innerHold+QPointF(sign*.7,-1));
        p->drawLine(innerHold+QPointF(-sign*1,3),innerHold+QPointF(sign*1.1,.1));
        p->drawLine(wrist+QPointF(0,3.5),wrist+QPointF(sign*.5,0));
        p->drawLine(outerHold+QPointF(-sign*.8,3),outerHold+QPointF(sign*1.2,.5));
        p->drawLine(outerHold+QPointF(-sign*.1,4),outerHold+QPointF(sign*1.4,1.4));
        p->setPen(QPen(skin.darker(125),.35));p->drawLine(innerHold+QPointF(-sign*1,2),innerHold+QPointF(sign,1));p->drawLine(outerHold+QPointF(0,2),outerHold+QPointF(sign,1));
    }
    for(int m=0;m<heads.size() && m<4;++m) {
        const auto head=heads[m].toMap();if(!head.value("active").toBool())continue;const auto target=location(head);
        const double radius=std::max(2.2,4.5/scale);
        p->setPen(QPen(QColor("#3a4458"),.5));p->setBrush(colors[m]);p->drawEllipse(target,radius,radius);
        p->setPen(QColor("#ffffff"));QFont font;font.setPixelSize(std::max(4,int(9/scale)));font.setBold(true);p->setFont(font);
        const int label=m_scene.value("reverseNumbering").toBool()?4-m:m+1;
        p->drawText(QRectF(target.x()-radius*1.5,target.y()-radius*1.5,radius*3,radius*3),Qt::AlignCenter,QString::number(label));
    }
    p->restore();

}
int MalletSceneView::pitchAt(QPointF point) const {
    if(point.y()<20) return -1;
    const QPointF pos = m_transform.inverted().map(point); const auto bars = m_scene.value("bars").toList();
    for (int row = 1; row >= 0; --row) for (int i = bars.size()-1; i >= 0; --i) {
        const auto bar = bars[i].toMap();
        if (bar.value("accidental").toBool() != bool(row)) continue;
        if (QRectF(bar.value("x").toDouble(), bar.value("y").toDouble(), bar.value("width").toDouble(), bar.value("length").toDouble()).contains(pos)) return bar.value("pitch").toInt();
    }
    return -1;
}
void MalletSceneView::setHovered(int pitch) { if (pitch == m_hoveredPitch) return; m_hoveredPitch = pitch; update(); emit hoveredPitchChanged(); }
void MalletSceneView::hoverMoveEvent(QHoverEvent* event) {
    // Qt can redeliver hover after repaint. A stationary pointer must not
    // immediately erase the bar selected with arrow keys.
    if (m_keyboardHover && event->position() == m_hoverPosition) return;
    m_keyboardHover = false; m_hoverPosition = event->position();
    setHovered(pitchAt(event->position()));
}
void MalletSceneView::hoverLeaveEvent(QHoverEvent*) { m_keyboardHover = false; m_hoverPosition = {-1, -1}; setHovered(-1); }
void MalletSceneView::mousePressEvent(QMouseEvent* event) {
    forceActiveFocus();

    const auto pose = m_scene.value("pose").toMap(); const auto heads = pose.value("heads").toList();
    for (const auto& h : heads) {
        const auto head = h.toMap();
        if (head.value("active").toBool() && QLineF(event->position(), m_transform.map(location(head))).length() < 9) {
            m_dragged = head.value("id").toInt(); emit malletSelected(m_dragged);
            const auto ids = pose.value("mallets").toList(); const auto pitches = pose.value("pitches").toList();
            const int index = ids.indexOf(m_dragged); m_draggedPitch = index >= 0 ? pitches[index].toInt() : -1;
            event->accept(); return;
        }
    }
    const int pitch = pitchAt(event->position());
    if(pitch>=0) {
        const auto pitches=pose.value("pitches").toList(),ids=pose.value("mallets").toList();const int index=pitches.indexOf(pitch);
        if(index>=0 && index<ids.size() && !m_scene.value("pickMode").toBool()) {
            m_dragged=ids[index].toInt();m_draggedPitch=pitch;emit malletSelected(m_dragged);
        } else emit pitchClicked(pitch);
    }
    event->accept();
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
bool MalletSceneView::event(QEvent* event) {
    if (event->type() == QEvent::ShortcutOverride && hasActiveFocus()) {
        const auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Left || key->key() == Qt::Key_Right
            || key->key() == Qt::Key_Space || key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            event->accept(); return true;
        }
    }
    return QQuickPaintedItem::event(event);
}
void MalletSceneView::wheelEvent(QWheelEvent* event) { setZoom(m_zoom + (event->angleDelta().y() > 0 ? .2 : -.2)); event->accept(); }
void MalletSceneView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        m_keyboardHover = true;
        int next = m_hoveredPitch < 0 ? m_scene.value("low").toInt() : m_hoveredPitch + (event->key() == Qt::Key_Left ? -1 : 1);
        setHovered(std::clamp(next, m_scene.value("low").toInt(), m_scene.value("high").toInt())); event->accept();
    } else if ((event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && m_hoveredPitch >= 0) { emit pitchClicked(m_hoveredPitch); event->accept(); }
    else QQuickPaintedItem::keyPressEvent(event);
}
