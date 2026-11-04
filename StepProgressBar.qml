pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

Item {
    id: root

    // 当前主阶段索引 (0 ~ 4)
    property int currentStep: 1 // 默认为 02 叫号

    // 5 大主阶段分组定义（与网页版流程监控一致）
    readonly property var stages: [
        { num: "01", name: "初始化", progress: "1 / 1", total: 1, done: 1 },
        { num: "02", name: "叫号",   progress: "2 / 3", total: 3, done: 2 },
        { num: "03", name: "准备",   progress: "0 / 2", total: 2, done: 0 },
        { num: "04", name: "出束",   progress: "0 / 2", total: 2, done: 0 },
        { num: "05", name: "结束",   progress: "0 / 2", total: 2, done: 0 }
    ]

    // 柔和色彩方案
    readonly property color panelBg: "#FFFFFF"
    readonly property color panelBorder: "#E2E8F0"
    readonly property color textPrimary: "#0F172A"
    readonly property color textMuted: "#64748B"
    readonly property color activeBlue: "#2563EB"
    readonly property color doneGreen: "#10B981"
    readonly property color pendingGray: "#94A3B8"

    implicitWidth: 800
    implicitHeight: 74

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: root.panelBg
        border.color: root.panelBorder
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            // 顶部：标题与状态图例 (完全对标网页版)
            RowLayout {
                Layout.fillWidth: true

                Row {
                    spacing: 6
                    Rectangle {
                        width: 14
                        height: 14
                        radius: 7
                        color: "#EFF6FF"
                        border.color: "#BFDBFE"
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            anchors.centerIn: parent
                            text: "⏱"
                            font.pixelSize: 9
                        }
                    }
                    Text {
                        text: "流程监控"
                        font.pixelSize: 12
                        font.bold: true
                        color: root.textPrimary
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item { Layout.fillWidth: true }

                // 右侧图例
                Row {
                    spacing: 12
                    Row {
                        spacing: 4
                        Rectangle { width: 7; height: 7; radius: 3.5; color: root.doneGreen; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "已完成"; font.pixelSize: 11; color: root.textMuted }
                    }
                    Row {
                        spacing: 4
                        Rectangle { width: 7; height: 7; radius: 3.5; color: root.activeBlue; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "执行中"; font.pixelSize: 11; color: root.textMuted }
                    }
                    Row {
                        spacing: 4
                        Rectangle { width: 7; height: 7; radius: 3.5; color: root.pendingGray; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "待执行"; font.pixelSize: 11; color: root.textMuted }
                    }
                }
            }

            // 下部：5 个阶段卡片
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Repeater {
                    model: root.stages.length

                    Rectangle {
                        id: stageCard
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6

                        readonly property bool isDone: stageCard.index < root.currentStep
                        readonly property bool isActive: stageCard.index === root.currentStep
                        readonly property bool isPending: stageCard.index > root.currentStep

                        color: stageCard.isActive ? "#EFF6FF" :
                               stageCard.isDone ? "#F0FDF4" : "#F8FAFC"
                        border.color: stageCard.isActive ? "#3B82F6" :
                                      stageCard.isDone ? "#86EFAC" : "#E2E8F0"
                        border.width: stageCard.isActive ? 1.5 : 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: root.stages[stageCard.index].num + " " + root.stages[stageCard.index].name
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: stageCard.isActive ? root.activeBlue :
                                           stageCard.isDone ? "#15803D" : root.textMuted
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: root.stages[stageCard.index].progress
                                    font.pixelSize: 10
                                    font.family: "Menlo"
                                    color: stageCard.isActive ? root.activeBlue :
                                           stageCard.isDone ? "#16A34A" : "#94A3B8"
                                }
                            }

                            // 底部细长微进度条
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 3
                                radius: 1.5
                                color: "#E2E8F0"

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    radius: 1.5
                                    width: parent.width * (
                                        stageCard.isDone ? 1.0 :
                                        stageCard.isActive ? (root.stages[stageCard.index].done / root.stages[stageCard.index].total) : 0.0
                                    )
                                    color: stageCard.isDone ? root.doneGreen : root.activeBlue
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.currentStep = stageCard.index;
                            }
                        }
                    }
                }
            }
        }
    }
}
