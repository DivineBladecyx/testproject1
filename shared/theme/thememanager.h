#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>

// 主题枚举
enum class Theme {
    Light = 0,
    Dark = 1
};

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentThemeValue READ currentThemeValue NOTIFY themeChanged)
    Q_PROPERTY(QString textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QString accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QString contentBg READ contentBg NOTIFY themeChanged)
    Q_PROPERTY(QString stepPendingText READ stepPendingText NOTIFY themeChanged)

public:
    static ThemeManager &instance();

    Theme currentTheme() const;
    int currentThemeValue() const;

    void loadSavedTheme();
    void setTheme(Theme theme);

    QString textPrimary() const;
    QString accent() const;
    QString contentBg() const;
    QString stepPendingText() const;

signals:
    void themeChanged(Theme newTheme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() = default;

    ThemeManager(const ThemeManager &) = delete;
    ThemeManager &operator=(const ThemeManager &) = delete;

    bool applyTheme(Theme theme);

    Theme m_currentTheme = Theme::Light;
};

#endif // THEMEMANAGER_H
