#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>

// 主题枚举
enum class Theme { Light=0, Dark=1 };

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentThemeValue READ currentThemeValue NOTIFY themeChanged)
    Q_PROPERTY(QString textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QString accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QString contentBg READ contentBg NOTIFY themeChanged)
    Q_PROPERTY(QString stepPendingText READ stepPendingText NOTIFY themeChanged)

public:
    // 获取单例实例（也可以不使用单例，根据您的架构决定）
    static ThemeManager& instance();

    // 获取当前主题
    Theme currentTheme() const;

    // 设置主题（会保存设置、应用样式表并发射信号）
    void setTheme(Theme theme);

    // 加载保存的主题（通常在程序启动时调用）
    void loadSavedTheme();
    int currentThemeValue() const;
    QString textPrimary() const;
    QString accent() const;
    QString contentBg() const;
    QString stepPendingText() const;

signals:
    // 主题变化时发出信号，参数为新主题
    void themeChanged(Theme newTheme);

private:
    // 私有构造函数（单例模式）
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() = default;

    // 实际应用样式表
    void applyTheme();

    Theme m_currentTheme = Theme::Light;
};

#endif // THEMEMANAGER_H
