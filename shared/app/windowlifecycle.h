#pragma once

#include "websocketclient.h"
#include "singleinstance.h"
#include <QCoreApplication>
#include <QMessageBox>
#include <QTimer>
#include <QWidget>

inline bool allowClientInstance(const SingleInstanceGuard &instance)
{
    if (instance.result() == SingleInstanceGuard::Result::Acquired) return true;
    if (instance.result() == SingleInstanceGuard::Result::AlreadyRunning) {
        QMessageBox::information(nullptr, QCoreApplication::translate("SIMM", "程序已经在运行"),
                                 QCoreApplication::translate("SIMM", "TCS 前端程序已经在运行，请使用已打开的窗口。"));
    } else {
        QMessageBox::critical(nullptr, QCoreApplication::translate("SIMM", "启动检查失败"),
                              QCoreApplication::translate("SIMM", "无法取得单实例锁，可能已有其他账户运行程序或锁访问权限不足。为避免重复启动，本次启动已取消。\n%1")
                                  .arg(instance.errorDescription()));
    }
    return false;
}

inline void configureClientMainWindow(QWidget *window)
{
    if (!window) return;
    Qt::WindowFlags flags = window->windowFlags();
    flags &= ~Qt::WindowType_Mask;
    flags &= ~Qt::FramelessWindowHint;
    flags |= Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint
             | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint;
    window->setWindowFlags(flags);
}

// Final local cleanup only, after application exit was authorized; this is NOT a stop/reset/logout.
inline void shutdownClientWindow(QWidget *window)
{
    if (!window) return;
    for (QTimer *timer : window->findChildren<QTimer *>()) timer->stop();
    for (WebSocketClient *client : window->findChildren<WebSocketClient *>()) {
        client->blockSignals(true);
        client->setAutoReconnect(false);
        client->disconnectFromServer();
    }
    window->hide();
}
