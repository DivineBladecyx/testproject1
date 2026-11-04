pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

Item {
    id: root

    // 当前具体的 12 步工作流索引 (0 ~ 11，默认为 2 即 03 排程叫号)
    property int currentWorkflowStep: 2

    // 当用户点击阶段卡片时通知外部
    signal stageSelected(int stageIndex)

    // 5 大主阶段分组定义（与网页版流程监控 100% 对应）
    readonly property var stages: [
        { num: "01", name: "初始化", totalSteps: 1, startStep: 0, endStep: 0 },
        { num: "02", name: "叫号",   totalSteps: 3, startStep: 1, endStep: 3 },
        { num: "03", name: "准备",   totalSteps: 2, startStep: 4, endStep: 5 },
        { num: "04", name: "出束",   totalSteps: 2, startStep: 6, endStep: 9 },
        { num: "05", name: "结束",   totalSteps: 2, startStep: 10, endStep: 11 }
    ]

    // 根据当前具体步骤计算所属的主阶段 (0 ~ 4)
    function currentStageIndex() {
        var s = root.currentWorkflowStep;
        if (s <= 0) return 0;
        if (s <= 3) return 1;
        if (s <= 5) return 2;
        if (s <= 9) return 3;
        return 4;
    }

    // 动态计算每个阶段的进度文字（如 1/1, 2/3, 0/2）
    function stageProgressText(stageIdx) {
        var curStage = currentStageIndex();
        var cfg = stages[stageIdx];
        if (stageIdx < curStage) {
            return cfg.totalSteps + " / " + cfg.totalSteps;
        } else if (stageIdx === curStage) {
            var doneInStage = 1;
            if (stageIdx === 0) doneInStage = 1;
            else if (stageIdx === 1) doneInStage = Math.min(cfg.totalSteps, Math.max(1, root.currentWorkflowStep - cfg.startStep + 1));
            else if (stageIdx === 2) doneInStage = Math.min(cfg.totalSteps, Math.max(1, root.currentWorkflowStep - cfg.startStep + 1));
            else if (stageIdx === 3) doneInStage = Math.min(cfg.totalSteps, Math.max(1, Math.floor((root.currentWorkflowStep - cfg.startStep) / 2) + 1));
            else doneInStage = Math.min(cfg.totalSteps, Math.max(1, root.currentWorkflowStep - cfg.startStep + 1));
            return doneInStage + " / " + cfg.totalSteps;
        } else {
            return "0 / " + cfg.totalSteps;
        }
    }

    // 动态计算每个阶段的进度比率 (0.0 ~ 1.0)
    function stageProgressRatio(stageIdx) {
        var curStage = currentStageIndex();
        var cfg = stages[stageIdx];
        if (stageIdx < curStage) return 1.0;
        if (stageIdx > curStage) return 0.0;
        if (stageIdx === 0) return 1.0;
        var doneInStage = 1;
        if (stageIdx === 1) doneInStage = Math.min(cfg.totalSteps, Math.max(1, root.currentWorkflowStep - cfg.startStep + 1));
        else if (stageIdx === 2) doneInStage = Math.min(cfg.totalSteps, Math.max(1, root.currentWorkflowStep - cfg.startStep + 1));
        else if (stageIdx === 3) doneInStage = Math.min(cfg.totalSteps, Math.max(1, Math.floor((root.currentWorkflowStep - cfg.startStep) / 2) + 1));
        else doneInStage = Math.min(cfg.totalSteps, Math.max(1, root.currentWorkflowStep - cfg.startStep + 1));
        return doneInStage / cfg.totalSteps;
    }

    // 柔和现代色彩
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

            // 顶部：标题与状态图例 (对标网页版)
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

                        readonly property int curStage: root.currentStageIndex()
                        readonly property bool isDone: stageCard.index < stageCard.curStage
                        readonly property bool isActive: stageCard.index === stageCard.curStage
                        readonly property bool isPending: stageCard.index > stageCard.curStage

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
                                    text: root.stageProgressText(stageCard.index)
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
                                    width: parent.width * root.stageProgressRatio(stageCard.index)
                                    color: stageCard.isDone ? root.doneGreen : root.activeBlue
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.stageSelected(stageCard.index);
                            }
                        }
                    }
                }
            }
        }
    }
}
