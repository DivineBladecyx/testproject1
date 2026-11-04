#include "thememanager.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QSettings>

namespace {

const QString kThemeSettingKey = QStringLiteral("theme");
const QString kLightStylePath = QStringLiteral(":/qss/config/style_light.qss");
const QString kDarkStylePath = QStringLiteral(":/qss/config/style_dark.qss");

QString settingsFilePath()
{
    return QApplication::applicationDirPath()
    + QStringLiteral("/config/app_config.ini");
}

bool isValidThemeValue(int value)
{
    return value == static_cast<int>(Theme::Light)
    || value == static_cast<int>(Theme::Dark);
}

} // namespace

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
}

ThemeManager &ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

int ThemeManager::currentThemeValue() const
{
    return static_cast<int>(m_currentTheme);
}

void ThemeManager::loadSavedTheme()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    const int savedValue = settings.value(
                                       kThemeSettingKey,
                                       static_cast<int>(Theme::Light))
                               .toInt();
    const Theme savedTheme = isValidThemeValue(savedValue)
                                 ? static_cast<Theme>(savedValue)
                                 : Theme::Light;

    if (!isValidThemeValue(savedValue)) {
        qWarning() << "Invalid saved theme value, using light theme:" << savedValue;
    }

    if (!applyTheme(savedTheme)) {
        return;
    }

    m_currentTheme = savedTheme;
    emit themeChanged(m_currentTheme);
}

void ThemeManager::setTheme(Theme theme)
{
    const int themeValue = static_cast<int>(theme);
    if (!isValidThemeValue(themeValue)) {
        qWarning() << "Ignoring invalid theme value:" << themeValue;
        return;
    }
    if (m_currentTheme == theme) {
        return;
    }
    if (!applyTheme(theme)) {
        return;
    }

    m_currentTheme = theme;

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue(kThemeSettingKey, themeValue);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        qWarning() << "Failed to save theme setting to" << settings.fileName();
    }

    emit themeChanged(m_currentTheme);
}

bool ThemeManager::applyTheme(Theme theme)
{
    if (!qApp) {
        qWarning() << "Cannot apply theme before QApplication is created";
        return false;
    }

    const QString filePath = theme == Theme::Light
                                 ? kLightStylePath
                                 : kDarkStylePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open theme file:" << filePath
                   << file.errorString();
        return false;
    }

    qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
    qDebug() << "Loaded QSS:" << filePath;
    return true;
}

QString ThemeManager::textPrimary() const
{
    return m_currentTheme == Theme::Light
               ? QStringLiteral("#1F2D3D")
               : QStringLiteral("#F0F6FF");
}

QString ThemeManager::accent() const
{
    return m_currentTheme == Theme::Light
               ? QStringLiteral("#2F80ED")
               : QStringLiteral("#4A90E2");
}

QString ThemeManager::contentBg() const
{
    return m_currentTheme == Theme::Light
               ? QStringLiteral("#EAF3FF")
               : QStringLiteral("#012F6B");
}

QString ThemeManager::stepPendingText() const
{
    return m_currentTheme == Theme::Light
               ? QStringLiteral("#5B6675")
               : QStringLiteral("#D7E4F7");
}
