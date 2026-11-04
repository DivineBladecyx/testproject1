#include "websocketclient.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLoggingCategory>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <QWebSocket>
#include "DataProvider.h"

Q_LOGGING_CATEGORY(wsClientLog, "WebSocketClient")

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_webSocket(nullptr)
    , m_reconnectTimer(new QTimer(this))
    , m_autoReconnect(false)
    , m_reconnectInterval(3000)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(5)
    , m_ignoreSslErrors(false)
    , m_pingTimer(nullptr)
    , m_pongTimeoutTimer(nullptr)
    , m_pingInterval(0)
    , m_pongTimeout(0)
{
    // 初始化重连定时器（单次触发，每次超时后手动重启）
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::tryReconnect);

    // 初始创建 WebSocket（此时不连接）
    createWebSocket();
}

WebSocketClient::~WebSocketClient()
{
    disconnectFromServer();
    // 清理资源（Qt 父子机制会自动删除，但显式停止定时器）
    if (m_pingTimer) m_pingTimer->stop();
    if (m_pongTimeoutTimer) m_pongTimeoutTimer->stop();
    qDebug() << "[WebSocketClient::~WebSocketClient] this =" << this;
}

void WebSocketClient::createWebSocket()
{
    // 如果已有 WebSocket，先安排删除（避免泄漏）
    if (m_webSocket) {
        m_webSocket->disconnect(this); // 断开所有连接到 this 的信号
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
    }

    m_webSocket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &WebSocketClient::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketClient::onError);
    connect(m_webSocket, &QWebSocket::sslErrors, this, &WebSocketClient::onSslErrors);
    connect(m_webSocket, &QWebSocket::pong, this, &WebSocketClient::onPong);
}

void WebSocketClient::connectToServer(const QString &url)
{
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        qCInfo(wsClientLog) << "Already connected to" << url;
        return;
    }

    m_serverUrl = url;
    m_reconnectAttempts = 0; // 重置重连尝试计数
    qCInfo(wsClientLog) << "Connecting to WebSocket server:" << url;
    m_webSocket->open(QUrl(url));
}

void WebSocketClient::disconnectFromServer()
{
    m_reconnectTimer->stop();
    if (m_webSocket) {
        m_webSocket->close();
    }
    stopHeartbeat();
}

void WebSocketClient::sendData(const QJsonObject &data)
{
    QJsonDocument doc(data);
    QString message = doc.toJson(QJsonDocument::Compact);
    sendData(message);
}

void WebSocketClient::sendData(const QString &message)
{
    qDebug() << "[WebSocketClient::onDataReceived] message:" << message;
    if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred(tr("Not connected to server"));
        return;
    }

    qint64 sent = m_webSocket->sendTextMessage(message);
    if (sent > 0) {
        // 仅在需要时生成消息 ID（例如外部请求追踪）
        QString messageId = generateMessageId();
        emit dataSent(messageId);
    } else {
        emit errorOccurred(tr("Failed to send message"));
    }
}

