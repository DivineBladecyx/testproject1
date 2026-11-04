import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 128
    height: 128

    property real percentage: 0.0 // 0 到 100
    property bool isBeamOn: false

    onPercentageChanged: canvas.requestPaint()
    onIsBeamOnChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var cx = width / 2
            var cy = height / 2
            var r = (width / 2) - 10
            var lineWidth = 8

            // 1. 底环 (270度仪表盘弧，或者整圆)
            var startAngle = 0.75 * Math.PI // 135度 (左下)
            var totalAngle = 1.5 * Math.PI  // 270度
            var endAngle = startAngle + totalAngle

            ctx.save()
            ctx.beginPath()
            ctx.arc(cx, cy, r, startAngle, endAngle, false)
            ctx.strokeStyle = "#e2e8f0"
            ctx.lineWidth = lineWidth
            ctx.lineCap = "round"
            ctx.stroke()
            ctx.restore()

            // 2. 进度弧
            if (root.percentage > 0) {
                var currentEnd = startAngle + totalAngle * (Math.min(100.0, Math.max(0.0, root.percentage)) / 100.0)

                ctx.save()
                ctx.beginPath()
                ctx.arc(cx, cy, r, startAngle, currentEnd, false)

                if (root.isBeamOn) {
                    ctx.strokeStyle = "#ef4444" // 出束中为警示红
                    ctx.shadowColor = "#ef4444"
                    ctx.shadowBlur = 6
                } else {
                    ctx.strokeStyle = "#2563eb" // 正常为科技蓝
                    ctx.shadowColor = "#3b82f6"
                    ctx.shadowBlur = 4
                }
                ctx.lineWidth = lineWidth
                ctx.lineCap = "round"
                ctx.stroke()
                ctx.restore()
            }
        }
    }

    // 环心文本
    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -4
        spacing: 1

        Text {
            text: Math.round(root.percentage) + "%"
            font.pixelSize: 18
            font.bold: true
            font.family: "Menlo"
            color: root.isBeamOn ? "#ef4444" : "#0f172a"
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            text: "完成度"
            font.pixelSize: 10
            color: "#64748b"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
