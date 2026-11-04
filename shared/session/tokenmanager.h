#ifndef TOKENMANAGER_H
#define TOKENMANAGER_H

#include <QObject>
#include<QDateTime>

class TokenManager:public QObject
{
public:
   static  TokenManager& instance();
    QString getAccessToken() const;
    QString getRefreshToken() const;
    bool isTokenValid() const;
    bool refreshToken();
    void clearTokens();
signals:
    void tokenExpired();
    void tokenRefreshed();
private:
    TokenManager(QObject *parent = nullptr);
    void saveTokens(const QString &accessToken,
                    const QString &refreshToken,
                    qint64 expiresIn);
    QString loadEncryptedToken(const QString &key) const;
    void saveEncryptedToken(const QString &key, const QString &token);

    QString m_accessToken;
    QString m_refreshToken;
    QDateTime m_expireTime;

};

#endif // TOKENMANAGER_H
