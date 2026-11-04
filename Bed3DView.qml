import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: root
    implicitWidth: 960
    implicitHeight: 520

    // ==========================================
    // 1. 数据绑定与平滑过渡
    // ==========================================
    property real axisA: (typeof bedController !== "undefined" && bedController) ? bedController.a1 : 0.0
    property real axisB: (typeof bedController !== "undefined" && bedController) ? bedController.a2 : 0.0
    property real axisC: (typeof bedController !== "undefined" && bedController) ? bedController.a3 : 0.0
    property real axisX: (typeof bedController !== "undefined" && bedController) ? bedController.a4 : 0.0
    property real axisY: (typeof bedController !== "undefined" && bedController) ? bedController.a5 : 0.0
    property real axisZ: (typeof bedController !== "undefined" && bedController) ? bedController.a6 : 0.0
    property bool isAlarm: (typeof bedController !== "undefined" && bedController) ? bedController.alarm : false

    // 当前所处 12 步状态机索引（0 ~ 11，默认排程叫号为 2，即 STEP 03）
    property int currentStepIndex: 2

    // 12 状态机定义（与网页版碳离子系统 100% 对应）
    readonly property var workflowSteps: [
        { num: "01", name: "系统初始化", code: "INIT", desc: "环境及通信检查" },
        { num: "02", name: "操作员登录", code: "LOGIN", desc: "身份验证通过" },
        { num: "03", name: "排程叫号", code: "SCHEDULE", desc: "等待叫号指令" },
        { num: "04", name: "计划确认", code: "PLAN_ACK", desc: "核对靶区与剂量" },
        { num: "05", name: "计划审核", code: "PLAN_VERIFY", desc: "物理师及医生审批" },
        { num: "06", name: "治疗摆位", code: "SETUP", desc: "六自由度床位对位" },
        { num: "07", name: "申请出束", code: "BEAM_APPLY", desc: "请求主控照射许可" },
        { num: "08", name: "排队就绪", code: "QUEUE_READY", desc: "束流准备就绪" },
        { num: "09", name: "束流照射", code: "IRRADIATION", desc: "正在出束治疗" },
        { num: "10", name: "出束暂停", code: "PAUSED", desc: "照射暂停保压" },
        { num: "11", name: "操作复位", code: "RESET", desc: "设备与床位归位" },
        { num: "12", name: "综合日志", code: "LOGGING", desc: "保存本次执行记录" }
    ]

    // 柔和配色方案（与网页版一致）
    readonly property color bgSlate: "#F8FAFC"
    readonly property color cardBg: "#FFFFFF"
    readonly property color cardBorder: "#E2E8F0"
    readonly property color textMain: "#0F172A"
    readonly property color textMuted: "#64748B"
    readonly property color accentBlue: "#2563EB"
    readonly property color accentLightBlue: "#EFF6FF"
    readonly property color accentGreen: "#059669"
    readonly property color accentLightGreen: "#ECFDF5"

    // 外层容器底色
    Rectangle {
        anchors.fill: parent
        color: root.bgSlate
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // =====================================================================
        // 左侧卡片：床坐标 (六自由度)
        // =====================================================================
        Rectangle {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            radius: 8
            color: root.cardBg
            border.color: root.cardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // 标题栏与胶囊状态
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: "床坐标（六自由度）"
                        font.pixelSize: 13
                        font.bold: true
                        color: root.textMain
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        implicitWidth: 42
                        implicitHeight: 20
                        radius: 4
                        color: root.accentLightBlue
                        border.color: "#BFDBFE"
                        Text {
                            anchors.centerIn: parent
                            text: "已就绪"
                            font.pixelSize: 10
                            font.bold: true
                            color: root.accentBlue
                        }
                    }

                    Rectangle {
                        implicitWidth: 32
                        implicitHeight: 20
                        radius: 4
                        color: root.accentLightGreen
                        border.color: "#A7F3D0"
                        Text {
                            anchors.centerIn: parent
                            text: "正常"
                            font.pixelSize: 10
                            font.bold: true
                            color: root.accentGreen
                        }
                    }
                }

                // 表头
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 24
                    radius: 4
                    color: "#F1F5F9"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        Text { text: "轴"; font.pixelSize: 11; font.bold: true; color: root.textMuted; Layout.preferredWidth: 32 }
                        Text { text: "实际位置"; font.pixelSize: 11; font.bold: true; color: root.textMuted; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
                        Text { text: "目标(偏差)"; font.pixelSize: 11; font.bold: true; color: root.textMuted; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight }
                    }
                }

                // 六轴数据列表
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    interactive: false

                    model: [
                        { name: "X",  val: (root.axisX !== 0 ? root.axisX.toFixed(2) : "2149.76"), unit: " mm", target: "2149.76" },
                        { name: "Y",  val: (root.axisY !== 0 ? root.axisY.toFixed(2) : "158.49"),  unit: " mm", target: "158.49" },
                        { name: "Z",  val: (root.axisZ !== 0 ? root.axisZ.toFixed(2) : "1433.03"), unit: " mm", target: "1433.03" },
                        { name: "Yaw",   val: (root.axisA !== 0 ? root.axisA.toFixed(2) : "0.42"),    unit: "°", target: "0.42" },
                        { name: "Pitch", val: (root.axisB !== 0 ? root.axisB.toFixed(2) : "0.15"),    unit: "°", target: "0.15" },
                        { name: "Roll",  val: (root.axisC !== 0 ? root.axisC.toFixed(2) : "0.02"),    unit: "°", target: "0.02" }
                    ]

                    delegate: Rectangle {
                        width: ListView.view.width
                        implicitHeight: 28
                        radius: 4
                        color: index % 2 === 0 ? "#FAFAFA" : "#FFFFFF"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8

                            Text {
                                text: modelData.name
                                font.pixelSize: 12
                                font.bold: true
                                color: root.accentBlue
                                Layout.preferredWidth: 32
                            }

                            Text {
                                text: modelData.val + modelData.unit
                                font.pixelSize: 12
                                font.bold: true
                                font.family: "Menlo"
                                color: root.textMain
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignRight
                            }

                            Row {
                                Layout.preferredWidth: 70
                                Layout.alignment: Qt.AlignRight
                                spacing: 4

                                Text {
                                    text: modelData.target
                                    font.pixelSize: 11
                                    font.family: "Menlo"
                                    color: root.textMuted
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Rectangle {
                                    width: 6
                                    height: 6
                                    radius: 3
                                    color: root.accentGreen
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }

                // 底部床控制操作区
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: "床控制"
                        font.pixelSize: 12
                        font.bold: true
                        color: root.textMuted
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        implicitWidth: 64
                        implicitHeight: 26
                        radius: 4
                        color: root.accentLightGreen
                        border.color: "#A7F3D0"

                        Text {
                            anchors.centerIn: parent
                            text: "就绪"
                            font.pixelSize: 12
                            font.bold: true
                            color: root.accentGreen
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                parent.color = "#D1FAE5";
                            }
                        }
                    }
                }
            }
        }

        // =====================================================================
        // 中央核心：12 步环形状态机雷达圆环（The 12-State Circular Orbit）
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

            readonly property real centerX: width / 2
            readonly property real centerY: height / 2
            readonly property real orbitRadius: Math.max(120, Math.min(width, height) * 0.38)

            // Canvas 绘制平滑的底层轨道与高亮流光弧
            Canvas {
                id: orbitCanvas
                anchors.fill: parent

                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();

                    var cx = orbitContainer.centerX;
                    var cy = orbitContainer.centerY;
                    var r = orbitContainer.orbitRadius;

                    // 1. 底层完整静态浅灰轨道环
                    ctx.beginPath();
                    ctx.arc(cx, cy, r, 0, Math.PI * 2, false);
                    ctx.strokeStyle = "#E2E8F0";
                    ctx.lineWidth = 2.5;
                    ctx.stroke();

                    // 2. 当前进度的高亮蓝色弧线 (从第一步 01 延伸到当前步)
                    var startAngle = -Math.PI / 3; // 01 在 1点钟方向 (-60 deg)
                    var currentStepAngle = startAngle + (root.currentStepIndex / 12) * Math.PI * 2;

                    ctx.beginPath();
                    ctx.arc(cx, cy, r, startAngle, currentStepAngle, false);
                    ctx.strokeStyle = "#2563EB";
                    ctx.lineWidth = 4.0;
                    ctx.lineCap = "round";
                    ctx.stroke();

                    // 3. 装饰性内外同心细环
                    ctx.beginPath();
                    ctx.arc(cx, cy, r - 26, 0, Math.PI * 2, false);
                    ctx.strokeStyle = "#F1F5F9";
                    ctx.lineWidth = 1;
                    ctx.stroke();

                    ctx.beginPath();
                    ctx.arc(cx, cy, r + 26, 0, Math.PI * 2, false);
                    ctx.strokeStyle = "#F8FAFC";
                    ctx.lineWidth = 1;
                    ctx.stroke();
                }

                Connections {
                    target: root
                    function onCurrentStepIndexChanged() { orbitCanvas.requestPaint(); }
                }

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
            }

            // 12 个沿圆周分布的精致胶囊节点
            Repeater {
                model: root.workflowSteps

                Item {
                    id: nodeItem
                    required property int index
                    required property var modelData

                    // 角度换算：01 处于 1点钟位置 (-60 deg)，30 度步进
                    readonly property real angleRad: ((nodeItem.index * 30 - 60) * Math.PI) / 180
                    readonly property real posX: orbitContainer.centerX + orbitContainer.orbitRadius * Math.cos(angleRad)
                    readonly property real posY: orbitContainer.centerY + orbitContainer.orbitRadius * Math.sin(angleRad)

                    readonly property bool isDone: nodeItem.index < root.currentStepIndex
                    readonly property bool isCurrent: nodeItem.index === root.currentStepIndex
                    readonly property bool isFuture: nodeItem.index > root.currentStepIndex

                    x: posX - width / 2
                    y: posY - height / 2
                    width: nodeCard.width
                    height: nodeCard.height

                    Rectangle {
                        id: nodeCard
                        implicitHeight: 28
                        implicitWidth: nodeContent.width + 16
                        radius: 14
                        color: nodeItem.isDone ? "#FFFFFF" :
                               nodeItem.isCurrent ? "#2563EB" : "#F8FAFC"
                        border.color: nodeItem.isDone ? "#10B981" :
                                      nodeItem.isCurrent ? "#1D4ED8" : "#E2E8F0"
                        border.width: nodeItem.isCurrent ? 2 : 1

                        // 当前活跃状态的呼吸光晕外环
                        Rectangle {
                            visible: nodeItem.isCurrent
                            anchors.fill: parent
                            anchors.margins: -4
                            radius: parent.radius + 4
                            color: "transparent"
                            border.color: "#93C5FD"
                            border.width: 2
                            opacity: 0.8

                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                running: nodeItem.isCurrent
                                NumberAnimation { to: 0.2; duration: 900; easing.type: Easing.InOutQuad }
                                NumberAnimation { to: 0.9; duration: 900; easing.type: Easing.InOutQuad }
                            }
                        }

                        Row {
                            id: nodeContent
                            anchors.centerIn: parent
                            spacing: 5

                            // 状态小圆标
                            Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                anchors.verticalCenter: parent.verticalCenter
                                color: nodeItem.isDone ? "#10B981" :
                                       nodeItem.isCurrent ? "#FFFFFF" : "#CBD5E1"

                                Text {
                                    anchors.centerIn: parent
                                    text: nodeItem.isDone ? "✓" : (nodeItem.index + 1)
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: nodeItem.isDone ? "#FFFFFF" :
                                           nodeItem.isCurrent ? "#2563EB" : "#64748B"
                                }
                            }

                            // 步骤中文名
                            Text {
                                text: nodeItem.modelData.name
                                font.pixelSize: 11
                                font.bold: nodeItem.isCurrent
                                color: nodeItem.isCurrent ? "#FFFFFF" :
                                       nodeItem.isDone ? "#059669" : "#475569"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.currentStepIndex = nodeItem.index;
                            }
                        }
                    }
                }
            }

            // =================================================================
            // 环心多层控制仪表盘（Center Control Core）
            // =================================================================
            Rectangle {
                id: centerCore
                width: Math.min(220, orbitContainer.orbitRadius * 1.25)
                height: width
                radius: width / 2
                anchors.centerIn: parent
                color: "#FFFFFF"
                border.color: "#E2E8F0"
                border.width: 1

                // 柔和外圈装饰微环
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
                    spacing: 4

                    // 顶部类别标签
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        implicitHeight: 18
                        implicitWidth: presetText.width + 12
                        radius: 9
                        color: root.accentLightBlue

                        Text {
                            id: presetText
                            anchors.centerIn: parent
                            text: "预置 · PRESET"
                            font.pixelSize: 9
                            font.bold: true
                            color: root.accentBlue
                        }
                    }

                    // 步骤统计
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "STEP " + root.workflowSteps[root.currentStepIndex].num + " / 12 (" + Math.round(((root.currentStepIndex + 1) / 12) * 100) + "%)"
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "Menlo"
                        color: root.textMuted
                    }

                    // 核心当前状态名称（粗体大字）
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.workflowSteps[root.currentStepIndex].name
                        font.pixelSize: 22
                        font.bold: true
                        color: root.textMain
                    }

                    // 状态代码药丸
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        implicitHeight: 20
                        implicitWidth: codeText.width + 12
                        radius: 4
                        color: "#F1F5F9"
                        border.color: "#CBD5E1"
                        border.width: 1

                        Text {
                            id: codeText
                            anchors.centerIn: parent
                            text: "WAITING_OIS_" + root.workflowSteps[root.currentStepIndex].code + "_CONFIRM"
                            font.pixelSize: 9
                            font.bold: true
                            font.family: "Menlo"
                            color: root.accentBlue
                        }
                    }

                    // 辅助提示说明
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: root.workflowSteps[root.currentStepIndex].desc
                        font.pixelSize: 10
                        color: root.textMuted
                    }
                }
            }
        }

        // =====================================================================
        // 右侧微控制面板：硬件联动微卡片
        // =====================================================================
        Rectangle {
            Layout.preferredWidth: 170
            Layout.fillHeight: true
            radius: 8
            color: root.cardBg
            border.color: root.cardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                // 卡片 1: 楔形过滤器
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 74
                    radius: 6
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Text {
                            text: "楔形过滤器"
                            font.pixelSize: 11
                            font.bold: true
                            color: root.textMuted
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 28
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#CBD5E1"

                            Text {
                                anchors.centerIn: parent
                                text: "✦ 移出"
                                font.pixelSize: 11
                                font.bold: true
                                color: root.textMain
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: parent.color = "#F1F5F9"
                            }
                        }
                    }
                }

                // 卡片 2: DR 状态
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 74
                    radius: 6
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Text {
                            text: "DR状态"
                            font.pixelSize: 11
                            font.bold: true
                            color: root.textMuted
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 28
                            radius: 4
                            color: root.accentLightBlue
                            border.color: "#BFDBFE"

                            Text {
                                anchors.centerIn: parent
                                text: "✦ 复位到位"
                                font.pixelSize: 11
                                font.bold: true
                                color: root.accentBlue
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: parent.color = "#DBEAFE"
                            }
                        }
                    }
                }

                // 卡片 3: DDS 状态
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 80
                    radius: 6
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "DDS状态"
                                font.pixelSize: 11
                                font.bold: true
                                color: root.textMuted
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                implicitWidth: 32
                                implicitHeight: 18
                                radius: 4
                                color: root.accentLightGreen
                                border.color: "#A7F3D0"
                                Text {
                                    anchors.centerIn: parent
                                    text: "正常"
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: root.accentGreen
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 28
                            radius: 4
                            color: "#FFFFFF"
                            border.color: "#CBD5E1"

                            Text {
                                anchors.centerIn: parent
                                text: "DDS故障复位"
                                font.pixelSize: 11
                                font.bold: true
                                color: root.textMain
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: parent.color = "#F1F5F9"
                            }
                        }
                    }
                }

                // 卡片 4: 联锁停机指示
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 52
                    radius: 6
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: root.accentGreen
                        }

                        ColumnLayout {
                            spacing: 1
                            Text {
                                text: "联锁停机"
                                font.pixelSize: 11
                                font.bold: true
                                color: root.textMain
                            }
                            Text {
                                text: "USER_STOPPED"
                                font.pixelSize: 9
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
