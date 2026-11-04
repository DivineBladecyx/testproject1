#include "thememanager.h"
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QDebug>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    // 构造函数可以留空，初始化列表已设置默认主题
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}
QString ThemeManager::contentBg() const
{
    return (m_currentTheme == Theme::Light)
    ? "#EAF3FF"
    : "#012F6B";
}
Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_currentTheme == theme)
        return;

    m_currentTheme = theme;

    QSettings settings("config/app_config.ini", QSettings::IniFormat);
    settings.setValue("theme", static_cast<int>(theme));
    settings.sync();

    applyTheme();
    emit themeChanged(theme);
}

void ThemeManager::loadSavedTheme()
{
    QSettings settings("config/app_config.ini", QSettings::IniFormat);
    m_currentTheme = static_cast<Theme>(
        settings.value("theme", static_cast<int>(Theme::Light)).toInt()
    );

    applyTheme();
    emit themeChanged(m_currentTheme);
}

void ThemeManager::applyTheme()
{
    // 构建样式表文件路径：可执行文件所在目录下的 config/xxx.qss
    //QString appDir = QApplication::applicationDirPath();
    //QString themeFileName = (m_currentTheme == Theme::Light) ? "style_light.qss" : "style_dark.qss";
    //QString filePath = appDir + "/config/" + themeFileName;
    QString filePath = (m_currentTheme == Theme::Light)
                           ? ":/qss/config/style_light.qss"
                           : ":/qss/config/style_dark.qss";
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Cannot open theme file:" << filePath;
        return;
    }
    qDebug() << "Loading QSS:" << filePath;
    QString styleSheet = QString::fromUtf8(file.readAll());
    qApp->setStyleSheet(styleSheet);
}
int ThemeManager::currentThemeValue() const
{
    return static_cast<int>(m_currentTheme);
}

QString ThemeManager::textPrimary() const
{
    return (m_currentTheme == Theme::Light)
    ? "#1F2D3D"
    : "#F0F6FF";
}

QString ThemeManager::accent() const
{
    return (m_currentTheme == Theme::Light)
    ? "#2F80ED"
    : "#4A90E2";
}

QString ThemeManager::stepPendingText() const
{
    return (m_currentTheme == Theme::Light)
    ? "#5B6675"
    : "#D7E4F7";
}