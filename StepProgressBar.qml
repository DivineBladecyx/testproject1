pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property var stepTitles: ["计划核对", "准备 / 摆位", "就绪 / 申请", "出束治疗", "结束 / 记录"]
    property int currentStep: 0

    // 柔和浅色主题色彩配置
    property color panelBg: "#FFFFFF"
    property color panelBorder: "#E2E8F0"
    property color activeAccent: "#0284C7" // 柔和医用蓝
    property color doneAccent: "#10B981"   // 柔和翠绿
    property color pendingColor: "#CBD5E1" // 浅灰导轨
    property color textPrimary: "#0F172A"  // 深色主标题
    property color textMuted: "#64748B"    // 柔和次级字

    implicitWidth: 800
    implicitHeight: 72

    // 柔和浅色外壳卡片
    Rectangle {
        anchors.fill: parent
        radius: 8
        color: root.panelBg
        border.color: root.panelBorder
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 0

            Repeater {
                model: root.stepTitles.length

                RowLayout {
                    id: stepItem
                    required property int index
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    property bool isDone: index < root.currentStep
                    property bool isActive: index === root.currentStep
                    property bool isPending: index > root.currentStep

                    // 左侧连接线（第一项除外）
                    Rectangle {
                        visible: stepItem.index > 0
                        Layout.fillWidth: true
                        Layout.preferredHeight: 2
                        radius: 1
                        color: stepItem.isDone || stepItem.isActive ? root.doneAccent : root.pendingColor
                        opacity: stepItem.isPending ? 0.35 : 1.0
                    }

                    // 步骤主体胶囊
                    Row {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 8

                        // 状态节点圆圈
                        Rectangle {
                            id: badgeCircle
                            width: 26
                            height: 26
                            radius: 13
                            anchors.verticalCenter: parent.verticalCenter
                            color: stepItem.isDone ? root.doneAccent :
                                   stepItem.isActive ? root.activeAccent : "#F1F5F9"
                            border.color: stepItem.isActive ? "#38BDF8" :
                                          stepItem.isDone ? "#34D399" : root.pendingColor
                            border.width: stepItem.isActive ? 2 : 1

                            // 极简温和呼吸动效
                            SequentialAnimation on opacity {
                                running: stepItem.isActive
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.8; duration: 900; easing.type: Easing.InOutQuad }
                                NumberAnimation { to: 1.0; duration: 900; easing.type: Easing.InOutQuad }
                            }

                            // 步骤序号或勾选
                            Text {
                                anchors.centerIn: parent
                                text: stepItem.isDone ? "✓" : (stepItem.index + 1).toString()
                                color: stepItem.isPending ? root.textMuted : "#FFFFFF"
                                font.pixelSize: stepItem.isDone ? 13 : 11
                                font.bold: true
                                font.family: "PingFang SC, -apple-system, sans-serif"
                            }
                        }

                        // 步骤文字标签
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.stepTitles[stepItem.index]
                            color: stepItem.isActive ? root.textPrimary :
                                   stepItem.isDone ? "#334155" : root.textMuted
                            font.pixelSize: 13
                            font.bold: stepItem.isActive
                            font.family: "PingFang SC, -apple-system, sans-serif"
                        }
                    }

                    // 右侧连接线（最后一项除外）
                    Rectangle {
                        visible: stepItem.index < root.stepTitles.length - 1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 2
                        radius: 1
                        color: stepItem.isDone ? root.doneAccent : root.pendingColor
                        opacity: stepItem.isPending ? 0.35 : 1.0
                    }
                }
            }
        }
    }
}
