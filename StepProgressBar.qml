pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

Item {
    id: root

    // 当前具体的 12 步时序索引 (0 ~ 11，默认为 2 即 WAITING_OIS_SCHEDULE_CONFIRM)
    property int currentWorkflowStep: 2
    property bool isErrorStopped: false

    // 点击某宏观阶段向外发射信号
    signal stageSelected(int stageIndex)

    // 5 大宏观临床状态定义 (与 Vue3 MACRO_PHASE_DEFS 1:1 严格对齐)
    readonly property var macroPhases: [
        { key: 'INIT',     title: '初始化', sub: 'INIT',     totalSteps: 1, startIdx: 0,  endIdx: 0 },
        { key: 'PRESET',   title: '预置',   sub: 'PRESET',   totalSteps: 4, startIdx: 1,  endIdx: 4 },
        { key: 'PREPARE',  title: '准备',   sub: 'PREPARE',  totalSteps: 3, startIdx: 5,  endIdx: 7 },
        { key: 'BEAM',     title: '出束',   sub: 'BEAM',     totalSteps: 2, startIdx: 8,  endIdx: 9 },
        { key: 'COMPLETE', title: '结案',   sub: 'COMPLETE', totalSteps: 2, startIdx: 10, endIdx: 11 }
    ]

    // 计算当前子状态归属的主阶段索引 (0 ~ 4)
    function currentStageIndex() {
        var s = root.currentWorkflowStep;
        for (var i = 0; i < macroPhases.length; ++i) {
            var p = macroPhases[i];
            if (s >= p.startIdx && s <= p.endIdx) return i;
        }
        return 0;
    }

    // 动态计算每个阶段的状态 ('completed' | 'active' | 'pending')
    function phaseStatus(stageIdx) {
        if (root.isErrorStopped) return 'pending';
        var curStage = currentStageIndex();
        if (stageIdx < curStage) return 'completed';
        if (stageIdx === curStage) return 'active';
        return 'pending';
    }

    // 动态计算阶段内部比率文本 (与 Vue3 ratioText 1:1 一致，如 1/1, 2/4, 0/3)
    function phaseRatioText(stageIdx) {
        var status = phaseStatus(stageIdx);
        var p = macroPhases[stageIdx];
        if (status === 'completed') {
            return p.totalSteps + "/" + p.totalSteps;
        } else if (status === 'active') {
            var currentStepInPhase = root.currentWorkflowStep - p.startIdx + 1;
            return currentStepInPhase + "/" + p.totalSteps;
        } else {
            return "0/" + p.totalSteps;
        }
    }

    // 动态计算微进度条百分比 (0.0 ~ 1.0)
    function phaseFillRatio(stageIdx) {
        var status = phaseStatus(stageIdx);
        var p = macroPhases[stageIdx];
        if (status === 'completed') return 1.0;
        if (status === 'active') {
            var currentStepInPhase = root.currentWorkflowStep - p.startIdx + 1;
            return Math.min(1.0, currentStepInPhase / p.totalSteps);
        }
        return 0.0;
    }

    // 柔和现代色彩配置
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

            // 顶部：标题与状态图例 (与 Vue3 HeaderBar 1:1 一致)
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

            // 下部：5 大宏观阶段卡片 (Macro Phase Step Bar)
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                Repeater {
                    model: root.macroPhases.length

                    Rectangle {
                        id: phaseStepItem
                        required property int index
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 6

                        readonly property string st: root.phaseStatus(phaseStepItem.index)
                        readonly property bool isCompleted: st === 'completed'
                        readonly property bool isActive: st === 'active'

                        color: isActive ? "#EFF6FF" :
                               isCompleted ? "#F0FDF4" : "#F8FAFC"
                        border.color: isActive ? "#3B82F6" :
                                      isCompleted ? "#86EFAC" : "#E2E8F0"
                        border.width: isActive ? 1.5 : 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true

                                // 序号与阶段名
                                Row {
                                    spacing: 4
                                    Text {
                                        text: "0" + (phaseStepItem.index + 1)
                                        font.pixelSize: 10
                                        font.family: "Menlo"
                                        font.bold: true
                                        color: phaseStepItem.isActive ? root.activeBlue :
                                               phaseStepItem.isCompleted ? "#15803D" : root.textMuted
                                    }
                                    Text {
                                        text: root.macroPhases[phaseStepItem.index].title
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: phaseStepItem.isActive ? root.activeBlue :
                                               phaseStepItem.isCompleted ? "#15803D" : root.textPrimary
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                // 状态圆标 + 比率文字
                                Row {
                                    spacing: 4
                                    Text {
                                        text: phaseStepItem.isCompleted ? "✓" : (phaseStepItem.isActive ? "●" : "○")
                                        font.pixelSize: 9
                                        color: phaseStepItem.isCompleted ? "#16A34A" :
                                               phaseStepItem.isActive ? root.activeBlue : "#94A3B8"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: root.phaseRatioText(phaseStepItem.index)
                                        font.pixelSize: 10
                                        font.family: "Menlo"
                                        color: phaseStepItem.isActive ? root.activeBlue :
                                               phaseStepItem.isCompleted ? "#16A34A" : "#94A3B8"
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }

                            // 底部微进度管 (phase-micro-progress-track)
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
                                    width: parent.width * root.phaseFillRatio(phaseStepItem.index)
                                    color: phaseStepItem.isCompleted ? root.doneGreen : root.activeBlue
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.stageSelected(phaseStepItem.index);
                            }
                        }
                    }
                }
            }
        }
    }
}
