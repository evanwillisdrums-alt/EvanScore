/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
import QtQuick
import Muse.Ui
Item {
    id: root
    objectName: "dynamics-curve"
    property int shape: 0
    property real bend: 0
    property int startLevel: 49
    property int endLevel: 112
    property int tapStartLevel: 49
    property int tapEndLevel: 49
    property bool showTapCurve: false
    property bool endpointsEditable: false
    property real previewBend: bend
    property int previewStart: startLevel
    property int previewEnd: endLevel
    signal curveEdited(int shape, real bend)
    signal endpointEdited(bool end, int velocity)
    implicitHeight: 188
    Accessible.name: qsTrc("notation", "Playback dynamics curve. Drag to reshape; double-click to make linear.")
    Accessible.role: Accessible.Slider
    onBendChanged: { if (!pointer.pressed) previewBend = bend; plot.requestPaint() }
    onTapStartLevelChanged: plot.requestPaint()
    onTapEndLevelChanged: plot.requestPaint()
    onShowTapCurveChanged: plot.requestPaint()
    onShapeChanged: plot.requestPaint()
    onStartLevelChanged: { if (!pointer.pressed) previewStart = startLevel; plot.requestPaint() }
    onEndLevelChanged: { if (!pointer.pressed) previewEnd = endLevel; plot.requestPaint() }
    onPreviewBendChanged: plot.requestPaint()
    onPreviewStartChanged: plot.requestPaint()
    onPreviewEndChanged: plot.requestPaint()
    function progress(t) {
        t = Math.max(0, Math.min(1, t))
        if (t === 0 || t === 1) return t
        if (shape === 1) {
            const k = 4 * Math.pow(2, previewBend)
            function logistic(x) { return 1 / (1 + Math.exp(-k * (x - .5))) }
            return (logistic(t) - logistic(0)) / (logistic(1) - logistic(0))
        }
        if (shape === 2) {
            const delay = Math.max(0, Math.min(.8, .4 + previewBend * .2))
            return Math.max(0, Math.min(1, (t - delay) / (1 - delay)))
        }
        return Math.pow(t, Math.pow(2, previewBend * 2))
    }
    function plotX(t) { return 22 + t * (width - 44) }
    function plotY(value) { return height - 25 - value / 127 * (height - 45) }
    Rectangle { anchors.fill: parent; radius: 10; color: ui.theme.backgroundPrimaryColor }
    Canvas {
        id: plot
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = ui.theme.strokeColor
            ctx.globalAlpha = .3
            ctx.lineWidth = 1
            for (let i = 0; i <= 4; ++i) {
                ctx.beginPath(); ctx.moveTo(root.plotX(0), root.plotY(i * 127 / 4)); ctx.lineTo(root.plotX(1), root.plotY(i * 127 / 4)); ctx.stroke()
                ctx.beginPath(); ctx.moveTo(root.plotX(i / 4), root.plotY(0)); ctx.lineTo(root.plotX(i / 4), root.plotY(127)); ctx.stroke()
            }
            ctx.globalAlpha = 1
            if (root.showTapCurve) {
                ctx.strokeStyle = ui.theme.fontPrimaryColor; ctx.lineWidth = 2; ctx.globalAlpha = .5
                ctx.setLineDash([4, 4]); ctx.beginPath()
                for (let i = 0; i <= 100; ++i) {
                    const t = i / 100, value = root.tapStartLevel + (root.tapEndLevel - root.tapStartLevel) * root.progress(t)
                    if (!i) ctx.moveTo(root.plotX(t), root.plotY(value)); else ctx.lineTo(root.plotX(t), root.plotY(value))
                }
                ctx.stroke(); ctx.setLineDash([]); ctx.globalAlpha = 1
            }
            ctx.strokeStyle = ui.theme.accentColor; ctx.lineWidth = 3
            ctx.beginPath()
            for (let i = 0; i <= 100; ++i) {
                const t = i / 100, value = root.previewStart + (root.previewEnd - root.previewStart) * root.progress(t)
                if (i === 0) ctx.moveTo(root.plotX(t), root.plotY(value)); else ctx.lineTo(root.plotX(t), root.plotY(value))
            }
            ctx.stroke()
            ctx.fillStyle = ui.theme.accentColor
            for (let t of [0, .5, 1]) {
                ctx.beginPath(); ctx.arc(root.plotX(t), root.plotY(root.previewStart + (root.previewEnd - root.previewStart) * root.progress(t)), t === .5 ? 6 : 4, 0, Math.PI * 2); ctx.fill()
            }
            ctx.fillStyle = ui.theme.fontPrimaryColor; ctx.font = "11px " + ui.theme.bodyFont.family
            ctx.fillText(String(root.previewStart), 4, 13); ctx.fillText(String(root.previewEnd), width - 26, 13)
            ctx.fillText("0%", root.plotX(0) - 7, height - 6); ctx.fillText("100%", root.plotX(1) - 25, height - 6)
        }
    }
    MouseArea {
        id: pointer
        objectName: "dynamics-curve-drag"
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.SizeAllCursor
        property int target: 0
        property real initialY: 0
        property real initialBend: 0
        onPressed: function(mouse) {
            root.forceActiveFocus()
            initialY = mouse.y; initialBend = root.previewBend
            target = root.endpointsEditable && mouse.x < 40 ? 1 : root.endpointsEditable && mouse.x > width - 40 ? 2 : 0
        }
        onPositionChanged: function(mouse) {
            if (!pressed) return
            if (target) {
                const v = Math.max(0, Math.min(127, Math.round((height - 25 - mouse.y) / (height - 45) * 127)))
                if (target === 1) root.previewStart = v; else root.previewEnd = v
            } else root.previewBend = Math.max(-2, Math.min(2, initialBend + (mouse.y - initialY) / 45 * (root.previewEnd >= root.previewStart ? 1 : -1)))
        }
        onReleased: {
            if (target) root.endpointEdited(target === 2, target === 1 ? root.previewStart : root.previewEnd)
            else root.curveEdited(root.shape, root.previewBend)
        }
        onCanceled: { root.previewBend = root.bend; root.previewStart = root.startLevel; root.previewEnd = root.endLevel }
        onDoubleClicked: { root.previewBend = 0; root.curveEdited(0, 0) }
    }
    focus: true
    Keys.onLeftPressed: { previewBend = Math.max(-2, previewBend - .05); curveEdited(shape, previewBend) }
    Keys.onRightPressed: { previewBend = Math.min(2, previewBend + .05); curveEdited(shape, previewBend) }
}
