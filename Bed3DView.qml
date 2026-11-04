import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    width: 720
    height: 480

    // 控制器属性安全访问
    property real axisA: (typeof bedController !== "undefined" && bedController) ? bedController.a1 : 0.0
    property real axisB: (typeof bedController !== "undefined" && bedController) ? bedController.a2 : 0.0
    property real axisC: (typeof bedController !== "undefined" && bedController) ? bedController.a3 : 0.0
    property real axisX: (typeof bedController !== "undefined" && bedController) ? bedController.a4 : 0.0
    property real axisY: (typeof bedController !== "undefined" && bedController) ? bedController.a5 : 0.0
    property real axisZ: (typeof bedController !== "undefined" && bedController) ? bedController.a6 : 0.0
    property bool isAlarm: (typeof bedController !== "undefined" && bedController) ? bedController.alarm : false

    // 轻量数值平滑过渡
    Behavior on axisA { SmoothedAnimation { duration: 120 } }
    Behavior on axisB { SmoothedAnimation { duration: 120 } }
    Behavior on axisC { SmoothedAnimation { duration: 120 } }
    Behavior on axisX { SmoothedAnimation { duration: 120 } }
    Behavior on axisY { SmoothedAnimation { duration: 120 } }
    Behavior on axisZ { SmoothedAnimation { duration: 120 } }

    // 柔和护眼浅灰底板
    Rectangle {
        anchors.fill: parent
        color: "#F1F5F9"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        // ==========================================
        // 1. 顶部状态栏（柔和浅色）
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                implicitHeight: 30
                implicitWidth: statusRow.width + 20
                radius: 6
                color: "#FFFFFF"
                border.color: root.isAlarm ? "#FECDD3" : "#E2E8F0"
                border.width: 1

                Row {
                    id: statusRow
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        width: 7
                        height: 7
                        radius: 3.5
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.isAlarm ? "#E11D48" : "#10B981"
                    }

                    Text {
                        text: root.isAlarm ? "安全联锁：受限" : "床位监控：实时就绪"
                        color: root.isAlarm ? "#BE123C" : "#334155"
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "PingFang SC, -apple-system, sans-serif"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "等中心基准坐标系 (Isocenter Frame)"
                color: "#64748B"
                font.pixelSize: 11
                font.family: "PingFang SC, -apple-system, sans-serif"
            }
        }

        // ==========================================
        // 2. 6 轴极简坐标仪表网格 (柔和浅色卡片)
        // ==========================================
        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 10

            Repeater {
                model: [
                    { name: "X 轴", desc: "侧向位移 (Lateral)", val: root.axisX, unit: "mm", min: -200, max: 200, color: "#0284C7" },
                    { name: "A 轴", desc: "旋转角 (Yaw)", val: root.axisA, unit: "°", min: -180, max: 180, color: "#059669" },
                    { name: "Y 轴", desc: "纵向进深 (Longitudinal)", val: root.axisY, unit: "mm", min: -600, max: 600, color: "#0284C7" },
                    { name: "B 轴", desc: "俯仰角 (Pitch)", val: root.axisB, unit: "°", min: -5, max: 5, color: "#059669" },
                    { name: "Z 轴", desc: "垂直升降 (Vertical)", val: root.axisZ, unit: "mm", min: -200, max: 200, color: "#0284C7" },
                    { name: "C 轴", desc: "翻滚角 (Roll)", val: root.axisC, unit: "°", min: -5, max: 5, color: "#059669" }
                ]

                // 单轴柔和卡片
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 8
                    color: "#FFFFFF"
                    border.color: "#E2E8F0"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        // 头部：轴标签与数值
                        RowLayout {
                            Layout.fillWidth: true

                            Rectangle {
                                implicitWidth: 30
                                implicitHeight: 20
                                radius: 4
                                color: "#F1F5F9"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.name.substring(0, 1)
                                    color: modelData.color
                                    font.pixelSize: 12
                                    font.bold: true
                                    font.family: "Menlo, Monaco, sans-serif"
                                }
                            }

                            Text {
                                text: modelData.desc
                                color: "#64748B"
                                font.pixelSize: 11
                                font.family: "PingFang SC, -apple-system, sans-serif"
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: (modelData.val >= 0 ? "+" : "") + modelData.val.toFixed(2)
                                color: "#0F172A"
                                font.pixelSize: 20
                                font.bold: true
                                font.family: "Menlo, Monaco, monospace"
                            }

                            Text {
                                text: modelData.unit
                                color: "#94A3B8"
                                font.pixelSize: 12
                                font.family: "Menlo, Monaco, monospace"
                            }
                        }

                        Item { Layout.fillHeight: true }

                        // 柔和双向动态刻度槽
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 6
                            radius: 3
                            color: "#E2E8F0"

                            // 零位中线
                            Rectangle {
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                width: 2
                                height: parent.height + 4
                                color: "#94A3B8"
                            }

                            // 动态游标条
                            Rectangle {
                                property real ratio: Math.max(-1, Math.min(1, modelData.val / modelData.max))
                                height: parent.height
                                radius: 3
                                color: root.isAlarm ? "#E11D48" : modelData.color
                                x: ratio >= 0 ? parent.width / 2 : (parent.width / 2) + ratio * (parent.width / 2)
                                width: Math.abs(ratio) * (parent.width / 2)

                                Behavior on width { SmoothedAnimation { duration: 120 } }
                                Behavior on x { SmoothedAnimation { duration: 120 } }
                            }
                        }

                        // 刻度标尺两极
                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: modelData.min + " " + modelData.unit
                                color: "#94A3B8"
                                font.pixelSize: 10
                                font.family: "Menlo, Monaco, monospace"
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: "0"
                                color: "#94A3B8"
                                font.pixelSize: 10
                                font.family: "Menlo, Monaco, monospace"
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                text: "+" + modelData.max + " " + modelData.unit
                                color: "#94A3B8"
                                font.pixelSize: 10
                                font.family: "Menlo, Monaco, monospace"
                            }
                        }
                    }
                }
            }
        }
    }
}
