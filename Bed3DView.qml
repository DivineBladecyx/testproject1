import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    implicitWidth: 960
    implicitHeight: 520

    // =========================================================================
    // 1. 业务数据模型与后端联动状态（与 Vue3 tcsStore 100% 对应）
    // =========================================================================
    property real axisA: (typeof bedController !== "undefined" && bedController) ? bedController.a1 : 0.0
    property real axisB: (typeof bedController !== "undefined" && bedController) ? bedController.a2 : 0.0
    property real axisC: (typeof bedController !== "undefined" && bedController) ? bedController.a3 : 0.0
    property real axisX: (typeof bedController !== "undefined" && bedController) ? bedController.a4 : 0.0
    property real axisY: (typeof bedController !== "undefined" && bedController) ? bedController.a5 : 0.0
    property real axisZ: (typeof bedController !== "undefined" && bedController) ? bedController.a6 : 0.0
    property bool isAlarm: (typeof bedController !== "undefined" && bedController) ? bedController.alarm : false

    // 当前时序状态索引 (0 ~ 11，默认为 2 即 WAITING_OIS_SCHEDULE_CONFIRM)
    property int currentStepIndex: 2
    property bool isErrorStopped: false

    // 外部联动信号
    signal stepSelected(int stepIndex)
    signal ridgeFilterToggled(bool inPosition)
    signal drResetRequested()
    signal ddsResetRequested()

    // 12 个时序状态列表与元数据定义 (与 Vue3 FSM_ORDER 及 STATE_META 1:1 严格对齐)
    readonly property var fsmOrder: [
        'INIT',
        'WAITING_LOGIN',
        'WAITING_OIS_SCHEDULE_CONFIRM',
        'WAITING_FRONTEND_PLAN_CONFIRM',
        'WAITING_DEVICE_PLAN_ACK',
        'WAITING_DEVICE_AUTOPOS_DONE',
        'WAITING_FRONTEND_BEAM_APPLY',
        'WAITING_DEVICE_BEAM_READY',
        'TREATMENT_IN_PROGRESS',
        'TREATMENT_PAUSED',
        'WAITING_TREATMENT_END',
        'WATING_CHART_GENERATION'
    ]

    readonly property var stateMetaList: [
        { key: "INIT", num: "01", cn: "系统初始化", en: "INIT", desc: "自检与硬件握手" },
        { key: "WAITING_LOGIN", num: "02", cn: "操作员登录", en: "LOGIN", desc: "登录与绑定机房" },
        { key: "WAITING_OIS_SCHEDULE_CONFIRM", num: "03", cn: "排程叫号", en: "SCHEDULE", desc: "接收叫号指令" },
        { key: "WAITING_FRONTEND_PLAN_CONFIRM", num: "04", cn: "计划确认", en: "PLAN_CONFIRM", desc: "核对计划与参数" },
        { key: "WAITING_DEVICE_PLAN_ACK", num: "05", cn: "计划审核", en: "PLAN_ACK", desc: "审核与标定确认" },
        { key: "WAITING_DEVICE_AUTOPOS_DONE", num: "06", cn: "图像摆位", en: "AUTOPOS", desc: "曝光、配准与移床" },
        { key: "WAITING_FRONTEND_BEAM_APPLY", num: "07", cn: "申请出束", en: "BEAM_APPLY", desc: "摆位就绪，申请出束" },
        { key: "WAITING_DEVICE_BEAM_READY", num: "08", cn: "排队就绪", en: "BEAM_READY", desc: "排队准入与计划下发" },
        { key: "TREATMENT_IN_PROGRESS", num: "09", cn: "束流照射", en: "IN_PROGRESS", desc: "束流照射与剂量监测" },
        { key: "TREATMENT_PAUSED", num: "10", cn: "出束暂停", en: "PAUSED", desc: "照射暂停，保持现场" },
        { key: "WAITING_TREATMENT_END", num: "11", cn: "硬件复位", en: "TREATMENT_END", desc: "退过滤器与退床" },
        { key: "WATING_CHART_GENERATION", num: "12", cn: "报告归档", en: "CHART_GEN", desc: "生成并上报报告" }
    ]

    // 5 大宏观临床状态定义 (与 Vue3 MACRO_PHASE_DEFS 一致)
    readonly property var macroPhaseDefs: [
        { key: 'INIT', title: '初始化', sub: 'INIT', startIdx: 0, endIdx: 0 },
        { key: 'PRESET', title: '预置', sub: 'PRESET', startIdx: 1, endIdx: 4 },
        { key: 'PREPARE', title: '准备', sub: 'PREPARE', startIdx: 5, endIdx: 7 },
        { key: 'BEAM', title: '出束', sub: 'BEAM', startIdx: 8, endIdx: 9 },
        { key: 'COMPLETE', title: '结案', sub: 'COMPLETE', startIdx: 10, endIdx: 11 }
    ]

    // 当前宏观阶段计算
    function currentMacroDef() {
        var idx = root.currentStepIndex;
        for (var i = 0; i < macroPhaseDefs.length; ++i) {
            var def = macroPhaseDefs[i];
            if (idx >= def.startIdx && idx <= def.endIdx) return def;
        }
        return macroPhaseDefs[0];
    }

    // 机构模拟状态（与 store.hardware 一致）
    property bool ridgeFilterInPosition: true
    property bool drAtWorkPosition: true
    property bool ddsHasFault: false
    property bool couchConnected: true
    property bool couchSystemReady: true
    property bool couchRemoteMode: true

    // 目标基准与配准坐标（与 store.targetCouch 一致）
    property var couchData: [
        { name: "X", actual: (root.axisX !== 0 ? root.axisX.toFixed(2) : "2149.76"), unit: "mm", target: "2149.76", base: "2150.0", shift: "-0.24" },
        { name: "Y", actual: (root.axisY !== 0 ? root.axisY.toFixed(2) : "158.49"),  unit: "mm", target: "158.49",  base: "158.0",  shift: "+0.49" },
        { name: "Z", actual: (root.axisZ !== 0 ? root.axisZ.toFixed(2) : "1433.03"), unit: "mm", target: "1433.03", base: "1433.0", shift: "+0.03" },
        { name: "Yaw", actual: (root.axisA !== 0 ? root.axisA.toFixed(2) : "0.42"),  unit: "°", target: "0.42",   base: "0.40",   shift: "+0.02" },
        { name: "Pitch", actual: (root.axisB !== 0 ? root.axisB.toFixed(2) : "0.15"), unit: "°", target: "0.15",   base: "0.10",   shift: "+0.05" },
        { name: "Roll", actual: (root.axisC !== 0 ? root.axisC.toFixed(2) : "0.02"), unit: "°", target: "0.02",   base: "0.00",   shift: "+0.02" }
    ]

    // 网页版标准调色盘 (Slate & Tech Blue)
    readonly property color bgSlate: "#F1F5F9"
    readonly property color cardBg: "#FFFFFF"
    readonly property color cardBorder: "#E2E8F0"
    readonly property color textMain: "#0F172A"
    readonly property color textMuted: "#64748B"
    readonly property color accentBlue: "#2563EB"
    readonly property color accentLightBlue: "#EFF6FF"
    readonly property color accentGreen: "#10B981"
    readonly property color accentLightGreen: "#ECFDF5"

    Rectangle {
        anchors.fill: parent
        color: root.bgSlate
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // =====================================================================
        // 左翼面板 (flank-left)：六自由度双列对比表 + 床控制模式
        // =====================================================================
        Rectangle {
            Layout.preferredWidth: Math.min(235, Math.max(180, root.width * 0.23))
            Layout.fillHeight: true
            radius: 8
            color: root.cardBg
            border.color: root.cardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // 标题栏与胶囊状态 (与 Vue3 一模一样)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "床坐标 (六自由度)"
                        font.pixelSize: 12
                        font.bold: true
                        color: root.textMain
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        implicitWidth: 38
                        implicitHeight: 18
                        radius: 3
                        color: root.couchConnected ? root.accentLightGreen : "#F1F5F9"
                        border.color: root.couchConnected ? "#A7F3D0" : "#E2E8F0"
                        Text {
                            anchors.centerIn: parent
                            text: root.couchConnected ? "已连接" : "待机"
                            font.pixelSize: 9
                            font.bold: true
                            color: root.couchConnected ? "#059669" : root.textMuted
                        }
                    }

                    Rectangle {
                        implicitWidth: 30
                        implicitHeight: 18
                        radius: 3
                        color: root.couchSystemReady ? root.accentLightGreen : "#F1F5F9"
                        border.color: root.couchSystemReady ? "#A7F3D0" : "#E2E8F0"
                        Text {
                            anchors.centerIn: parent
                            text: root.couchSystemReady ? "就绪" : "待机"
                            font.pixelSize: 9
                            font.bold: true
                            color: root.couchSystemReady ? "#059669" : root.textMuted
                        }
                    }
                }

                // 双列对比表头
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 22
                    radius: 3
                    color: "#F8FAFC"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        Text { text: "轴"; font.pixelSize: 10; font.bold: true; color: root.textMuted; Layout.preferredWidth: 28 }
                        Text { text: "实际位置"; font.pixelSize: 10; font.bold: true; color: root.textMuted; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                        Text { text: "目标 (基准+配准)"; font.pixelSize: 10; font.bold: true; color: root.textMuted; Layout.preferredWidth: 85; horizontalAlignment: Text.AlignRight }
                    }
                }

                // 六轴实际 vs 目标(基准+配准)
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 3
                    interactive: false

                    model: root.couchData

                    delegate: Rectangle {
                        width: ListView.view.width
                        implicitHeight: 28
                        radius: 3
                        color: index % 2 === 0 ? "#F8FAFC" : "#FFFFFF"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6

                            // 轴徽章
                            Rectangle {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 18
                                radius: 3
                                color: "#EFF6FF"
                                border.color: "#BFDBFE"
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.name
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: root.accentBlue
                                }
                            }

                            // 实际位置
                            Text {
                                text: modelData.actual + modelData.unit
                                font.pixelSize: 11
                                font.bold: true
                                font.family: "Menlo"
                                color: root.textMain
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignRight
                            }

                            // 目标值 + (基准+配准细分)
                            ColumnLayout {
                                Layout.preferredWidth: 85
                                spacing: 1
                                Text {
                                    text: modelData.target
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.family: "Menlo"
                                    color: root.accentBlue
                                    Layout.alignment: Qt.AlignRight
                                }
                                Text {
                                    text: "(" + modelData.base + " " + modelData.shift + ")"
                                    font.pixelSize: 8
                                    font.family: "Menlo"
                                    color: root.textMuted
                                    Layout.alignment: Qt.AlignRight
                                }
                            }
                        }
                    }
                }

                // 底部床控制卡片 (与 Vue3 一致)
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 34
                    radius: 4
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        Text {
                            text: "床控制"
                            font.pixelSize: 11
                            font.bold: true
                            color: root.textMuted
                        }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            implicitWidth: 36
                            implicitHeight: 20
                            radius: 3
                            color: root.couchRemoteMode ? root.accentLightGreen : "#F1F5F9"
                            border.color: root.couchRemoteMode ? "#A7F3D0" : "#E2E8F0"
                            Text {
                                anchors.centerIn: parent
                                text: root.couchRemoteMode ? "远控" : "就地"
                                font.pixelSize: 9
                                font.bold: true
                                color: root.couchRemoteMode ? "#059669" : root.textMuted
                            }
                        }
                        Rectangle {
                            implicitWidth: 46
                            implicitHeight: 22
                            radius: 3
                            color: root.accentLightGreen
                            border.color: "#A7F3D0"
                            Text {
                                anchors.centerIn: parent
                                text: "就绪"
                                font.pixelSize: 10
                                font.bold: true
                                color: root.accentGreen
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: parent.color = "#D1FAE5"
                            }
                        }
                    }
                }
            }
        }

        // =====================================================================
        // 中央核心舞台 (workflow-ring-container)
        // =====================================================================
        Rectangle {
            id: orbitContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: root.cardBg
            border.color: root.cardBorder
            border.width: 1
            clip: true

            readonly property real cx: width / 2
            readonly property real cy: height / 2
            // 依据容器尺寸弹性自适应基准半径
            readonly property real scaleFactor: Math.min(width / 480, height / 480)
            readonly property real baseRadius: 182 * scaleFactor
            readonly property real zoneRadius: 212 * scaleFactor

            // 1. 底层 Canvas 绘制外层 5 大宏观分区弧线与主进度弧
            Canvas {
                id: ringCanvas
                anchors.fill: parent

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();

                    var cx = orbitContainer.cx;
                    var cy = orbitContainer.cy;
                    var rMain = orbitContainer.baseRadius;
                    var rZone = orbitContainer.zoneRadius;

                    // 1.1 外层宏观阶段底轨 (r=212)
                    ctx.beginPath();
                    ctx.arc(cx, cy, rZone, 0, Math.PI * 2, false);
                    ctx.strokeStyle = "#F1F5F9";
                    ctx.lineWidth = 3;
                    ctx.stroke();

                    // 1.2 当前宏观阶段的高亮外弧
                    var curMacro = root.currentMacroDef();
                    var startAngle = ((curMacro.startIdx * 30 - 90) * Math.PI) / 180;
                    var endAngle = (((curMacro.endIdx + 1) * 30 - 90) * Math.PI) / 180;
                    ctx.beginPath();
                    ctx.arc(cx, cy, rZone, startAngle, endAngle, false);
                    ctx.strokeStyle = "#3B82F6";
                    ctx.lineWidth = 3.5;
                    ctx.stroke();

                    // 1.3 主进度底轨 (r=182 / 175)
                    ctx.beginPath();
                    ctx.arc(cx, cy, rMain, 0, Math.PI * 2, false);
                    ctx.strokeStyle = "#E2E8F0";
                    ctx.lineWidth = 2.5;
                    ctx.stroke();

                    // 1.4 高亮进度流光弧 (从 step 0 顺时针延伸至当前 step)
                    var arcStart = -Math.PI / 2; // 01 在 12点钟 (-90度)
                    var arcCurrent = arcStart + ((root.currentStepIndex + 0.3) / 12) * Math.PI * 2;
                    ctx.beginPath();
                    ctx.arc(cx, cy, rMain, arcStart, arcCurrent, false);
                    ctx.strokeStyle = "#2563EB";
                    ctx.lineWidth = 4;
                    ctx.lineCap = "round";
                    ctx.stroke();
                }

                Connections {
                    target: root
                    function onCurrentStepIndexChanged() { ringCanvas.requestPaint(); }
                }
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
            }

            // 2. 12 个时序节点围绕 (cx, cy, baseRadius) 顺时针均匀分布
            Repeater {
                model: root.stateMetaList

                Item {
                    id: nodeDelegate
                    required property int index
                    required property var modelData

                    // 角度换算：01 处于 12点钟正上方 (-90 deg)，30 度等分
                    readonly property real angleRad: ((nodeDelegate.index * 30 - 90) * Math.PI) / 180
                    readonly property real posX: orbitContainer.cx + orbitContainer.baseRadius * Math.cos(angleRad)
                    readonly property real posY: orbitContainer.cy + orbitContainer.baseRadius * Math.sin(angleRad)

                    readonly property bool isDone: nodeDelegate.index < root.currentStepIndex
                    readonly property bool isCurrent: nodeDelegate.index === root.currentStepIndex

                    x: posX - width / 2
                    y: posY - height / 2
                    width: nodePill.width
                    height: nodePill.height

                    Rectangle {
                        id: nodePill
                        implicitHeight: Math.max(22, Math.min(26, orbitContainer.scaleFactor * 26))
                        implicitWidth: nodeLayout.width + 12
                        radius: height / 2
                        color: nodeDelegate.isDone ? "#FFFFFF" :
                               nodeDelegate.isCurrent ? "#2563EB" : "#F8FAFC"
                        border.color: nodeDelegate.isDone ? "#10B981" :
                                      nodeDelegate.isCurrent ? "#1D4ED8" : "#CBD5E1"
                        border.width: nodeDelegate.isCurrent ? 2 : 1

                        // 当前活跃节点的外圈脉冲光晕
                        Rectangle {
                            visible: nodeDelegate.isCurrent
                            anchors.fill: parent
                            anchors.margins: -4
                            radius: parent.radius + 4
                            color: "transparent"
                            border.color: "#93C5FD"
                            border.width: 2
                            opacity: 0.8
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                running: nodeDelegate.isCurrent
                                NumberAnimation { to: 0.2; duration: 900; easing.type: Easing.InOutQuad }
                                NumberAnimation { to: 0.9; duration: 900; easing.type: Easing.InOutQuad }
                            }
                        }

                        Row {
                            id: nodeLayout
                            anchors.centerIn: parent
                            spacing: 4

                            // 序号小圆标
                            Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                anchors.verticalCenter: parent.verticalCenter
                                color: nodeDelegate.isDone ? "#10B981" :
                                       nodeDelegate.isCurrent ? "#FFFFFF" : "#CBD5E1"
                                Text {
                                    anchors.centerIn: parent
                                    text: nodeDelegate.isDone ? "✓" : nodeDelegate.modelData.num
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: nodeDelegate.isDone ? "#FFFFFF" :
                                           nodeDelegate.isCurrent ? "#2563EB" : "#475569"
                                }
                            }

                            // 状态中文名
                            Text {
                                text: nodeDelegate.modelData.cn
                                font.pixelSize: 10
                                font.bold: nodeDelegate.isCurrent
                                color: nodeDelegate.isCurrent ? "#FFFFFF" :
                                       nodeDelegate.isDone ? "#059669" : "#334155"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.currentStepIndex = nodeDelegate.index;
                                root.stepSelected(nodeDelegate.index);
                            }
                        }
                    }
                }
            }

            // 3. 环心核心仪表盘 (ring-center-hub)
            Rectangle {
                id: hub
                width: Math.min(190, Math.max(110, orbitContainer.baseRadius * 1.15))
                height: width
                radius: width / 2
                anchors.centerIn: parent
                color: "#FFFFFF"
                border.color: "#E2E8F0"
                border.width: 1

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 6
                    radius: width / 2
                    color: "transparent"
                    border.color: "#F1F5F9"
                    border.width: 1
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 3

                    // 宏观阶段徽章
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        implicitHeight: 18
                        implicitWidth: hubMacroText.width + 12
                        radius: 9
                        color: root.accentLightBlue
                        Text {
                            id: hubMacroText
                            anchors.centerIn: parent
                            text: root.currentMacroDef().title + " · " + root.currentMacroDef().sub
                            font.pixelSize: 9
                            font.bold: true
                            color: root.accentBlue
                        }
                    }

                    // STEP 序号与完成度百分比
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "STEP " + root.stateMetaList[root.currentStepIndex].num + " / 12 (" + Math.round(((root.currentStepIndex + 1) / 12) * 100) + "%)"
                        font.pixelSize: 10
                        font.bold: true
                        font.family: "Menlo"
                        color: root.textMuted
                    }

                    // 核心当前中文状态名 (粗体大字)
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.stateMetaList[root.currentStepIndex].cn
                        font.pixelSize: Math.max(14, Math.min(20, hub.width * 0.13))
                        font.bold: true
                        color: root.textMain
                    }

                    // 英文 FSM 状态代码药丸
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        implicitHeight: 18
                        implicitWidth: hubCodeText.width + 10
                        radius: 3
                        color: "#F1F5F9"
                        border.color: "#CBD5E1"
                        Text {
                            id: hubCodeText
                            anchors.centerIn: parent
                            text: root.stateMetaList[root.currentStepIndex].key
                            font.pixelSize: 8
                            font.bold: true
                            font.family: "Menlo"
                            color: root.accentBlue
                        }
                    }

                    // 辅助状态描述
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.stateMetaList[root.currentStepIndex].desc
                        font.pixelSize: 9
                        color: root.textMuted
                    }
                }
            }
        }

        // =====================================================================
        // 右翼面板 (flank-right)：脊形过滤器 + DR + DDS + 13 联锁停机
        // =====================================================================
        Rectangle {
            Layout.preferredWidth: Math.min(170, Math.max(130, root.width * 0.17))
            Layout.fillHeight: true
            radius: 8
            color: root.cardBg
            border.color: root.cardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // 1. 脊形过滤器 (Ridge Filter) 移入/移出
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 68
                    radius: 4
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "脊形过滤器"; font.pixelSize: 11; font.bold: true; color: root.textMuted }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                implicitWidth: 32
                                implicitHeight: 16
                                radius: 3
                                color: root.ridgeFilterInPosition ? root.accentLightGreen : "#F1F5F9"
                                border.color: root.ridgeFilterInPosition ? "#A7F3D0" : "#E2E8F0"
                                Text {
                                    anchors.centerIn: parent
                                    text: root.ridgeFilterInPosition ? "到位" : "移出"
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: root.ridgeFilterInPosition ? root.accentGreen : root.textMuted
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 24
                            radius: 3
                            color: "#FFFFFF"
                            border.color: "#CBD5E1"
                            Text {
                                anchors.centerIn: parent
                                text: root.ridgeFilterInPosition ? "✦ 移出" : "✦ 移入"
                                font.pixelSize: 10
                                font.bold: true
                                color: root.textMain
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.ridgeFilterInPosition = !root.ridgeFilterInPosition;
                                    root.ridgeFilterToggled(root.ridgeFilterInPosition);
                                }
                            }
                        }
                    }
                }

                // 2. DR 状态
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 68
                    radius: 4
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "DR状态"; font.pixelSize: 11; font.bold: true; color: root.textMuted }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                implicitWidth: 32
                                implicitHeight: 16
                                radius: 3
                                color: root.drAtWorkPosition ? root.accentLightGreen : "#F1F5F9"
                                border.color: root.drAtWorkPosition ? "#A7F3D0" : "#E2E8F0"
                                Text {
                                    anchors.centerIn: parent
                                    text: root.drAtWorkPosition ? "就绪" : "待位"
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: root.drAtWorkPosition ? root.accentGreen : root.textMuted
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 24
                            radius: 3
                            color: root.accentLightBlue
                            border.color: "#BFDBFE"
                            Text {
                                anchors.centerIn: parent
                                text: "✦ 复位到位"
                                font.pixelSize: 10
                                font.bold: true
                                color: root.accentBlue
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    parent.color = "#DBEAFE";
                                    root.drResetRequested();
                                }
                            }
                        }
                    }
                }

                // 3. DDS 状态与故障复位
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 74
                    radius: 4
                    color: root.ddsHasFault ? "#FFF1F2" : "#F8FAFC"
                    border.color: root.ddsHasFault ? "#FECDD3" : "#E2E8F0"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "DDS状态"; font.pixelSize: 11; font.bold: true; color: root.textMuted }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                implicitWidth: 32
                                implicitHeight: 16
                                radius: 3
                                color: root.ddsHasFault ? "#FFE4E6" : root.accentLightGreen
                                border.color: root.ddsHasFault ? "#FDA4AF" : "#A7F3D0"
                                Text {
                                    anchors.centerIn: parent
                                    text: root.ddsHasFault ? "故障" : "正常"
                                    font.pixelSize: 8
                                    font.bold: true
                                    color: root.ddsHasFault ? "#DC2626" : root.accentGreen
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 24
                            radius: 3
                            color: "#FFFFFF"
                            border.color: root.ddsHasFault ? "#F87171" : "#CBD5E1"
                            Text {
                                anchors.centerIn: parent
                                text: "DDS故障复位"
                                font.pixelSize: 10
                                font.bold: true
                                color: root.ddsHasFault ? "#DC2626" : root.textMain
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.ddsHasFault = false;
                                    root.ddsResetRequested();
                                }
                            }
                        }
                    }
                }

                // 4. 13 联锁保护停机状态指示 (与 Vue3 一模一样)
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 46
                    radius: 4
                    color: root.isErrorStopped ? "#FFF1F2" : "#F8FAFC"
                    border.color: root.isErrorStopped ? "#FDA4AF" : "#E2E8F0"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 6

                        Rectangle {
                            width: 18
                            height: 18
                            radius: 9
                            color: root.isErrorStopped ? "#EF4444" : "#10B981"
                            Text {
                                anchors.centerIn: parent
                                text: "13"
                                font.pixelSize: 8
                                font.bold: true
                                color: "#FFFFFF"
                            }
                        }

                        ColumnLayout {
                            spacing: 1
                            Text {
                                text: "联锁停机"
                                font.pixelSize: 11
                                font.bold: true
                                color: root.isErrorStopped ? "#DC2626" : root.textMain
                            }
                            Text {
                                text: root.isErrorStopped ? "ERROR_STOPPED" : "NORMAL"
                                font.pixelSize: 8
                                font.family: "Menlo"
                                color: root.textMuted
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
