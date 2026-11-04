#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QSslError>
#include <QAbstractSocket>

class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    // 连接管理
    void connectToServer(const QString &url);
    void disconnectFromServer();

    // 数据发送（支持 JSON 对象和原始文本）
    void sendData(const QJsonObject &data);
    void sendData(const QString &message);

    // 状态查询
    bool isConnected() const;

    // 自动重连设置（默认关闭）
    void setAutoReconnect(bool enabled, int intervalMs = 3000);
    void setMaxReconnectAttempts(int maxAttempts);

    // 心跳检测（发送 ping，超时自动断开）
    void setHeartbeatInterval(int intervalMs, int timeoutMs = 0);

    // SSL 错误处理（忽略所有错误，仅用于测试环境！）
    void setIgnoreSslErrors(bool ignore);

    // 中间件认证参数（连接成功或设置后立即发送）
    void setMiddlewareAuthParams(const QJsonObject &authParams);

signals:
    void connected();
    void disconnected();
    void dataReceived(const QJsonObject &data);
    void textMessageReceived(const QString &message);
    void dataSent(const QString &messageId);
    void errorOccurred(const QString &errorString);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onSslErrors(const QList<QSslError> &errors);
    void onPong(quint64 elapsedTime, const QByteArray &payload);
    void onPongTimeout();

private:
    // 核心 WebSocket 对象
    QWebSocket *m_webSocket;

    // 自动重连相关
    QTimer *m_reconnectTimer;
    bool m_autoReconnect;
    int m_reconnectInterval;          // 基础重连间隔（毫秒）
    int m_reconnectAttempts;           // 当前已尝试次数
    int m_maxReconnectAttempts;        // 最大尝试次数（0 表示无限）
    QString m_serverUrl;               // 上次连接的 URL

    // SSL 选项
    bool m_ignoreSslErrors;

    // 心跳检测相关
    QTimer *m_pingTimer;
    QTimer *m_pongTimeoutTimer;
    int m_pingInterval;                // 发送 ping 的间隔（0 表示禁用）
    int m_pongTimeout;                 // 等待 pong 的超时时间

    // 中间件认证参数
    QJsonObject m_middlewareAuthParams;

    // 内部辅助函数
    void createWebSocket();            // 创建并连接信号
    void tryReconnect();               // 尝试重连（由定时器触发）
    void reconnect();                  // 立即重连（重建 socket）
    QString generateMessageId() const;  // 生成消息唯一 ID
    int calculateReconnectDelay() const; // 计算指数退避延迟
    void sendPing();                   // 发送心跳 ping
    void startHeartbeat();             // 启用心跳定时器
    void stopHeartbeat();              // 停止心跳定时器
    void sendMiddlewareAuth();         // 发送中间件认证消息
};

#endif // WEBSOCKETCLIENT_H