bool WebSocketClient::isConnected() const
{
    return m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::setAutoReconnect(bool enabled, int intervalMs)
{
    m_autoReconnect = enabled;
    if (enabled) {
        m_reconnectInterval = intervalMs;
    } else {
        m_reconnectTimer->stop();
    }
}

void WebSocketClient::setMaxReconnectAttempts(int maxAttempts)
{
    m_maxReconnectAttempts = maxAttempts;
}

void WebSocketClient::setHeartbeatInterval(int intervalMs, int timeoutMs)
{
    m_pingInterval = intervalMs;
    m_pongTimeout = timeoutMs;

    if (m_pingInterval > 0) {
        if (!m_pingTimer) {
            m_pingTimer = new QTimer(this);
            connect(m_pingTimer, &QTimer::timeout, this, &WebSocketClient::sendPing);
        }
        if (m_pongTimeout > 0 && !m_pongTimeoutTimer) {
            m_pongTimeoutTimer = new QTimer(this);
            m_pongTimeoutTimer->setSingleShot(true);
            connect(m_pongTimeoutTimer, &QTimer::timeout, this, &WebSocketClient::onPongTimeout);
        }
    }

    // 如果当前已连接，立即启用心跳
    if (isConnected()) {
        startHeartbeat();
    } else {
        stopHeartbeat();
    }
}

void WebSocketClient::setIgnoreSslErrors(bool ignore)
{
    m_ignoreSslErrors = ignore;
}

void WebSocketClient::setMiddlewareAuthParams(const QJsonObject &authParams)
{
    m_middlewareAuthParams = authParams;
    qCInfo(wsClientLog) << "Middleware auth parameters set";

    if (isConnected()) {
        sendMiddlewareAuth();
    }
}

void WebSocketClient::onConnected()
{
    m_reconnectAttempts = 0;
    m_reconnectTimer->stop();
    qCInfo(wsClientLog) << "WebSocket connected successfully";
    emit connected();

    // 如果存在网关认证参数，自动发送
    if (!m_middlewareAuthParams.isEmpty()) {
        sendMiddlewareAuth();
    }

    // 启用心跳
    startHeartbeat();
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "[WebSocketClient::onDisconnected] this =" << this;
    qCInfo(wsClientLog) << "WebSocket disconnected";
    stopHeartbeat();
    emit disconnected();

    // 如果启用自动重连且未达到最大尝试次数，则启动重连定时器（使用指数退避）
    if (m_autoReconnect && (m_maxReconnectAttempts <= 0 || m_reconnectAttempts < m_maxReconnectAttempts)) {
        int delay = calculateReconnectDelay();
        m_reconnectTimer->start(delay);
        qCInfo(wsClientLog) << "Scheduling reconnect in" << delay << "ms, attempt" << m_reconnectAttempts + 1;
    }
}

void WebSocketClient::onTextMessageReceived(const QString &message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        emit dataReceived(doc.object());
    } else {
        emit textMessageReceived(message);
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    qDebug() << "[WebSocketClient::onError] this =" << this << "error =" << error;
    Q_UNUSED(error)
    if (!m_webSocket) {
        emit errorOccurred(tr("Unknown error (socket destroyed)"));
        return;
    }
    QString errorStr = m_webSocket->errorString();
    qCWarning(wsClientLog) << "WebSocket error:" << errorStr;
    emit errorOccurred(errorStr);
}

void WebSocketClient::onSslErrors(const QList<QSslError> &errors)
{
    qCWarning(wsClientLog) << "SSL errors occurred:";
    for (const QSslError &e : errors) {
        qCWarning(wsClientLog) << "  -" << e.errorString();
    }

    if (m_ignoreSslErrors && m_webSocket) {
        // 忽略所有 SSL 错误（仅适用于测试环境！）
        m_webSocket->ignoreSslErrors();
    } else {
        // 可以选择断开连接
        // m_webSocket->close();
    }
}

void WebSocketClient::onPong(quint64 elapsedTime, const QByteArray &payload)
{
    Q_UNUSED(elapsedTime)
    Q_UNUSED(payload)
    // 收到 pong，停止超时计时器
    if (m_pongTimeoutTimer) {
        m_pongTimeoutTimer->stop();
    }
    qDebug() << "[WebSocketClient::sendPing]";
}

void WebSocketClient::onPongTimeout()
{
    qCWarning(wsClientLog) << "Pong timeout, connection may be dead. Closing socket.";
    if (m_webSocket) {
        m_webSocket->abort(); // 强制关闭，触发 disconnected
    }
    qDebug() << "[WebSocketClient::sendPing]";
}

void WebSocketClient::tryReconnect()
{
    qDebug() << "[WebSocketClient::tryReconnect] 尝试重连，attempt=" << m_reconnectAttempts;

    if (m_autoReconnect && !isConnected()) {
        m_reconnectAttempts++;
        qCInfo(wsClientLog) << "Attempting to reconnect... (attempt" << m_reconnectAttempts << ")";
        reconnect();
    }
}

void WebSocketClient::reconnect()
{
    // 保存自动重连状态，临时禁用（避免 onDisconnected 中再次触发定时器）
    bool wasAutoReconnect = m_autoReconnect;
    m_autoReconnect = false;

    // 关闭旧连接（异步，但会触发 onDisconnected，但此时 m_autoReconnect=false 不会启动定时器）
    if (m_webSocket) {
        m_webSocket->close();
    }

    // 重建 WebSocket（同时断开旧对象的所有信号连接）
    createWebSocket();

    // 恢复自动重连设置
    m_autoReconnect = wasAutoReconnect;

    // 尝试连接
    if (!m_serverUrl.isEmpty()) {
        m_webSocket->open(QUrl(m_serverUrl));
    }

    // 停止重连定时器（因为刚刚尝试连接，若失败将由 onDisconnected 再次启动）
    m_reconnectTimer->stop();

    qCInfo(wsClientLog) << "Reconnecting to" << m_serverUrl;
}

QString WebSocketClient::generateMessageId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

int WebSocketClient::calculateReconnectDelay() const
{
    // 指数退避：初始间隔，每次尝试加倍，最大不超过 30 秒
    int delay = m_reconnectInterval * (1 << qMin(m_reconnectAttempts, 5)); // 最多 32 倍
    return qMin(delay, 30000);
}

void WebSocketClient::sendPing()
{
    qDebug() << "[WebSocketClient::sendPing] 发送 ping";
    if (!isConnected()) {
        stopHeartbeat();
        return;
    }

    // 发送 ping 帧（可携带负载）
    m_webSocket->ping(QUuid::createUuid().toByteArray(QUuid::WithoutBraces));

    // 启动超时计时器
    if (m_pongTimeout > 0 && m_pongTimeoutTimer) {
        m_pongTimeoutTimer->start(m_pongTimeout);
    }
}

void WebSocketClient::startHeartbeat()
{
    if (m_pingInterval > 0 && m_pingTimer) {
        m_pingTimer->start(m_pingInterval);
    }
}

void WebSocketClient::stopHeartbeat()
{
    if (m_pingTimer) {
        m_pingTimer->stop();
    }
    if (m_pongTimeoutTimer) {
        m_pongTimeoutTimer->stop();
    }
}

void WebSocketClient::sendMiddlewareAuth()
{
    if (m_middlewareAuthParams.isEmpty()) {
        return;
    }

    QJsonObject authMsg;
    authMsg["type"] = "auth";
    authMsg["data"] = m_middlewareAuthParams;
    authMsg["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    sendData(authMsg);
    qCInfo(wsClientLog) << "Middleware authentication message sent";
}
