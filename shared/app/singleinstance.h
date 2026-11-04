#pragma once

#include <QString>
#include <QtGlobal>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <string>
#else
#include <QCryptographicHash>
#include <QDir>
#include <QLockFile>
#include <memory>
#endif

class SingleInstanceGuard
{
public:
    enum class Result { Acquired, AlreadyRunning, Unavailable };

    // One stable key for all installation paths, versions and frontend modes.
    explicit SingleInstanceGuard(const QString &key = QStringLiteral("Global\\SIMM.TCS.Frontend.SingleInstance"))
    {
#ifdef Q_OS_WIN
        const std::wstring name = key.toStdWString();
        SetLastError(ERROR_SUCCESS);
        HANDLE candidate = CreateMutexW(nullptr, FALSE, name.c_str());
        const DWORD error = GetLastError();
        if (!candidate) {
            m_error = QStringLiteral("Windows error %1").arg(error);
            return;
        }
        if (error == ERROR_ALREADY_EXISTS) {
            CloseHandle(candidate);
            m_result = Result::AlreadyRunning;
            return;
        }
        m_handle = candidate;
        m_result = Result::Acquired;
#else
        const QString hash = QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex());
        m_lock = std::make_unique<QLockFile>(QDir(QDir::tempPath()).filePath(QStringLiteral("simm-tcs-%1.lock").arg(hash)));
        m_lock->setStaleLockTime(0);
        if (m_lock->tryLock(0)) m_result = Result::Acquired;
        else if (m_lock->error() == QLockFile::LockFailedError) m_result = Result::AlreadyRunning;
        else m_error = QStringLiteral("Lock error %1").arg(int(m_lock->error()));
#endif
    }

    ~SingleInstanceGuard()
    {
#ifdef Q_OS_WIN
        if (m_handle) CloseHandle(m_handle);
#endif
    }
    SingleInstanceGuard(const SingleInstanceGuard &) = delete;
    SingleInstanceGuard &operator=(const SingleInstanceGuard &) = delete;
    Result result() const { return m_result; }
    QString errorDescription() const { return m_error; }

private:
    Result m_result = Result::Unavailable;
    QString m_error;
#ifdef Q_OS_WIN
    HANDLE m_handle = nullptr;
#else
    std::unique_ptr<QLockFile> m_lock;
#endif
};
