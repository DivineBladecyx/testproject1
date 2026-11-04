import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    width: 280
    height: 185
    color: "#ffffff"
    border.color: "#cbd5e1"
    border.width: 1
    radius: 6

    // 暴露给外部 C++ 绑定的属性
    property bool wsConnected: true
    property bool tcpConnected: true
    property bool tcsCoreActive: true
    property bool oisSync: true
    property bool dispatchActive: true
    property bool devicePlcActive: true

    property string ipFrontend: "127.0.0.1:8080"
    property string ipTcp: "10.10.3.192:8081"
    property string ipTcs: "10.10.3.192:8081"
    property string ipOis: "10.10.3.192:8081"
    property string ipDispatch: "10.10.3.192:8081"
    property string ipDevice: "10.10.3.192:8081"

    // 内部动画驱动计时器
    property real animProgress: 0.0
    NumberAnimation on animProgress {
        from: 0.0
        to: 1.0
        duration: 1500
        loops: Animation.Infinite
        running: true
    }

    onAnimProgressChanged: {
        topoCanvas.requestPaint()
    }

    Column {
        anchors.fill: parent
        anchors.margins: 7
        spacing: 4

        // 1. 卡片头部
        Item {
            width: parent.width
            height: 16

            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                Text {
                    text: "🌐"
                    font.pixelSize: 11
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "通信网络拓扑 (TOPOLOGY)"
                    font.pixelSize: 10
                    font.bold: true
                    color: "#475569"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: root.wsConnected ? "#059669" : "#dc2626"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: root.wsConnected ? "ONLINE" : "OFFLINE"
                    font.pixelSize: 9
                    font.family: "Menlo"
                    font.bold: true
                    color: root.wsConnected ? "#059669" : "#dc2626"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // 分割线
        Rectangle {
            width: parent.width
            height: 1
            color: "#f1f5f9"
        }

        // 2. 拓扑中控图绘制区
        Item {
            id: topoCenterArea
            width: parent.width
            height: 124

            // 左列：Console UI + TCP Panel
            Column {
                id: colLeft
                x: 0
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                // Node 1: Console UI
                Rectangle {
                    width: 68
                    height: 32
                    radius: 4
                    color: root.wsConnected ? "#ecfdf5" : "#f8fafc"
                    border.color: root.wsConnected ? "#059669" : "#cbd5e1"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 1
                        Text {
                            text: "Console UI"
                            font.pixelSize: 9
                            font.family: "Menlo"
                            font.bold: true
                            color: root.wsConnected ? "#059669" : "#475569"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: root.wsConnected ? root.ipFrontend : "Offline"
                            font.pixelSize: 7
                            font.family: "Menlo"
                            color: root.wsConnected ? "#059669" : "#94a3b8"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                // Node 2: TCP Panel
                Rectangle {
                    width: 68
                    height: 32
                    radius: 4
                    color: root.tcpConnected ? "#ecfdf5" : "#f8fafc"
                    border.color: root.tcpConnected ? "#059669" : "#cbd5e1"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 1
                        Text {
                            text: "TCP Panel"
                            font.pixelSize: 9
                            font.family: "Menlo"
                            font.bold: true
                            color: root.tcpConnected ? "#059669" : "#475569"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: root.tcpConnected ? "Connected" : "Offline"
                            font.pixelSize: 7
                            font.family: "Menlo"
                            color: root.tcpConnected ? "#059669" : "#94a3b8"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }

            // 中间核心：TCS CORE
            Rectangle {
                id: nodeCore
                x: 92
                width: 74
                height: 38
                anchors.verticalCenter: parent.verticalCenter
                radius: 4
                color: root.tcsCoreActive ? "#eff6ff" : "#f8fafc"
                border.color: root.tcsCoreActive ? "#2563eb" : "#cbd5e1"
                border.width: 1.5

                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    Text {
                        text: "TCS CORE"
                        font.pixelSize: 9
                        font.family: "Menlo"
                        font.bold: true
                        color: root.tcsCoreActive ? "#1d4ed8" : "#475569"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: root.tcsCoreActive ? "Active:8081" : "Offline"
                        font.pixelSize: 7
                        font.family: "Menlo"
                        color: root.tcsCoreActive ? "#2563eb" : "#94a3b8"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            // 右列：OIS Gateway + Dispatch + Device PLC
            Column {
                id: colRight
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 5

                // Node: OIS
                Rectangle {
                    width: 68
                    height: 28
                    radius: 4
                    color: root.oisSync ? "#ecfdf5" : "#f8fafc"
                    border.color: root.oisSync ? "#059669" : "#cbd5e1"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        Text {
                            text: "OIS Gateway"
                            font.pixelSize: 8
                            font.family: "Menlo"
                            font.bold: true
                            color: root.oisSync ? "#059669" : "#475569"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: root.oisSync ? "Sync" : "Offline"
                            font.pixelSize: 7
                            font.family: "Menlo"
                            color: root.oisSync ? "#059669" : "#94a3b8"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                // Node: Dispatch
                Rectangle {
                    width: 68
                    height: 28
                    radius: 4
                    color: root.dispatchActive ? "#ecfdf5" : "#f8fafc"
                    border.color: root.dispatchActive ? "#059669" : "#cbd5e1"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        Text {
                            text: "Dispatch"
                            font.pixelSize: 8
                            font.family: "Menlo"
                            font.bold: true
                            color: root.dispatchActive ? "#059669" : "#475569"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: root.dispatchActive ? "Ready" : "Offline"
                            font.pixelSize: 7
                            font.family: "Menlo"
                            color: root.dispatchActive ? "#059669" : "#94a3b8"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                // Node: Device PLC
                Rectangle {
                    width: 68
                    height: 28
                    radius: 4
                    color: root.devicePlcActive ? "#ecfdf5" : "#f8fafc"
                    border.color: root.devicePlcActive ? "#059669" : "#cbd5e1"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        Text {
                            text: "Device PLC"
                            font.pixelSize: 8
                            font.family: "Menlo"
                            font.bold: true
                            color: root.devicePlcActive ? "#059669" : "#475569"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: root.devicePlcActive ? "Realtime" : "Offline"
                            font.pixelSize: 7
                            font.family: "Menlo"
                            color: root.devicePlcActive ? "#059669" : "#94a3b8"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }

            // 绘制连线与数据流光点
            Canvas {
                id: topoCanvas
                anchors.fill: parent
                z: -1

                function drawBezier(ctx, x0, y0, x1, y1, isConnected, progress) {
                    ctx.save()
                    ctx.beginPath()
                    ctx.moveTo(x0, y0)
                    var cx = (x0 + x1) / 2
                    ctx.bezierCurveTo(cx, y0, cx, y1, x1, y1)
                    ctx.strokeStyle = isConnected ? "#059669" : "#cbd5e1"
                    ctx.lineWidth = 1.5
                    ctx.stroke()

                    // 数据包脉冲微粒
                    if (isConnected) {
                        var t = progress
                        // 计算贝塞尔曲线上某点
                        var bx = Math.pow(1 - t, 3) * x0 + 3 * Math.pow(1 - t, 2) * t * cx + 3 * (1 - t) * Math.pow(t, 2) * cx + Math.pow(t, 3) * x1
                        var by = Math.pow(1 - t, 3) * y0 + 3 * Math.pow(1 - t, 2) * t * y0 + 3 * (1 - t) * Math.pow(t, 2) * y1 + Math.pow(t, 3) * y1

                        ctx.beginPath()
                        ctx.arc(bx, by, 2.5, 0, 2 * Math.PI)
                        ctx.fillStyle = "#2563eb"
                        ctx.shadowColor = "#2563eb"
                        ctx.shadowBlur = 4
                        ctx.fill()
                    }
                    ctx.restore()
                }

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    var coreLeftX = nodeCore.x
                    var coreRightX = nodeCore.x + nodeCore.width
                    var coreY = nodeCore.y + nodeCore.height / 2

                    // 左连线 1: Console UI (x=68, y=36) -> Core (x=92, y=62)
                    drawBezier(ctx, 68, colLeft.y + 16, coreLeftX, coreY, root.wsConnected, root.animProgress)

                    // 左连线 2: TCP Panel (x=68, y=76) -> Core (x=92, y=62)
                    var tcpP = (root.animProgress + 0.5) % 1.0
                    drawBezier(ctx, 68, colLeft.y + 32 + 8 + 16, coreLeftX, coreY, root.tcpConnected, tcpP)

                    // 右连线 1: Core -> OIS (colRight.x, colRight.y + 14)
                    var oisP = (root.animProgress + 0.2) % 1.0
                    drawBezier(ctx, coreRightX, coreY, colRight.x, colRight.y + 14, root.oisSync, oisP)

                    // 右连线 2: Core -> Dispatch (colRight.x, colRight.y + 28 + 5 + 14)
                    var dispP = (root.animProgress + 0.6) % 1.0
                    drawBezier(ctx, coreRightX, coreY, colRight.x, colRight.y + 28 + 5 + 14, root.dispatchActive, dispP)

                    // 右连线 3: Core -> Device (colRight.x, colRight.y + 66 + 14)
                    var devP = (root.animProgress + 0.4) % 1.0
                    drawBezier(ctx, coreRightX, coreY, colRight.x, colRight.y + 28*2 + 10 + 14, root.devicePlcActive, devP)
                }
            }
        }

        // 3. 底部微标条 (工业级协议 Indicator)
        Rectangle {
            width: parent.width
            height: 1
            color: "#e2e8f0"
        }

        Item {
            width: parent.width
            height: 16

            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: "UI: "
                    font.pixelSize: 8
                    font.family: "Menlo"
                    color: "#64748b"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: root.wsConnected ? "Connected  " : "Offline  "
                    font.pixelSize: 8
                    font.family: "Menlo"
                    font.bold: true
                    color: root.wsConnected ? "#059669" : "#dc2626"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "OIS: "
                    font.pixelSize: 8
                    font.family: "Menlo"
                    color: "#64748b"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: root.oisSync ? "Sync  " : "Offline  "
                    font.pixelSize: 8
                    font.family: "Menlo"
                    font.bold: true
                    color: root.oisSync ? "#1d4ed8" : "#94a3b8"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: "DEV: "
                    font.pixelSize: 8
                    font.family: "Menlo"
                    color: "#64748b"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: root.devicePlcActive ? "Realtime  " : "Offline  "
                    font.pixelSize: 8
                    font.family: "Menlo"
                    font.bold: true
                    color: root.devicePlcActive ? "#059669" : "#94a3b8"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 14
                radius: 2
                color: root.wsConnected ? "#ecfdf5" : "#fef2f2"
                Row {
                    anchors.centerIn: parent
                    anchors.margins: 2
                    Text {
                        text: root.wsConnected ? " NORMAL " : " OFFLINE "
                        font.pixelSize: 8
                        font.family: "Menlo"
                        font.bold: true
                        color: root.wsConnected ? "#059669" : "#dc2626"
                    }
                }
            }
        }
    }
}
