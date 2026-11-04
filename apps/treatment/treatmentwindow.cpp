#include "treatmentwindow.h"
#include "ui_treatmentwindow.h"
#include "workflowoverview.h"
#include "faultdisplay.h"

#include "logindialog.h"
#include "websocketclient.h"
#include "thememanager.h"
#include "todayschedule.h"
#include "DataProvider.h"
#include "telemetryvalue.h"
#include "roomcapabilities.h"
#include "auditlogger.h"
#include "spotmapwidget.h"

#include <QMessageBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QStatusBar>
#include <QDebug>
#include <QUrl>
#include <QUuid>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTextEdit>
#include <QDateTime>
#include <QQmlContext>
#include <QQuickWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QQuickItem>
#include <QTimer>
#include <QLocale>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QTabWidget>
#include <QToolButton>
#include <QIcon>
#include <QSize>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QStringList>
#include <QMap>
#include <QStyle>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QTreeWidget>
#include <QMenu>
#include <utility>
#include <cmath>

namespace {

// Presentation-only circular replacement for the original treatment MU progress bar.
// QProgressBar's range, value, format and tooltip remain the source of truth.
class CircularDoseProgressBar final : public QProgressBar
{
public:
    explicit CircularDoseProgressBar(QWidget *parent = nullptr) : QProgressBar(parent)
    {
        setTextVisible(false);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedSize(164, 164);
        setProperty("clinicalHasMuProgress", false);
    }

    QSize sizeHint() const override { return QSize(164, 164); }
    QSize minimumSizeHint() const override { return QSize(164, 164); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const bool dark = ThemeManager::instance().currentTheme() == Theme::Dark;
        const QColor track = dark ? QColor("#40546B") : QColor("#E4EBF3");
        const QColor progress = dark ? QColor("#69ABFF") : QColor("#3785E7");
        const QColor ink = dark ? QColor("#EBF3FF") : QColor("#173B60");
        const QColor hint = dark ? QColor("#AEC3D7") : QColor("#74899B");
        const bool known = property("clinicalHasMuProgress").toBool() && maximum() > minimum();
        const double fraction = known
            ? qBound(0.0, double(value() - minimum()) / double(maximum() - minimum()), 1.0)
            : 0.0;

        const QPointF mid(width() * 0.5, height() * 0.5);
        const qreal diameter = qMin(width(), height()) - 22.0;
        const QRectF arc(mid.x() - diameter / 2.0, mid.y() - diameter / 2.0,
                         diameter, diameter);
        QPen trackPen(track, 10, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(trackPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawArc(arc, 90 * 16, -360 * 16);

        if (known && fraction > 0.0) {
            QPen progressPen(progress, 10, Qt::SolidLine, Qt::RoundCap);
            painter.setPen(progressPen);
            painter.drawArc(arc, 90 * 16, -qRound(360.0 * 16.0 * fraction));
        }

        painter.setPen(ink);
        QFont big = font();
        big.setPixelSize(24);
        big.setBold(true);
        painter.setFont(big);
        const QString percent = known ? QStringLiteral("%1%").arg(fraction * 100.0, 0, 'f', 1)
                                      : QStringLiteral("—");
        painter.drawText(QRectF(9, mid.y() - 30, width() - 18, 40),
                         Qt::AlignCenter, percent);

        painter.setPen(hint);
        QFont small = font();
        small.setPixelSize(12);
        small.setBold(false);
        painter.setFont(small);
        painter.drawText(QRectF(9, mid.y() + 12, width() - 18, 27), Qt::AlignCenter,
                         known ? QObject::tr("MU 进度") : QObject::tr("等待MU回读"));
    }
};

void activateDialog(QDialog *dialog)
{
    if (!dialog || !dialog->isVisible()) return;
    QWidget *target = QApplication::activeModalWidget();
    if (!target || (target != dialog && !dialog->isAncestorOf(target))) target = dialog;
    target->raise();
    target->activateWindow();
}

void rejectWorkflowDialogs(QWidget *owner, QDialog *except = nullptr)
{
    QVector<QPointer<QDialog>> dialogs;
    for (QDialog *dialog : owner->findChildren<QDialog *>()) {
        if (dialog != except && dialog->isVisible()) dialogs.append(dialog);
    }
    // Unwind nested message boxes before rejecting their owning confirmation.
    for (auto it = dialogs.crbegin(); it != dialogs.crend(); ++it) {
        if (*it) (*it)->reject();
    }
}

// Nested Qt layouts can underestimate wrapped alert height after a theme/width change.
class WorkflowTextLabel final : public QLabel
{
public:
    using QLabel::QLabel;
    void setMessage(const QString &message) {
        if (text() != message) { setText(message); m_measuredWidth = -1; }
        fitText();
    }
protected:
    void resizeEvent(QResizeEvent *event) override { QLabel::resizeEvent(event); fitText(); }
    void showEvent(QShowEvent *event) override { QLabel::showEvent(event); m_measuredWidth = -1; fitText(); }
    void changeEvent(QEvent *event) override {
        QLabel::changeEvent(event);
        if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) {
            m_measuredWidth = -1;
            fitText();
        }
    }
private:
    void fitText() {
        if (!isVisible()) { m_measuredWidth = -1; return; }
        if (width() == m_measuredWidth) return;
        m_measuredWidth = width();
        // QLabel's heightForWidth is bounded by its existing minimum height.
        setMinimumHeight(0);
        const int height = heightForWidth(width());
        if (height >= 0) setMinimumHeight(height);
    }
    int m_measuredWidth = -1;
};

class ClinicalReadbackView final : public QTextEdit
{
public:
    explicit ClinicalReadbackView(QWidget *parent) : QTextEdit(parent) {
        setReadOnly(true);
        setFrameShape(QFrame::NoFrame);
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        document()->setDocumentMargin(0);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumWidth(0);
        setProperty("clinicalRole", "identifier");
    }
    void setReadback(const QString &text) {
        if (toPlainText() != text) setPlainText(text);
        measure();
    }
protected:
    void resizeEvent(QResizeEvent *event) override { QTextEdit::resizeEvent(event); measure(); }
private:
    void measure() {
        if (viewport()->width() <= 0) return;
        document()->setTextWidth(viewport()->width());
        const int wanted = qMax(34, static_cast<int>(std::ceil(document()->size().height())) + 12);
        if (height() != wanted) setFixedHeight(wanted);
    }
};

void setTreatmentRole(QWidget *widget, const QString &role)
{
    widget->setStyleSheet(QString());
    widget->setProperty("tcsStyle", role);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QString dispatchMonitorText(const QJsonObject &payload, int section = 0)
{
    const auto value = [&payload](const char *key) {
        const QJsonValue field = payload.value(QLatin1String(key));
        if (field.isDouble()) return QString::number(field.toDouble(), 'g', 8);
        if (field.isString()) return field.toString();
        return QStringLiteral("-");
    };
    const auto flag = [&payload](const char *key, const QString &yes, const QString &no) {
        const QJsonValue field = payload.value(QLatin1String(key));
        const QJsonValue binary = TelemetryValue::binaryFlag(field);
        return binary.isBool() ? (binary.toBool() ? yes : no) : QObject::tr("未知");
    };
    QString result = QObject::tr("未知");
    const QJsonValue delivery = payload.value("beam_delivery_result");
    const int deliveryCode = delivery.isDouble() && delivery.toDouble() == delivery.toInt(-1)
        ? delivery.toInt(-1) : -1;
    if (deliveryCode >= 0) {
        switch (deliveryCode) {
        case 0: result = QObject::tr("等待"); break;
        case 1: result = QObject::tr("治疗中"); break;
        case 2: result = QObject::tr("治疗正常完成"); break;
        case 7: result = QObject::tr("出束故障报警"); break;
        default: result = QObject::tr("未知结果码 %1").arg(value("beam_delivery_result")); break;
        }
    }
    QStringList lines;
    QStringList interlocks;
    QStringList devices;
    const QString supply = deliveryCode == 0 || deliveryCode == 2 ? QObject::tr("未供束")
        : deliveryCode == 1 ? QObject::tr("供束中")
        : deliveryCode == 7 ? QObject::tr("故障，实际供束状态未确定")
        : QObject::tr("未知");
    lines << QObject::tr("供束：%1 | 照射结果：%2").arg(supply, result);
    // The published protocol defines current_dose as current-layer cumulative,
    // sum_dose as cumulative MU and dose_rate as Gy/s. Never relabel as MU/s.
    lines << QObject::tr("当前层累计：%1 | 累计MU：%2 | 剂量率：%3 Gy/s")
        .arg(value("current_dose"), value("sum_dose"), value("dose_rate"));
    lines << QObject::tr("当前点：%1 | 当前层：%2 | 下一层：%3 | DDS计划：%4")
        .arg(value("current_point"), value("current_layer"), value("next_layer"), value("therapy_id"));
    interlocks << QObject::tr("DDS联锁：%1 | 联锁使能：%2 | 加速器就绪：%3")
        .arg(flag("interlock_ok", QObject::tr("正常"), QObject::tr("触发")),
             flag("dds_ilk_en_rb", QObject::tr("已使能"), QObject::tr("未使能")),
             flag("accel_ready", QObject::tr("就绪"), QObject::tr("未就绪")));
    if (delivery.isDouble() && delivery.toInt(-1) == 7) {
        const QStringList faultTypes = {QObject::tr("无"), QObject::tr("急停"), QObject::tr("层扫描故障")};
        const QStringList faultDetails = {QObject::tr("无"), QObject::tr("急停"), QObject::tr("能量异常"),
                                          QObject::tr("剂量超限"), QObject::tr("位置偏离"), QObject::tr("点剂量累积超时")};
        const auto description = [&payload](const char *key, const QStringList &names) {
            const QJsonValue field = payload.value(QLatin1String(key));
            const int code = field.isDouble() ? field.toInt(-1) : -1;
            return code >= 0 && code < names.size() ? names.at(code) : QObject::tr("未知");
        };
        devices << QObject::tr("治疗故障：%1 | 详情：%2")
            .arg(description("therapy_fault_code", faultTypes), description("therapy_fault_info", faultDetails));
        devices << QObject::tr("故障码：治疗 %1 / 详情 %2 / 点 %3 / 层 %4")
            .arg(value("therapy_fault_code"), value("therapy_fault_info"),
                 value("point_fault_code"), value("layer_fault_code"));
    } else {
        devices << (delivery.isDouble() ? QObject::tr("出束故障：当前结果未报告故障")
                                       : QObject::tr("出束故障：等待遥测"));
    }
    if (section == 1) return interlocks.join(QLatin1Char('\n'));
    if (section == 2) return devices.join(QLatin1Char('\n'));
    return lines.join(QLatin1Char('\n'));
}

QString liveBeamDeliveryText(const QJsonObject &payload, const LogoutState &state, qint64 maxAgeMs)
{
    if (!state.deliveryFresh(maxAgeMs)) return QObject::tr("供束：未知 | 等待实时出束结果回读");
    if (state.liveBeam.isBool() && state.liveBeam.toBool() && state.beamAge.isValid()
        && state.beamAge.elapsed() < maxAgeMs && state.deliveryResult != 1)
        return QObject::tr("供束：状态冲突，按供束中处理 | 请核实设备回读");
    return dispatchMonitorText(payload).section(QLatin1Char('\n'), 0, 0);
}

void updateGantryIndicators(QWidget *window, const QJsonObject &payload)
{
    auto *faultLamp = window->findChild<QLabel *>(QStringLiteral("ddsFaultLamp"));
    auto *faultText = window->findChild<QLabel *>(QStringLiteral("ddsFaultText"));
    if (faultLamp && faultText) {
        const QJsonValue result = payload.value(QStringLiteral("beam_delivery_result"));
        const bool known = result.isDouble()
            && (result.toInt(-1) == 0 || result.toInt(-1) == 1
                || result.toInt(-1) == 2 || result.toInt(-1) == 7);
        const bool fault = known && result.toInt(-1) == 7;
        const QString state = !known ? QObject::tr("状态未知")
            : fault ? QObject::tr("故障") : QObject::tr("未报告故障");
        faultLamp->setProperty("statusTone", !known ? 0 : fault ? 4 : 2);
        setTreatmentRole(faultLamp, QStringLiteral("statusDot"));
        faultText->setText(QObject::tr("DDS出束：%1").arg(state));
        const QString details = dispatchMonitorText(payload, 2);
        faultLamp->setToolTip(details);
        faultText->setToolTip(details);
        faultLamp->setAccessibleName(faultText->text() + QLatin1Char('\n') + details);
    }
    auto *acceleratorLamp = window->findChild<QLabel *>(QStringLiteral("acceleratorReadyLamp"));
    auto *acceleratorText = window->findChild<QLabel *>(QStringLiteral("acceleratorReadyText"));
    if (acceleratorLamp && acceleratorText) {
        const QJsonValue ready = TelemetryValue::binaryFlag(
            payload.value(QStringLiteral("accel_ready")));
        const int tone = ready.isBool() ? (ready.toBool() ? 2 : 3) : 0;
        const QString state = ready.isBool()
            ? (ready.toBool() ? QObject::tr("就绪") : QObject::tr("未就绪"))
            : QObject::tr("状态未知");
        acceleratorLamp->setProperty("statusTone", tone);
        setTreatmentRole(acceleratorLamp, QStringLiteral("statusDot"));
        acceleratorText->setText(QObject::tr("加速器：%1").arg(state));
        acceleratorLamp->setToolTip(acceleratorText->text());
        acceleratorLamp->setAccessibleName(acceleratorText->text());
    }
    for (const QString &gantry : {QStringLiteral("g01"), QStringLiteral("g02")}) {
        auto *lamp = window->findChild<QLabel *>(gantry + QStringLiteral("HomedLamp"));
        auto *text = window->findChild<QLabel *>(gantry + QStringLiteral("HomedText"));
        if (!lamp || !text) continue;
        const QJsonValue homed = TelemetryValue::binaryFlag(
            payload.value(gantry + QStringLiteral("_homed")));
        const int tone = homed.isBool() ? (homed.toBool() ? 2 : 3) : 0;
        const QString state = homed.isBool()
            ? (homed.toBool() ? QObject::tr("到位") : QObject::tr("未到位"))
            : QObject::tr("状态未知");
        lamp->setProperty("statusTone", tone);
        setTreatmentRole(lamp, QStringLiteral("statusDot"));
        text->setText(QObject::tr("机架 %1：%2").arg(gantry.toUpper(), state));
        lamp->setToolTip(text->text());
        lamp->setAccessibleName(text->text());
    }
}

void updateDdsInterlock(QWidget *panel, const QJsonObject &payload)
{
    auto *lamp = panel->findChild<QLabel *>(QStringLiteral("label_46"));
    auto *title = panel->findChild<QLabel *>(QStringLiteral("label_37"));
    const QJsonValue ok = TelemetryValue::binaryFlag(payload.value("interlock_ok"));
    const QJsonValue enabled = TelemetryValue::binaryFlag(payload.value("dds_ilk_en_rb"));
    int tone = 0;
    QString state = QObject::tr("状态未知：等待联锁及使能反馈");
    if (ok.isBool() && !ok.toBool()) {
        tone = 4;
        state = QObject::tr("联锁触发：具体原因未提供");
    } else if (enabled.isBool() && !enabled.toBool()) {
        tone = 3;
        state = QObject::tr("保护未使能");
    } else if (ok.isBool() && enabled.isBool()) {
        tone = 2;
        state = QObject::tr("正常：联锁保护已使能");
    }
    const auto raw = [](const QJsonValue &value) {
        return value.isBool() ? (value.toBool() ? QStringLiteral("true") : QStringLiteral("false"))
                              : QObject::tr("未知");
    };
    const QString tooltip = state + QLatin1Char('\n')
        + QObject::tr("联锁状态（interlock_ok）：%1\n保护使能（dds_ilk_en_rb）：%2")
        .arg(raw(ok), raw(enabled));
    lamp->setProperty("statusTone", tone);
    setTreatmentRole(lamp, QStringLiteral("statusDot"));
    lamp->setToolTip(tooltip);
    lamp->setAccessibleName(QObject::tr("DDS联锁：%1").arg(state));
    title->setText(QObject::tr("DDS联锁"));
    title->setToolTip(tooltip);
}

QString appConfigPath()
{
    return QApplication::applicationDirPath()
           + QStringLiteral("/config/app_config.ini");
}

bool isSensitiveField(const QString &key)
{
    static const QStringList sensitiveFragments = {
        QStringLiteral("password"),
        QStringLiteral("token"),
        QStringLiteral("secret"),
        QStringLiteral("authorization"),
        QStringLiteral("credential")
    };

    for (const QString &fragment : sensitiveFragments) {
        if (key.contains(fragment, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QJsonValue maskSensitiveValue(const QJsonValue &value);

QJsonObject maskSensitivePayload(const QJsonObject &source)
{
    QJsonObject masked;
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        if (isSensitiveField(it.key())) {
            masked.insert(it.key(), QStringLiteral("***"));
        } else {
            masked.insert(it.key(), maskSensitiveValue(it.value()));
        }
    }
    return masked;
}

QJsonValue maskSensitiveValue(const QJsonValue &value)
{
    if (value.isObject()) {
        return maskSensitivePayload(value.toObject());
    }
    if (value.isArray()) {
        QJsonArray masked;
        const QJsonArray values = value.toArray();
        for (const QJsonValue &item : values) {
            masked.append(maskSensitiveValue(item));
        }
        return masked;
    }
    return value;
}

QString readStringValue(const QJsonObject &object,
                        std::initializer_list<const char*> keys,
                        const QString &fallback = QStringLiteral("-"))
{
    for (const char *key : keys) {
        const QJsonValue value = object.value(QLatin1String(key));
        if (value.isString() && !value.toString().isEmpty()) {
            return value.toString();
        }
        if (value.isDouble()) {
            return QString::number(value.toDouble());
        }
    }
    return fallback;
}

QString readStringValue(const QJsonObject &primary,
                        const QJsonObject &secondary,
                        std::initializer_list<const char*> keys,
                        const QString &fallback = QStringLiteral("-"))
{
    const QString primaryValue = readStringValue(primary, keys, QString());
    if (!primaryValue.isEmpty()) {
        return primaryValue;
    }
    return readStringValue(secondary, keys, fallback);
}

// An account id / username is not necessarily an employee number. Do not
// silently display those values in the work-number field.
QString operatorEmployeeNo(const QJsonObject &userInfo)
{
    return readStringValue(userInfo,
                           {"employee_no", "employee_id", "staff_no", "staff_id",
                            "job_number", "work_no", "work_number", "personnel_no"},
                           QString()).trimmed();
}

double readDoubleValue(const QJsonObject &object,
                       std::initializer_list<const char*> keys,
                       double fallback = -1.0)
{
    for (const char *key : keys) {
        const QJsonValue value = object.value(QLatin1String(key));
        if (value.isDouble()) {
            return value.toDouble();
        }
        if (value.isString()) {
            bool ok = false;
            QString text = value.toString();
            text.remove(QStringLiteral("Gy"), Qt::CaseInsensitive);
            const double number = text.trimmed().toDouble(&ok);
            if (ok) {
                return number;
            }
        }
    }
    return fallback;
}

double readDoubleValue(const QJsonObject &primary,
                       const QJsonObject &secondary,
                       std::initializer_list<const char*> keys,
                       double fallback = -1.0)
{
    const double primaryValue = readDoubleValue(primary, keys, -1.0);
    if (primaryValue >= 0.0) {
        return primaryValue;
    }
    return readDoubleValue(secondary, keys, fallback);
}

int readIntValue(const QJsonObject &object,
                 std::initializer_list<const char*> keys,
                 int fallback = -1)
{
    for (const char *key : keys) {
        const QJsonValue value = object.value(QLatin1String(key));
        if (value.isDouble()) {
            return value.toInt();
        }
        if (value.isString()) {
            bool ok = false;
            const int number = value.toString().toInt(&ok);
            if (ok) {
                return number;
            }
        }
    }
    return fallback;
}

int readIntValue(const QJsonObject &primary,
                 const QJsonObject &secondary,
                 std::initializer_list<const char*> keys,
                 int fallback = -1)
{
    const int primaryValue = readIntValue(primary, keys, -1);
    if (primaryValue >= 0) {
        return primaryValue;
    }
    return readIntValue(secondary, keys, fallback);
}

void setLabelTextIfChanged(QLabel *label, const QString &text)
{
    if (label && label->text() != text) {
        label->setText(text);
    }
}

void setProgressBarIfChanged(QProgressBar *progressBar,
                             int minimum,
                             int maximum,
                             int value,
                             const QString &format)
{
    if (!progressBar) {
        return;
    }

    if (progressBar->minimum() != minimum || progressBar->maximum() != maximum) {
        progressBar->setRange(minimum, maximum);
    }
    if (progressBar->value() != value) {
        progressBar->setValue(value);
    }
    if (progressBar->format() != format) {
        progressBar->setFormat(format);
    }
}





QString doseTelemetrySnapshot(const QJsonObject &payload)
{
    QJsonObject snapshot = payload;

    const int currentLayer = readIntValue(payload, {"current_layer"});
    if (currentLayer >= 0) {
        snapshot.insert(QStringLiteral("current_layer"), currentLayer);
    }

    const int currentBeam = readIntValue(
        payload, {"current_beam", "current_beam_number", "beam_number"});
    if (currentBeam >= 0) {
        snapshot.insert(QStringLiteral("current_beam"), currentBeam);
    }

    // These are different physical quantities: sum_dose is MU;
    // workflow cumulative_dose is cGy. Never merge their values.
    const double cumulativeDoseCgy = readDoubleValue(payload, {"cumulative_dose"});
    if (cumulativeDoseCgy >= 0.0)
        snapshot.insert(QStringLiteral("cumulative_dose"), QString::number(cumulativeDoseCgy, 'f', 2));
    const double cumulativeMu = readDoubleValue(payload, {"sum_dose"});
    if (cumulativeMu >= 0.0)
        snapshot.insert(QStringLiteral("sum_dose"), QString::number(cumulativeMu, 'f', 2));

    const double actualMu = readDoubleValue(payload, {"actual_mu", "delivered_mu"});
    if (actualMu >= 0.0) {
        snapshot.insert(QStringLiteral("actual_mu"), QString::number(actualMu, 'f', 1));
    }

    const double doseDeviation = readDoubleValue(payload, {"dose_deviation", "dose_deviation_percent"});
    if (doseDeviation >= 0.0) {
        snapshot.insert(QStringLiteral("dose_deviation"), QString::number(doseDeviation, 'f', 1));
    }

    for (const QString &key : {QStringLiteral("interruption_status"),
                               QStringLiteral("is_delivering")}) {
        if (!payload.contains(key)) continue;
        const QJsonValue normalized = TelemetryValue::binaryFlag(payload.value(key));
        if (normalized.isBool()) snapshot.insert(key, normalized);
    }
    const QString compensationStatus = payload.value(QStringLiteral("interruption_compensation_status")).toString(
        payload.value(QStringLiteral("compensation_status")).toString());
    if (!compensationStatus.isEmpty()) {
        snapshot.insert(QStringLiteral("compensation_status"), compensationStatus);
    }

    if (snapshot.isEmpty()) {
        return QString();
    }

    return QString::fromUtf8(QJsonDocument(snapshot).toJson(QJsonDocument::Compact));
}

QString gyText(double value, int precision = 1)
{
    return value >= 0.0 ? QStringLiteral("%1 Gy").arg(value, 0, 'f', precision)
                        : QStringLiteral("-");
}

double fractionBeamDoseGy(const QJsonObject &summary)
{
    const QJsonArray fractionGroups = summary.value(QStringLiteral("fraction_groups")).toArray();
    if (fractionGroups.isEmpty()) {
        return -1.0;
    }

    const QJsonArray referencedBeams = fractionGroups.first()
                                            .toObject()
                                            .value(QStringLiteral("referenced_beams"))
                                            .toArray();
    if (referencedBeams.isEmpty()) {
        return -1.0;
    }

    double beamDoseSum = 0.0;
    bool hasBeamDose = false;
    for (const QJsonValue &beamValue : referencedBeams) {
        const double beamDose = readDoubleValue(beamValue.toObject(), {"beam_dose_gy"});
        if (beamDose >= 0.0) {
            beamDoseSum += beamDose;
            hasBeamDose = true;
        }
    }

    return hasBeamDose ? beamDoseSum : -1.0;
}

QJsonArray normalizedLayerGroups(const QJsonObject &payload)
{
    const QJsonArray rawLayers = payload.value(QStringLiteral("layers")).toArray();
    if (rawLayers.isEmpty()) {
        return {};
    }

    if (rawLayers.first().isArray()) {
        return rawLayers;
    }

    if (rawLayers.first().isObject()) {
        QJsonArray singleBeamLayers;
        singleBeamLayers.append(rawLayers);
        return singleBeamLayers;
    }

    return {};
}

QJsonArray beamsWithPlanLayers(const QJsonArray &summaryBeams,
                               const QJsonObject &payload)
{
    const QJsonArray layerGroups = normalizedLayerGroups(payload);
    if (layerGroups.isEmpty()) {
        return summaryBeams;
    }

    QJsonArray mergedBeams;
    const qsizetype beamCount = qMax(summaryBeams.size(), layerGroups.size());
    for (qsizetype beamIndex = 0; beamIndex < beamCount; ++beamIndex) {
        QJsonObject beam = beamIndex < summaryBeams.size()
            ? summaryBeams.at(beamIndex).toObject()
            : QJsonObject();
        if (!beam.contains(QStringLiteral("beam_number"))) {
            beam.insert(QStringLiteral("beam_number"), static_cast<int>(beamIndex + 1));
        }

        QJsonArray beamLayers;
        if (beamIndex < layerGroups.size()) {
            beamLayers = layerGroups.at(beamIndex).toArray();
        }
        if (beamLayers.isEmpty()) {
            beamLayers = beam.value(QStringLiteral("layers")).toArray();
        }
        beam.insert(QStringLiteral("layers"), beamLayers);

        if (!beamLayers.isEmpty()) {
            beam.insert(QStringLiteral("energy_layers"), beamLayers.size());

            int totalSpots = 0;
            for (const QJsonValue &layerValue : beamLayers) {
                const QJsonObject layer = layerValue.toObject();
                int layerSpots = readIntValue(layer, {"num_spots"}, -1);
                if (layerSpots < 0) {
                    layerSpots = layer.value(QStringLiteral("spot_meterset_weights"))
                                     .toArray()
                                     .size();
                }
                totalSpots += qMax(0, layerSpots);
            }
            beam.insert(QStringLiteral("total_scan_spots"), totalSpots);
        }

        mergedBeams.append(beam);
    }

    return mergedBeams;
}

double spotMetersetWeightSum(const QJsonObject &layer)
{
    const QJsonArray spotWeights =
        layer.value(QStringLiteral("spot_meterset_weights")).toArray();
    double total = 0.0;
    bool hasWeight = false;
    for (const QJsonValue &spotWeight : spotWeights) {
        if (spotWeight.isDouble()) {
            total += spotWeight.toDouble();
            hasWeight = true;
        }
    }
    return hasWeight ? total : -1.0;
}

double plannedMetersetForLayer(const QJsonArray &layers,
                               qsizetype layerPosition)
{
    if (layerPosition < 0 || layerPosition >= layers.size()) {
        return -1.0;
    }

    const QJsonObject layer = layers.at(layerPosition).toObject();
    const double directMeterset =
        readDoubleValue(layer, {"layer_meterset", "planned_mu", "meterset"});
    if (directMeterset >= 0.0) {
        return directMeterset;
    }

    const double spotWeightSum = spotMetersetWeightSum(layer);
    if (spotWeightSum >= 0.0) {
        return spotWeightSum;
    }

    const double currentCumulative =
        readDoubleValue(layer, {"cumulative_meterset_weight"});
    if (currentCumulative < 0.0) {
        return -1.0;
    }

    if (layerPosition == 0) {
        return currentCumulative;
    }

    const double previousCumulative = readDoubleValue(
        layers.at(layerPosition - 1).toObject(),
        {"cumulative_meterset_weight"});
    return previousCumulative >= 0.0
        ? qMax(0.0, currentCumulative - previousCumulative)
        : -1.0;
}

struct PlanDisplayInfo
{
    QJsonObject payload;
    QJsonObject summary;
    QString patientId;
    QString patientName;
    QString planId;
    QString fractionId;
    QString treatmentSite;
    QString targetArea;
    QString qaStatus;
    QString releaseStatus;
    QString releaseReason;
    QString approvalStatus;
    QString fractionDoseSource;
    QString prescriptionText;
    QString isocenterText;
    double prescribedDose = -1.0;
    double fractionDose = -1.0;
    double completedDose = -1.0;
    double plannedMu = -1.0;
    int totalFractions = -1;
    int currentFraction = -1;
    QJsonArray beams;
};

PlanDisplayInfo parsePlanDisplayInfo(const QJsonObject &payload)
{
    PlanDisplayInfo info;
    info.payload = payload;

    info.summary = payload.value(QStringLiteral("plan_summary")).toObject();
    if (info.summary.isEmpty()) {
        info.summary = payload.value(QStringLiteral("plan_package"))
                           .toObject()
                           .value(QStringLiteral("summary_json"))
                           .toObject();
    }

    info.patientId = payload.value(QStringLiteral("patient_id")).toString(QStringLiteral("-"));
    info.patientName = payload.value(QStringLiteral("patient_name")).toString(QStringLiteral("-"));
    info.planId = payload.value(QStringLiteral("plan_id")).toString(QStringLiteral("-"));
    info.fractionId = payload.value(QStringLiteral("fraction_id")).toString(QStringLiteral("-"));
    info.treatmentSite = payload.value(QStringLiteral("treatment_site")).toString(QStringLiteral("-"));
    info.targetArea = info.summary.value(QStringLiteral("target_area")).toString(QStringLiteral("-"));
    const QJsonArray isocenter = info.summary.value(QStringLiteral("isocenter_mm")).toArray();
    const bool hasIsocenter = isocenter.size() == 3
        && isocenter.at(0).isDouble()
        && isocenter.at(1).isDouble()
        && isocenter.at(2).isDouble();
    info.isocenterText = hasIsocenter
        ? QStringLiteral("X: %1 mm, Y: %2 mm, Z: %3 mm")
              .arg(isocenter.at(0).toDouble(), 0, 'f', 3)
              .arg(isocenter.at(1).toDouble(), 0, 'f', 3)
              .arg(isocenter.at(2).toDouble(), 0, 'f', 3)
        : QStringLiteral("-");
    info.qaStatus = payload.value(QStringLiteral("dose_confirmation_status")).toString(QStringLiteral("-"));
    info.releaseStatus = payload.value(QStringLiteral("release_status")).toString(QStringLiteral("-"));
    info.releaseReason = payload.value(QStringLiteral("release_reason")).toString(QStringLiteral("-"));
    info.approvalStatus = info.summary.value(QStringLiteral("approval_status")).toString(QStringLiteral("-"));
    info.fractionDoseSource = info.summary.value(QStringLiteral("dose_per_fraction_source")).toString(QStringLiteral("-"));

    info.prescribedDose = readDoubleValue(info.summary, {"prescribed_dose_gy"});
    info.totalFractions = readIntValue(info.summary, {"total_fractions"});
    info.prescriptionText = gyText(info.prescribedDose);
    if (info.prescribedDose >= 0.0 && info.totalFractions > 0) {
        info.prescriptionText = QStringLiteral("%1 / %2 次").arg(info.prescriptionText).arg(info.totalFractions);
    }

    if (info.summary.contains(QStringLiteral("dose_per_fraction_gy"))) {
        info.fractionDose = readDoubleValue(info.summary, {"dose_per_fraction_gy"});
    } else {
        info.fractionDose = fractionBeamDoseGy(info.summary);
    }

    info.completedDose = readDoubleValue(payload, {"completed_dose_gy"});

    info.beams = beamsWithPlanLayers(
        info.summary.value(QStringLiteral("beams")).toArray(),
        payload);

    // TASK_PLAN_CONFIRM / TASK_SYNC_WORKFLOW_STATE uses target_mu.
    info.plannedMu = readDoubleValue(payload, {"target_mu", "planned_mu"});
    if (info.plannedMu < 0.0) {
        double beamMetersetSum = 0.0;
        bool hasBeamMeterset = false;
        for (const QJsonValue &beamValue : info.beams) {
            const double beamMeterset = readDoubleValue(
                beamValue.toObject(), {"beam_meterset"});
            if (beamMeterset >= 0.0) {
                beamMetersetSum += beamMeterset;
                hasBeamMeterset = true;
            }
        }
        if (hasBeamMeterset) {
            info.plannedMu = beamMetersetSum;
        }
    }

    return info;
}

enum class StatusTone
{
    Neutral,
    Information,
    Success,
    Warning,
    Error
};



void setStatusPill(QLabel *label,
                   const QString &deviceName,
                   const QString &status,
                   StatusTone tone,
                   const QString &tooltip = QString())
{
    if (!label) {
        return;
    }

    QString displayStatus = status;
    QString connectionText = QObject::tr("等待连接状态");
    QString ledColor = QStringLiteral("#98A2B3");
    if (displayStatus.startsWith(QObject::tr("已连接 / "))) {
        displayStatus.remove(0, QObject::tr("已连接 / ").size());
        connectionText = QObject::tr("已连接");
        ledColor = QStringLiteral("#20B26B");
    } else if (displayStatus == QObject::tr("未连接")) {
        connectionText = QObject::tr("未连接");
        ledColor = QStringLiteral("#D9534F");
    }

    label->setTextFormat(Qt::RichText);
    label->setText(QStringLiteral(
        "<span style=\"color:%1; font-size:15px;\">&#9679;</span>"
        "&nbsp;%2：%3")
        .arg(ledColor,
             deviceName.toHtmlEscaped(),
             displayStatus.toHtmlEscaped()));
    const QString connectionTooltip =
        QObject::tr("连接状态：%1").arg(connectionText);
    label->setToolTip(
        tooltip.isEmpty()
            ? QStringLiteral("%1\n%2")
                  .arg(connectionTooltip,
                       QObject::tr("设备状态：%1").arg(displayStatus))
            : QStringLiteral("%1\n%2").arg(connectionTooltip, tooltip));
    label->setProperty("statusTone", static_cast<int>(tone));
    label->setProperty("deviceName", deviceName);
    label->setProperty("deviceState", displayStatus);
    label->setProperty("deviceConnection", connectionText);
    if (label->property("clinicalDevice").toBool()) {
        label->setTextFormat(Qt::PlainText);
        label->setText(deviceName + QStringLiteral(" · ") + displayStatus
                       + (label->property("clinicalCompact").toBool() ? QString()
                           : QLatin1Char('\n') + connectionText));
    }
    setTreatmentRole(label, QStringLiteral("deviceStatus"));
}

void setStatusDot(QLabel *label,
                  StatusTone tone,
                  const QString &tooltip)
{
    if (!label) {
        return;
    }

    label->setProperty("statusTone", static_cast<int>(tone));
    setTreatmentRole(label, QStringLiteral("statusDot"));
    label->setToolTip(tooltip);
}

QJsonObject firstObjectValue(const QJsonObject &object,
                             const QStringList &keys)
{
    for (const QString &key : keys) {
        const QJsonObject value = object.value(key).toObject();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return {};
}

bool firstBoolValue(const QJsonObject &object,
                    const QStringList &keys,
                    bool fallback = false)
{
    for (const QString &key : keys) {
        // Accept only binary values recognized by the shared telemetry decoder
        // (JSON bool and supported 0/1 representations). Unknown stays unknown.
        const QJsonValue value = TelemetryValue::binaryFlag(object.value(key));
        if (value.isBool()) {
            return value.toBool();
        }
    }
    return fallback;
}

bool tryBoolValue(const QJsonObject &object,
                  const QStringList &keys,
                  bool *result)
{
    for (const QString &key : keys) {
        const QJsonValue value = TelemetryValue::binaryFlag(object.value(key));
        if (value.isBool()) {
            if (result) {
                *result = value.toBool();
            }
            return true;
        }
    }
    return false;
}

QString lowerCamelFromSnake(const QString &snakeName)
{
    const QStringList parts = snakeName.split(QLatin1Char('_'));
    if (parts.isEmpty()) {
        return snakeName;
    }

    QString camelName = parts.first();
    for (qsizetype index = 1; index < parts.size(); ++index) {
        QString part = parts.at(index);
        if (!part.isEmpty()) {
            part[0] = part.at(0).toUpper();
            camelName += part;
        }
    }
    return camelName;
}

QString normalizeRoomId(const QJsonValue &value)
{
    QString text;
    if (value.isString()) {
        text = value.toString().trimmed();
    } else if (value.isDouble()) {
        const double raw = value.toDouble();
        const int number = value.toInt(-1);
        if (number < 0 || raw != number) return QString();
        text = QString::number(number);
    } else {
        return QString();
    }
    if (text.isEmpty()) return QString();
    static const QRegularExpression prefixed(
        QStringLiteral("^ROOM_(\\d+)$"), QRegularExpression::CaseInsensitiveOption);
    const auto match = prefixed.match(text);
    if (match.hasMatch()) text = match.captured(1);
    bool ok = false;
    const int number = text.toInt(&ok);
    return ok && number >= 0 ? QStringLiteral("ROOM_%1").arg(number) : QString();
}

bool telemetryRoomMatches(const QJsonObject &outer, const QJsonObject &inner,
                          const QString &currentRoomId)
{
    const QString expected = normalizeRoomId(QJsonValue(currentRoomId));
    for (const QJsonObject &scope : {outer, inner}) {
        if (!scope.contains(QStringLiteral("room_id"))) continue;
        const QJsonValue room = scope.value(QStringLiteral("room_id"));
        // Preserve the existing empty-string broadcast convention.
        if (room.isString() && room.toString().trimmed().isEmpty()) continue;
        const QString actual = normalizeRoomId(room);
        if (actual.isEmpty() || expected.isEmpty() || actual != expected) return false;
    }
    return true;
}

} // namespace

TreatmentWindow::TreatmentWindow(WebSocketClient *webSocketClient,
                 const QString &username,
                 const QString &authToken,
                 const QJsonObject &userInfo,
                 const QJsonObject &loginResponse,
                 const QString &loginPassword,
                 QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TreatmentWindow)
    , m_settings(new QSettings(appConfigPath(), QSettings::IniFormat, this))
    , m_webSocketClient(webSocketClient)
    , m_currentUsername(username)
    , m_loginPassword(loginPassword)
    , m_authToken(authToken)
    , m_userInfo(userInfo)
{
    ui->setupUi(this);

    m_startupState.authenticated = !username.trimmed().isEmpty()
        && loginResponse.value(QStringLiteral("type")) == QStringLiteral("TASK")
        && loginResponse.value(QStringLiteral("cmd")) == QStringLiteral("TASK_LOGIN_SUCCESS");

    initUiLayoutTuning();
    setWindowState(windowState() | Qt::WindowMaximized);
    initTables();
    const QJsonObject payload = loginResponse.value("payload").toObject();
    const QString startupRoomId = normalizeRoomId(payload.value(QStringLiteral("room_id")));
    m_roomId = startupRoomId.isEmpty()
        ? m_settings->value(QStringLiteral("Treatment/RoomId")).toString().trimmed()
        : startupRoomId;
    m_activeBeamRoomId = payload.value("active_beam_room_id").toString(
        payload.value("beam_room_id").toString());

    QJsonArray entries = payload.value("entries").toArray(
        payload.value("data").toArray());

    if (entries.isEmpty()) {
        entries = loginResponse.value("entries").toArray(
            loginResponse.value("data").toArray());
    }

    updateTodaySchedule(entries);
    initStatusIndicators();
    loadMiddlewareSettings();

    m_bedController = new BedController(this);

    initTheme();
    setupBeamRoomOverview();
    initBedView();
    setupDoseInfoPanel();
    updateDdsInterlock(ui->groupBoxSystemStatus, QJsonObject());
    auto *gantryRow = new QHBoxLayout;
    gantryRow->setSpacing(16);
    for (const QString &gantry : {QStringLiteral("g01"), QStringLiteral("g02")}) {
        auto *lamp = new QLabel;
        lamp->setObjectName(gantry + QStringLiteral("HomedLamp"));
        lamp->setFixedSize(14, 14);
        auto *text = new QLabel;
        text->setObjectName(gantry + QStringLiteral("HomedText"));
        gantryRow->addWidget(lamp);
        gantryRow->addWidget(text);
    }
    auto *acceleratorLamp = new QLabel;
    acceleratorLamp->setObjectName(QStringLiteral("acceleratorReadyLamp"));
    acceleratorLamp->setFixedSize(14, 14);
    auto *acceleratorText = new QLabel;
    acceleratorText->setObjectName(QStringLiteral("acceleratorReadyText"));
    gantryRow->addWidget(acceleratorLamp);
    gantryRow->addWidget(acceleratorText);
    auto *faultLamp = new QLabel;
    faultLamp->setObjectName(QStringLiteral("ddsFaultLamp"));
    faultLamp->setFixedSize(14, 14);
    auto *faultText = new QLabel;
    faultText->setObjectName(QStringLiteral("ddsFaultText"));
    gantryRow->addWidget(faultLamp);
    gantryRow->addWidget(faultText);
    gantryRow->addStretch(1);
    ui->gridLayout_deviceStatus->addLayout(gantryRow, 2, 0, 1, 5);
    updateGantryIndicators(this, QJsonObject());
    updateDoseInfoPanel();
    initStepView();
    setTreatmentState(TreatmentState::Idle, "init");
    updateUserInfoUi();
    updateBeamRoomOverview(m_activeBeamRoomId);
    initClock();
    setupWorkflowRecovery();
    setupLogoutFlow();
    setupClinicalWorkspace();
    if (!m_webSocketClient) m_webSocketClient = new WebSocketClient(this);
    applyTreatmentStateToUi();
    initWebSocket();
    initConnectionState();
    QTimer::singleShot(0, this, [this] {
        if (m_workflowSyncRequired) queryWorkflowState();
    });

    onThemeChanged(ThemeManager::instance().currentTheme());

    updateAuditContext();
    QJsonObject startupPayload;
    startupPayload["room_id"] = m_roomId;
    startupPayload["schedule_count"] = entries.size();
    auditTreatmentEvent(QStringLiteral("session"),
                        QStringLiteral("treatment_client_started"),
                        QStringLiteral("client"),
                        QStringLiteral("treatment"),
                        QStringLiteral("success"),
                        tr("治疗客户端已启动"),
                        startupPayload);
}

void TreatmentWindow::setupLogoutFlow()
{
    m_logoutTimer = new QTimer(this);
    m_logoutTimer->setSingleShot(true);
    connect(m_logoutTimer, &QTimer::timeout, this, [this] {
        if (m_logoutState.phase != LogoutState::Phase::Waiting) return;
        m_logoutState.phase = LogoutState::Phase::Uncertain;
        m_closeAfterLogout = false;
        applyTreatmentStateToUi();
        statusBar()->showMessage(tr("注销结果待核实：未收到中间层确认，不会自动重发；请联系现场人员核实会话和 OIS 记录。"));
        auditTreatmentEvent("auth", "logout_timeout", "request", m_logoutState.requestId, "warning");
    });
    connect(m_clockTimer, &QTimer::timeout, this, &TreatmentWindow::updateSessionUi);
}

bool TreatmentWindow::logoutBeamSafe() const
{
    const bool beamActive = m_treatmentState == TreatmentState::Treating
        || m_backendWorkflowState == QStringLiteral("TREATMENT_IN_PROGRESS");
    return m_logoutState.beamSafe(m_webSocketClient && m_webSocketClient->isConnected(),
        !m_workflowSyncRequired && !m_resetPending, beamActive,
        m_settings->value(QStringLiteral("Logout/BeamStatusMaxAgeMs"), 5000).toLongLong());
}

QString TreatmentWindow::logoutBlockReason() const
{
    if (m_logoutState.phase == LogoutState::Phase::Waiting) return tr("正在注销，等待中间层与 OIS 确认");
    if (m_logoutState.phase == LogoutState::Phase::Uncertain)
        return tr("注销结果待核实：请求无事务号，不能重复发送；请联系现场人员核实会话和 OIS 记录");
    const qint64 maxAgeMs = m_settings->value(QStringLiteral("Logout/BeamStatusMaxAgeMs"), 5000).toLongLong();
    if (m_treatmentState == TreatmentState::Treating
        || m_backendWorkflowState == QStringLiteral("TREATMENT_IN_PROGRESS")
        || (m_logoutState.deliveryFresh(maxAgeMs) && m_logoutState.deliveryResult == 1)
        || (m_logoutState.liveBeam.isBool() && m_logoutState.liveBeam.toBool()
            && m_logoutState.beamAge.isValid() && m_logoutState.beamAge.elapsed() < maxAgeMs))
        return tr("正在出束，禁止注销；注销不是停束操作");
    if (m_logoutState.deliveryFresh(maxAgeMs) && m_logoutState.deliveryResult == 7)
        return tr("出束故障，实际停束状态未确认；请核实现场，不能仅凭故障结果注销");
    if (!logoutBeamSafe()) return tr("连接、现场或实时出束结果未确认，请同步并等待有效的 0/2 回读");
    if (!m_settings->value(QStringLiteral("Logout/ProtocolEnabled"), true).toBool())
        return tr("注销接口尚未接通：请先完成中间层及 OIS 对接，不能仅断开连接作为注销");
    return QString();
}

bool TreatmentWindow::handleLogoutReply(const QJsonObject &message)
{
    if (message.value("type") != QStringLiteral("TASK")
        || message.value("cmd") != QStringLiteral("TASK_LOGOUT_SUCCESS")) return false;
    const auto result = m_logoutState.consume(message);
    if (result == LogoutState::Reply::Ignored) return true;
    m_logoutTimer->stop();
    if (result == LogoutState::Reply::Completed) {
        completeLogout(true);
        return true;
    }
    m_closeAfterLogout = false;
    applyTreatmentStateToUi();
    if (result == LogoutState::Reply::Rejected) {
        const QString reason = m_logoutState.message.isEmpty() ? tr("后台未提供原因") : m_logoutState.message;
        statusBar()->showMessage(tr("注销失败：%1；当前会话未清理").arg(reason), 8000);
        auditTreatmentEvent("auth", "logout_rejected", "request", m_logoutState.requestId, "failed", reason);
    } else {
        statusBar()->showMessage(tr("注销回执状态不明确，本机会话未清理；请联系现场人员核实。"));
        auditTreatmentEvent("auth", "logout_incomplete_reply", "request", m_logoutState.requestId, "warning");
    }
    return true;
}

bool TreatmentWindow::sessionReady() const
{
    return m_startupState.canOperate() && !m_loginTransitionPending && !m_workflowReauthPending
        && !m_logoutState.busy() && !m_logoutConfirming;
}

void TreatmentWindow::updateSessionUi()
{
    const bool allowed = sessionReady();
    // Restore ordinary UI state first; workflow-specific controls are gated again below.
    for (QAbstractButton *button : ui->centralwidget->findChildren<QAbstractButton *>()) {
        if (button == ui->logout_pushButton) continue;
        if (button->property("sessionReadOnly").toBool()
            || qobject_cast<QTabBar *>(button->parentWidget())) {
            button->setEnabled(true);
            button->setProperty("sessionEnabled", QVariant());
            continue;
        }
        if (!allowed) {
            if (!button->property("sessionEnabled").isValid())
                button->setProperty("sessionEnabled", button->isEnabled());
            button->setEnabled(false);
        } else if (button->property("sessionEnabled").isValid()) {
            button->setEnabled(button->property("sessionEnabled").toBool());
            button->setProperty("sessionEnabled", QVariant());
        }
    }
    for (QLineEdit *edit : {ui->lineEdit_a, ui->lineEdit_b, ui->lineEdit_c,
                            ui->lineEdit_x, ui->lineEdit_y, ui->lineEdit_z}) edit->setEnabled(allowed);
    for (QTabWidget *tabs : ui->centralwidget->findChildren<QTabWidget *>())
        tabs->tabBar()->setEnabled(true);
    ui->logout_pushButton->setText(m_startupState.authenticated ? tr("注销") : tr("登录"));
    if (m_startupState.authenticated && m_logoutState.busy())
        ui->logout_pushButton->setText(m_logoutState.phase == LogoutState::Phase::Waiting
            ? tr("注销中…") : tr("注销待核实"));
    ui->logout_pushButton->setIcon(QIcon(m_startupState.authenticated
        ? QStringLiteral(":/icons/icons/logout.svg") : QStringLiteral(":/icons/icons/user.svg")));
    const QString logoutReason = m_startupState.authenticated ? logoutBlockReason() : QString();
    ui->logout_pushButton->setToolTip(logoutReason);
    ui->logout_pushButton->setEnabled(!m_closing && !m_loginTransitionPending
        && !m_logoutConfirming && logoutReason.isEmpty());
    if (auto *supply = findChild<QLabel *>(QStringLiteral("labelBeamDeliveryState"))) {
        const qint64 maxAgeMs = m_settings->value(QStringLiteral("Logout/BeamStatusMaxAgeMs"), 5000).toLongLong();
        QString text = m_workflowSyncRequired ? tr("供束：未知 | 现场待同步")
            : liveBeamDeliveryText(m_latestTelemetryPayload, m_logoutState, maxAgeMs);
        if (m_clinicalBody) text.replace(QStringLiteral(" | "), QStringLiteral("\n"));
        setLabelTextIfChanged(supply, text);
    }
    if (m_initializationLabel) {
        m_initializationLabel->setVisible(!m_startupState.initialized);
        m_initializationLabel->setText(!m_startupState.failure.isEmpty()
            ? tr("初始化失败：%1").arg(m_startupState.failure)
            : tr("正在初始化 · 等待后台完成通知"));
        m_initializationLabel->setToolTip(m_settings->value(
            QStringLiteral("Startup/InitializationCompleteCommand")).toString().trimmed().isEmpty()
                ? tr("等待 TASK_INITIALIZATION_COMPLETED；连接成功、登录成功或收到快照均不能代替初始化完成。")
                : QString());
    }
}

void TreatmentWindow::handleInitializationNotification(const QJsonObject &message)
{
    if (!m_webSocketClient || !m_webSocketClient->isConnected()) return;
    const auto result = m_startupState.observe(message,
        m_settings->value(QStringLiteral("Startup/InitializationCompleteCommand")).toString().trimmed(),
        m_settings->value(QStringLiteral("Startup/InitializationFailedCommand")).toString().trimmed(), m_roomId);
    if (result == StartupState::Notification::Ignored) return;
    if (result == StartupState::Notification::Failed) {
        if (m_startupState.failure.isEmpty()) m_startupState.failure = tr("后台未提供具体原因");
        ++m_workflowRevision;
        rejectWorkflowDialogs(this);
        if (m_todayScheduleDialog) m_todayScheduleDialog->close();
    } else if (loginDialog) {
        loginDialog->setQaModeAvailable(true);
    }
    applyTreatmentStateToUi();
    if (result == StartupState::Notification::Ready) {
        statusBar()->showMessage(tr("后台初始化已完成"), 4000);
        restorePendingWorkflowMessages();
        if (!m_deferredPlanConfirmation.isEmpty() && sessionReady()) {
            const QJsonObject pending = m_deferredPlanConfirmation;
            m_deferredPlanConfirmation = QJsonObject();
            handleTaskMessage(pending);
        }
        if (m_workflowSyncRequired) QTimer::singleShot(0, this, [this] {
            if (m_workflowSyncRequired) queryWorkflowState();
        });
        else scheduleWorkflowConfirmation();
    }
}

TreatmentWindow::~TreatmentWindow()
{
    if (loginDialog) {
        disconnect(loginDialog, nullptr, this, nullptr);
        loginDialog->reject();
    }
    auditTreatmentEvent(QStringLiteral("session"),
                        QStringLiteral("treatment_client_closed"),
                        QStringLiteral("client"),
                        QStringLiteral("treatment"),
                        QStringLiteral("success"),
                        tr("治疗客户端已关闭"));

    delete ui;
}

void TreatmentWindow::closeEvent(QCloseEvent *event)
{
    if (m_startupState.authenticated && !m_closing) {
        event->ignore();
        m_closeAfterLogout = true;
        on_logout_pushButton_clicked();
        return;
    }
    QMainWindow::closeEvent(event);
    if (!event->isAccepted() || m_closing) return;

    m_closing = true;
    ++m_workflowRevision;
    m_loginTransitionPending = false;
    m_workflowReauthPending = true;
    m_workflowSyncRequired = true;
    m_resetPending = false;
    if (m_workflowTimer) m_workflowTimer->stop();
    if (m_clockTimer) m_clockTimer->stop();
    if (loginDialog) loginDialog->clearPassword();
    rejectWorkflowDialogs(this);
    if (m_todayScheduleDialog) m_todayScheduleDialog->close();
    if (m_webSocketClient) {
        m_webSocketClient->setAutoReconnect(false);
        disconnect(m_webSocketClient, nullptr, this, nullptr);
        m_webSocketClient->disconnectFromServer();
    }
    m_loginPassword.fill(QChar(u'\0'));
    m_loginPassword.clear();

    // Closing a hidden login owner does not reliably emit lastWindowClosed.
    QCoreApplication::quit();
}

void TreatmentWindow::setupWorkflowRecovery()
{
    m_queryWorkflowButton = new QPushButton(ui->groupBoxTopActions);
    m_queryWorkflowButton->setObjectName(QStringLiteral("queryWorkflowButton"));
    m_queryWorkflowButton->setAccessibleName(tr("同步现场"));
    m_queryWorkflowButton->setFixedSize(42, 42);
    m_queryWorkflowButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_queryWorkflowButton->setToolTip(tr("同步现场：查询后端现场快照，不发送治疗操作"));
    ui->horizontalLayout->insertWidget(1, m_queryWorkflowButton);
    ui->horizontalLayout->setSpacing(8);
    ui->todays_pushButton->setMinimumWidth(120);
    ui->theme_pushButton->setMinimumWidth(120);
    ui->logout_pushButton->setMinimumWidth(90);
    m_resetWorkflowButton = new QPushButton(ui->groupBoxTopActions);
    m_resetWorkflowButton->setObjectName(QStringLiteral("resetWorkflowButton"));
    m_resetWorkflowButton->setAccessibleName(tr("重置流程（测试）"));
    m_resetWorkflowButton->setFixedSize(42, 42);
    m_resetWorkflowButton->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));
    m_resetWorkflowButton->setToolTip(tr("重置流程（测试）：请求后端强制重置；不是急停或停束按钮"));
    ui->horizontalLayout->insertWidget(2, m_resetWorkflowButton);
    ui->todays_pushButton->setMinimumWidth(100);
    ui->theme_pushButton->setMinimumWidth(100);
    ui->logout_pushButton->setMinimumWidth(70);
    connect(m_resetWorkflowButton, &QPushButton::clicked, this, &TreatmentWindow::requestWorkflowReset);
    connect(m_queryWorkflowButton, &QPushButton::clicked, this, &TreatmentWindow::queryWorkflowState);
    m_workflowTimer = new QTimer(this);
    m_workflowTimer->setSingleShot(true);
    m_workflowTimer->setInterval(15000);
    connect(m_workflowTimer, &QTimer::timeout, this, [this] {
        const bool reset = m_resetPending;
        m_resetPending = false;
        m_workflowSyncRequired = true;
        applyTreatmentStateToUi();
        statusBar()->showMessage(reset
            ? tr("未收到重置确认，未清理本地现场；请同步现场核实，勿假定设备已停止")
            : tr("现场同步超时，操作已锁定；请检查连接后重新同步"));
        auditTreatmentEvent(QStringLiteral("workflow"),
            reset ? QStringLiteral("workflow_reset_timeout") : QStringLiteral("workflow_state_query_timeout"),
            QStringLiteral("room"), m_roomId, QStringLiteral("failed"),
            reset ? tr("后端未确认重置") : tr("未收到有效现场快照"));
    });
    applyTreatmentStateToUi();
}

void TreatmentWindow::queryWorkflowState()
{
    if (!m_workflowTimer || m_workflowTimer->isActive() || m_resetPending
        || m_workflowReauthPending || !m_webSocketClient || !m_webSocketClient->isConnected()) return;
    if (!m_startupState.authenticated && !m_settings->value(
            QStringLiteral("Startup/AllowAnonymousSnapshotQuery"), false).toBool()) return;
    m_workflowSyncRequired = true;
    applyTreatmentStateToUi();
    statusBar()->showMessage(tr("正在查询后端现场，治疗操作暂时锁定"));
    QJsonObject request;
    request.insert(QStringLiteral("cmd"), QStringLiteral("QUERY_WORKFLOW_STATE"));
    m_workflowTimer->start();
    m_webSocketClient->sendData(request);
    auditTreatmentEvent(QStringLiteral("workflow"), QStringLiteral("workflow_state_query_sent"),
        QStringLiteral("room"), m_roomId, QStringLiteral("attempt"), tr("已发送现场快照查询，等待后端返回"));
}

void TreatmentWindow::restorePendingWorkflowMessages()
{
    if (!m_startupState.authenticated || !m_webSocketClient || !m_webSocketClient->isConnected()) return;
    const QList<QJsonObject> pending = m_webSocketClient->takePendingWorkflowMessages();
    for (const QJsonObject &message : pending) {
        if (m_workflowReauthPending) break;
        handleInitializationNotification(message);
        handleTaskMessage(message);
    }
}

void TreatmentWindow::requestWorkflowReset()
{
    // Backend recovery must remain available even when local workflow sync has failed.
    const auto canSendReset = [this] {
        return sessionReady() && m_webSocketClient && m_webSocketClient->isConnected()
            && !m_resetPending && !m_workflowReauthPending;
    };
    if (!canSendReset()) {
        QMessageBox::warning(this, tr("暂不能重置"),
            m_resetPending ? tr("重置请求已发送，请等待后端确认。")
                          : tr("请先登录并连接后端，再请求重置流程。"));
        return;
    }
    const quint64 revision = m_workflowRevision;
    const auto answer = QMessageBox::warning(this, tr("确认重置流程"),
        tr("确定请求后端强制重置当前治疗流程吗？\n"
           "收到后端重置通知后将清空本次界面并重新登录。\n"
           "此操作不是急停，不能据此判断设备或束流已停止。"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes || revision != m_workflowRevision || !canSendReset()) return;
    m_resetPending = true;
    applyTreatmentStateToUi();
    statusBar()->showMessage(tr("已请求重置，等待后端确认；本地现场尚未清除"));
    QJsonObject request;
    request.insert(QStringLiteral("cmd"), QStringLiteral("FORCE_RESET"));
    m_workflowTimer->start();
    m_webSocketClient->sendData(request);
    auditTreatmentEvent(QStringLiteral("workflow"), QStringLiteral("force_reset_requested"),
        QStringLiteral("room"), m_roomId, QStringLiteral("attempt"), tr("请求后端强制重置流程"));
}

void TreatmentWindow::handleWorkflowStateSync(const QJsonObject &snapshot)
{
    if (m_closing || m_resetPending || m_workflowReauthPending) return;
    const QString room = normalizeRoomId(snapshot.value(QStringLiteral("room_id")));
    QJsonObject syncDetails;
    syncDetails.insert(QStringLiteral("snapshot_room_id"), room);
    syncDetails.insert(QStringLiteral("snapshot_state"), snapshot.value(QStringLiteral("state")));
    auditTreatmentEvent(QStringLiteral("workflow"), QStringLiteral("workflow_snapshot_received"),
        QStringLiteral("room"), m_roomId, QStringLiteral("info"), tr("已收到后端现场快照"), syncDetails);
    const QString currentRoom = normalizeRoomId(QJsonValue(m_roomId));
    if (room.isEmpty() || currentRoom.isEmpty() || room != currentRoom) {
        m_workflowSyncRequired = true;
        applyTreatmentStateToUi();
        statusBar()->showMessage(tr("快照治疗室缺失或不匹配，拒绝恢复；请检查配置"));
        auditTreatmentEvent(QStringLiteral("workflow"), QStringLiteral("workflow_snapshot_rejected"),
            QStringLiteral("room"), m_roomId, QStringLiteral("failed"), tr("快照治疗室缺失或不匹配"), syncDetails);
        return;
    }
    const QString state = snapshot.value(QStringLiteral("state")).toString();
    const QMap<QString, TreatmentState> states = {
        {QStringLiteral("WAITING_LOGIN"), TreatmentState::Idle},
        {QStringLiteral("WAITING_OIS_SCHEDULE_CONFIRM"), TreatmentState::Idle},
        {QStringLiteral("WAITING_FRONTEND_PLAN_CONFIRM"), TreatmentState::PlanConfirm},
        {QStringLiteral("WAITING_DEVICE_PLAN_ACK"), TreatmentState::Preparing},
        {QStringLiteral("WAITING_DEVICE_AUTOPOS_DONE"), TreatmentState::Preparing},
        {QStringLiteral("WAITING_FRONTEND_BEAM_APPLY"), TreatmentState::ReadyToApplyBeam},
        {QStringLiteral("WAITING_DEVICE_BEAM_READY"), TreatmentState::BeamRequested},
        {QStringLiteral("TREATMENT_IN_PROGRESS"), TreatmentState::Treating},
        {QStringLiteral("TREATMENT_PAUSED"), TreatmentState::Paused},
        {QStringLiteral("WAITING_TREATMENT_END"), TreatmentState::BeamFinished},
        {QStringLiteral("ERROR_STOPPED"), TreatmentState::ErrorStopped},
        {QStringLiteral("WAITING_CHART_GENERATION"), TreatmentState::BeamFinished}
    };
    const bool stateKnown = states.contains(state);
    if (!m_workflowSyncRequired && snapshot == m_lastWorkflowSnapshot) return;
    // Display data and permission checks are separate: unknown states still have useful data.
    const bool sameConfirmation = !m_workflowSyncRequired
        && (m_planConfirmationPending || m_workflowConfirmationPending || m_credentialDialog)
        && (m_treatmentState == TreatmentState::PlanConfirm
            || m_treatmentState == TreatmentState::ReadyToApplyBeam)
        && stateKnown && states.value(state) == m_treatmentState
        && !snapshot.value("plan_id").toString().isEmpty()
        && snapshot.value("plan_id") == m_currentPlanPayload.value("plan_id")
        && snapshot.value("fraction_id") == m_currentPlanPayload.value("fraction_id")
        && snapshot.value("patient_id") == m_currentPlanPayload.value("patient_id");
    if (sameConfirmation) return; // An identical late snapshot must not close the live confirmation.
    m_lastWorkflowSnapshot = snapshot;
    m_workflowTimer->stop();
    ++m_workflowRevision;
    rejectWorkflowDialogs(this, loginDialog);
    if (state == QStringLiteral("WAITING_LOGIN")) {
        m_deferredPlanConfirmation = QJsonObject();
        if (m_logoutState.busy()) {
            // Session-state broadcasts are not proof that OIS saved this logout transaction.
            m_backendWorkflowState = state;
            m_treatmentState = TreatmentState::Idle;
            m_workflowSyncRequired = false;
            applyTreatmentStateToUi();
            return;
        }
        if (!m_startupState.authenticated) {
            m_currentPlanPayload = QJsonObject();
            m_backendWorkflowState = state;
            m_treatmentState = TreatmentState::Idle;
            m_workflowSyncRequired = false;
            ui->spotMapWidget->clearPlan();
            ui->patient_id->setText(tr("ID：-"));
            ui->patient_name->setText(tr("姓名：-"));
            for (QLabel *label : {ui->labelPlanIdValue,
                    ui->labelPlanFractionIdValue, ui->labelPlanSiteTargetValue,
                    ui->labelPlanIsocenterValue, ui->labelPlanQaReleaseValue})
                label->setText(QStringLiteral("-"));
            updateDoseInfoPanel();
            applyTreatmentStateToUi();
            return;
        }
        auditTreatmentEvent(QStringLiteral("workflow"), QStringLiteral("workflow_snapshot_waiting_login"),
            QStringLiteral("room"), room, QStringLiteral("warning"), tr("后端现场为等待登录，没有可恢复的活动治疗现场"));
        handleWorkflowReset();
        return;
    }

    // Replace, never merge, patient/plan context from a previous session.
    const bool contextChanged = snapshot.value("plan_id") != m_currentPlanPayload.value("plan_id")
        || snapshot.value("fraction_id") != m_currentPlanPayload.value("fraction_id")
        || snapshot.value("patient_id") != m_currentPlanPayload.value("patient_id");
    if (contextChanged) {
        clearGrpcCallDisplay();
        m_logoutState.invalidateBeam();
    }
    m_currentPlanPayload = snapshot;
    if (snapshot.value("target_mu").isDouble())
        m_currentPlanPayload.insert("planned_mu", snapshot.value("target_mu"));
    m_latestTelemetryPayload = QJsonObject();
    m_lastDoseTelemetrySnapshot.clear();
    m_currentDeviceActionText = tr("现场已恢复，等待设备动作反馈");
    if (m_bedView) {
        m_bedView->show();
        m_bedView->setToolTip(tr("等待位置反馈；三维模型仅供示意，不代表已确认的实机位置。"));
    }
    for (QLabel *label : {ui->label_posa, ui->label_posb, ui->label_posc,
                          ui->label_posx, ui->label_posy, ui->label_posz})
        label->setText(QStringLiteral("-"));
    m_currentDeviceIssues.clear();
    m_activeBeamRoomId.clear();
    // Repeated error snapshots must not make ErrorStopped its own previous state.
    if (stateKnown) {
        if (m_treatmentState != TreatmentState::ErrorStopped)
            m_previousTreatmentState = m_treatmentState;
        m_treatmentState = states.value(state);
    }
    m_backendWorkflowState = state;
    if (stateKnown) m_deferredPlanConfirmation = QJsonObject();
    m_workflowSyncRequired = !stateKnown;
    const PlanDisplayInfo plan = parsePlanDisplayInfo(m_currentPlanPayload);
    ui->patient_id->setText(tr("ID：%1").arg(snapshot.value("patient_id").toString(plan.patientId)));
    ui->patient_name->setText(tr("姓名：%1").arg(snapshot.value("patient_name").toString(plan.patientName)));
    ui->patient_id->setToolTip(tr("病历号：%1").arg(snapshot.value("patient_mrn").toString(QStringLiteral("-"))));
    ui->labelPlanIdValue->setText(plan.planId);
    ui->labelPlanFractionIdValue->setText(plan.fractionId);
    ui->labelPlanSiteTargetValue->setText(plan.targetArea);
    ui->labelPlanIsocenterValue->setText(plan.isocenterText);
    ui->labelPlanQaReleaseValue->setText(tr("%1 / %2").arg(plan.qaStatus, plan.releaseStatus));
    ui->spotMapWidget->setPlanPayload(m_currentPlanPayload);
    initStatusIndicators();
    m_workflowErrorMessage.clear();
    if (m_treatmentState == TreatmentState::ErrorStopped) {
        const QString reason = snapshot.value(QStringLiteral("error_message")).toString(
            snapshot.value(QStringLiteral("message")).toString()).trimmed();
        m_workflowErrorMessage = reason.isEmpty() ? tr("后端未提供异常原因") : reason;
    }
    QJsonObject telemetry;
    for (const QString &key : {QStringLiteral("current_layer"), QStringLiteral("energy"), QStringLiteral("dose_rate"),
                              QStringLiteral("target_mu"), QStringLiteral("current_delivered_mu"),
                              QStringLiteral("cumulative_dose")}) {
        if (snapshot.contains(key)) telemetry.insert(key, snapshot.value(key));
    }
    telemetry.insert(QStringLiteral("workflow_mu_is_snapshot"), true);
    telemetry.insert(QStringLiteral("dose_rate_is_snapshot"), true);
    telemetry.insert(QStringLiteral("beam_on_is_snapshot"), true);
    if (snapshot.contains(QStringLiteral("cumulative_dose")))
        telemetry.insert(QStringLiteral("cumulative_dose_is_snapshot"), true);
    if (snapshot.value("is_beam_on").isBool()) telemetry.insert("beam_on", snapshot.value("is_beam_on"));
    if (m_logoutState.deliveryFresh(m_settings->value(QStringLiteral("Logout/BeamStatusMaxAgeMs"), 5000).toLongLong()))
        telemetry.insert(QStringLiteral("beam_delivery_result"), m_logoutState.deliveryResult);
    // current_delivered_mu is whole-treatment progress, not layer MU or Gy.
    m_latestTelemetryPayload = telemetry;
    ui->spotMapWidget->setTelemetry(telemetry);
    updateDoseInfoPanel(m_currentPlanPayload);
    updateDdsInterlock(ui->groupBoxSystemStatus, telemetry);
    updateGantryIndicators(this, telemetry);
    updateAuditContext();
    updateUserInfoUi();
    updateBeamRoomOverview();
    applyTreatmentStateToUi();
    statusBar()->showMessage(!stateKnown
        ? tr("快照数据已回填，后端状态尚未识别：%1；操作保持锁定").arg(state)
        : m_treatmentState == TreatmentState::ErrorStopped
            ? tr("现场已同步：后端流程异常中止，治疗操作保持锁定") : tr("现场已同步"), 4000);
    auditTreatmentEvent(QStringLiteral("workflow"),
        stateKnown ? QStringLiteral("workflow_state_synced") : QStringLiteral("workflow_snapshot_data_restored"),
        QStringLiteral("room"), room, stateKnown ? QStringLiteral("success") : QStringLiteral("warning"), state);
    // Restore only the human confirmation step; never replay an accepted command.
    if (stateKnown) scheduleWorkflowConfirmation();
}

void TreatmentWindow::scheduleWorkflowConfirmation()
{
    if (!sessionReady() || m_closing || m_workflowSyncRequired || m_resetPending || m_workflowReauthPending
        || m_loginTransitionPending || !m_webSocketClient || !m_webSocketClient->isConnected()) return;
    const TreatmentState expectedState = m_treatmentState;
    if (expectedState != TreatmentState::PlanConfirm
        && expectedState != TreatmentState::ReadyToApplyBeam) return;
    if (m_currentPlanPayload.value(QStringLiteral("plan_id")).toString().trimmed().isEmpty()) {
        statusBar()->showMessage(tr("已恢复待确认状态，但快照缺少计划ID，请同步完整计划"), 5000);
        return;
    }
    const quint64 revision = m_workflowRevision;
    if (m_workflowConfirmationPending && m_workflowConfirmationRevision == revision
        && m_workflowConfirmationState == expectedState) return;
    m_workflowConfirmationPending = true;
    m_workflowConfirmationRevision = revision;
    m_workflowConfirmationState = expectedState;
    m_planConfirmationPending = expectedState == TreatmentState::PlanConfirm;
    QTimer::singleShot(0, this, [this, revision, expectedState] {
        const auto ownsRequest = [this, revision, expectedState] {
            return m_workflowConfirmationPending && m_workflowConfirmationRevision == revision
                && m_workflowConfirmationState == expectedState;
        };
        if (!ownsRequest()) return;
        if (sessionReady() && !m_closing && revision == m_workflowRevision && m_treatmentState == expectedState
            && !m_workflowSyncRequired && !m_resetPending && !m_workflowReauthPending
            && !m_loginTransitionPending && m_webSocketClient && m_webSocketClient->isConnected()) {
            if (m_credentialDialog) activateDialog(m_credentialDialog);
            else if (expectedState == TreatmentState::PlanConfirm) on_confirmPlan_pushButton_clicked();
            else on_beamApply_pushButton_clicked();
        }
        if (ownsRequest()) {
            m_workflowConfirmationPending = false;
            m_planConfirmationPending = false;
        }
    });
}

void TreatmentWindow::handleWorkflowReset()
{
    m_deferredPlanConfirmation = QJsonObject();
    if (m_workflowReauthPending) return;
    if (m_logoutState.busy()) {
        m_logoutState.phase = LogoutState::Phase::Uncertain;
        m_closeAfterLogout = false;
        m_logoutTimer->stop();
        m_treatmentState = TreatmentState::Idle;
        m_backendWorkflowState = QStringLiteral("WAITING_LOGIN");
        m_workflowSyncRequired = true;
        applyTreatmentStateToUi();
        statusBar()->showMessage(tr("后台流程已重置，但注销与 OIS 记录结果尚未确认"));
        queryWorkflowState();
        return;
    }
    if (!m_startupState.authenticated) {
        m_workflowSyncRequired = true;
        applyTreatmentStateToUi();
        return;
    }
    ++m_workflowRevision;
    m_workflowTimer->stop();
    m_resetPending = false;
    m_workflowSyncRequired = true;
    m_workflowReauthPending = true;
    m_treatmentState = TreatmentState::Idle;
    m_previousTreatmentState = TreatmentState::Idle;
    m_currentDeviceIssues.clear();
    m_currentDeviceActionText = tr("流程已重置，等待登录");
    clearGrpcCallDisplay();
    applyTreatmentStateToUi();
    statusBar()->showMessage(tr("后端已通知流程重置，将退出当前登录"));
    auditTreatmentEvent(QStringLiteral("workflow"), QStringLiteral("workflow_reset_received"),
        QStringLiteral("room"), m_roomId, QStringLiteral("success"), tr("后端通知流程重置"));
    // Defer relogin until the current network callback and any open confirmation unwind.
    rejectWorkflowDialogs(this, loginDialog);
    QTimer::singleShot(0, this, [this] { completeLogout(false); });
}

void TreatmentWindow::initUiLayoutTuning()
{
    auto *oldAction = ui->labelCurrentTreatmentAction;
    auto *action = new WorkflowTextLabel(oldAction->parentWidget());
    action->setObjectName(oldAction->objectName());
    action->setWordWrap(true);
    action->setTextFormat(Qt::PlainText);
    action->setAlignment(oldAction->alignment());
    action->setText(oldAction->text());
    delete ui->verticalLayout_flowBox->replaceWidget(oldAction, action);
    ui->labelCurrentTreatmentAction = action;
    delete oldAction;
    ui->verticalLayout_flowBox->setSizeConstraint(QLayout::SetMinimumSize);
    resize(1600, 980);
    ui->verticalLayout_root->setSpacing(6);
    ui->horizontalLayout_topArea->setSpacing(8);

    // Keep logout wiring explicit because this action owns the client/session
    // handoff and must not depend solely on automatic slot-name discovery.
    disconnect(ui->logout_pushButton, &QPushButton::clicked,
               this, &TreatmentWindow::on_logout_pushButton_clicked);
    connect(ui->logout_pushButton, &QPushButton::clicked,
            this, &TreatmentWindow::on_logout_pushButton_clicked,
            Qt::UniqueConnection);

    for (QLabel *deviceStatus : {ui->labelCouchDeviceStatus,
                                 ui->labelDrDeviceStatus,
                                 ui->labelIgrtDeviceStatus,
                                 ui->labelFilterDeviceStatus,
                                 ui->labelPlcDeviceStatus}) {
        deviceStatus->setWordWrap(true);
        ui->gridLayout_deviceStatus->removeWidget(deviceStatus);
    }
    ui->gridLayout_deviceStatus->addWidget(ui->labelCouchDeviceStatus, 1, 0);
    ui->gridLayout_deviceStatus->addWidget(ui->labelDrDeviceStatus, 1, 1);
    ui->gridLayout_deviceStatus->addWidget(ui->labelIgrtDeviceStatus, 1, 2);
    ui->gridLayout_deviceStatus->addWidget(ui->labelFilterDeviceStatus, 1, 3);
    ui->gridLayout_deviceStatus->addWidget(ui->labelPlcDeviceStatus, 1, 4);
    for (int column = 0; column < 5; ++column) {
        ui->gridLayout_deviceStatus->setColumnStretch(column, 1);
    }
    ui->labelDeviceStatusCaption->setText(tr("设备状态"));
    ui->labelDeviceStatusCaption->show();

    auto *statusArea = new QWidget(this);
    statusArea->setObjectName(QStringLiteral("widgetStatusArea"));
    statusArea->setMinimumHeight(164);
    auto *statusAreaLayout = new QVBoxLayout(statusArea);
    statusAreaLayout->setContentsMargins(0, 0, 0, 0);
    statusAreaLayout->setSpacing(6);

    auto *deviceReadinessGroup = new QGroupBox(statusArea);
    deviceReadinessGroup->setObjectName(QStringLiteral("groupBoxDeviceReadiness"));
    auto *deviceReadinessLayout = new QVBoxLayout(deviceReadinessGroup);
    deviceReadinessLayout->setContentsMargins(10, 8, 10, 8);
    ui->verticalLayout_statusBox->removeItem(ui->gridLayout_deviceStatus);
    ui->gridLayout_deviceStatus->setParent(nullptr);
    deviceReadinessLayout->addLayout(ui->gridLayout_deviceStatus);
    ui->gridLayout_deviceStatus->removeWidget(ui->labelDeviceStatusCaption);
    ui->gridLayout_deviceStatus->addWidget(ui->labelDeviceStatusCaption, 0, 0, 1, 5);

    if (ui->mainGrid) {
        ui->mainGrid->setColumnStretch(0, 6);
        ui->mainGrid->setColumnStretch(1, 5);
        ui->mainGrid->setColumnStretch(2, 5);
        ui->mainGrid->setColumnStretch(3, 3);
        ui->mainGrid->setColumnStretch(4, 5);

        ui->mainGrid->setRowStretch(0, 1);
        ui->mainGrid->setRowStretch(1, 0);
        ui->mainGrid->setRowStretch(2, 0);
        ui->mainGrid->setRowStretch(3, 0);
        ui->mainGrid->setRowStretch(4, 0);
        ui->mainGrid->setRowMinimumHeight(1, 0);

        ui->mainGrid->setHorizontalSpacing(6);
        ui->mainGrid->setVerticalSpacing(6);
        ui->mainGrid->setContentsMargins(0, 0, 0, 0);

        ui->mainGrid->removeWidget(ui->groupBoxTreatmentFlow);
        ui->mainGrid->removeWidget(ui->groupBoxSystemStatus);
        ui->mainGrid->removeWidget(ui->groupBoxOperationQueue);
        ui->mainGrid->removeItem(ui->verticalLayout_preview);
        ui->mainGrid->removeWidget(ui->groupBoxBedPosition);
        ui->mainGrid->removeWidget(ui->groupBoxTreatmentPlan);
        ui->mainGrid->addWidget(ui->groupBoxTreatmentPlan, 0, 0);

        auto *treatmentColumn = new QWidget(this);
        treatmentColumn->setObjectName(QStringLiteral("widgetTreatmentColumn"));
        auto *treatmentColumnLayout = new QVBoxLayout(treatmentColumn);
        treatmentColumnLayout->setContentsMargins(0, 0, 0, 0);
        treatmentColumnLayout->setSpacing(6);
        treatmentColumnLayout->setSizeConstraint(QLayout::SetMinimumSize);
        treatmentColumnLayout->addWidget(ui->groupBoxTreatmentFlow, 0);
        auto *liveDoseGroup = new QGroupBox(treatmentColumn);
        liveDoseGroup->setObjectName(QStringLiteral("groupBoxLiveDose"));
        QSizePolicy liveDosePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        liveDosePolicy.setHeightForWidth(true);
        liveDoseGroup->setSizePolicy(liveDosePolicy);
        auto *liveDoseLayout = new QGridLayout(liveDoseGroup);
        liveDoseLayout->setObjectName(QStringLiteral("gridLayoutLiveDose"));
        liveDoseLayout->setContentsMargins(10, 8, 10, 10);
        liveDoseLayout->setHorizontalSpacing(8);
        liveDoseLayout->setVerticalSpacing(6);
        liveDoseLayout->setColumnStretch(0, 1);
        liveDoseLayout->setColumnStretch(1, 1);
        liveDoseLayout->setAlignment(Qt::AlignTop);
        liveDoseLayout->setSizeConstraint(QLayout::SetMinimumSize);
        treatmentColumnLayout->addWidget(liveDoseGroup, 0);
        statusAreaLayout->addWidget(ui->groupBoxSystemStatus, 1);
        statusAreaLayout->addWidget(deviceReadinessGroup, 1);
        statusArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        treatmentColumnLayout->addWidget(statusArea, 0);
        treatmentColumnLayout->addStretch(1);
        auto *treatmentScroll = new QScrollArea(this);
        treatmentScroll->setObjectName(QStringLiteral("scrollAreaTreatmentColumn"));
        treatmentScroll->setWidgetResizable(true);
        treatmentScroll->setFrameShape(QFrame::NoFrame);
        treatmentScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        treatmentScroll->setWidget(treatmentColumn);
        ui->mainGrid->addWidget(treatmentScroll, 0, 1, 1, 2);

        auto *dispatchColumn = new QWidget(this);
        dispatchColumn->setObjectName(QStringLiteral("widgetDispatchColumn"));
        auto *dispatchColumnLayout = new QVBoxLayout(dispatchColumn);
        dispatchColumnLayout->setObjectName(QStringLiteral("layoutDispatchColumn"));
        dispatchColumnLayout->setContentsMargins(0, 0, 0, 0);
        dispatchColumnLayout->setSpacing(6);
        dispatchColumnLayout->addWidget(ui->groupBoxOperationQueue, 0);
        auto *supportTabs = new QTabWidget(dispatchColumn);
        supportTabs->setObjectName(QStringLiteral("treatmentSupportTabs"));
        auto *bedPage = new QWidget(supportTabs);
        auto *bedPageLayout = new QVBoxLayout(bedPage);
        bedPageLayout->setContentsMargins(0, 0, 0, 0);
        bedPageLayout->addLayout(ui->verticalLayout_preview);
        supportTabs->addTab(bedPage, tr("治疗床 / 机架"));
        auto *doseScrollArea = new QScrollArea(dispatchColumn);
        doseScrollArea->setObjectName(QStringLiteral("scrollAreaDoseInformation"));
        doseScrollArea->setWidgetResizable(true);
        doseScrollArea->setFrameShape(QFrame::NoFrame);
        doseScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        doseScrollArea->setWidget(ui->groupBoxBedPosition);
        supportTabs->addTab(doseScrollArea, tr("束流明细"));
        dispatchColumnLayout->addWidget(supportTabs, 1);
        ui->mainGrid->addWidget(dispatchColumn, 0, 3, 1, 2);
    }

    for (QGroupBox *box : {ui->groupBoxPatientInfo, ui->groupBoxPrescriptionDose, ui->groupBoxTherapistInfo, ui->groupBoxRoomInfo, ui->groupBoxCurrentTime, ui->groupBoxTopActions}) {
        if (box) {
            box->setMinimumHeight(110);
            box->setMaximumHeight(124);
            box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    ui->groupBoxPatientInfo->setMinimumWidth(220);
    ui->groupBoxPrescriptionDose->setMinimumWidth(190);
    ui->groupBoxTherapistInfo->setMinimumWidth(210);
    ui->groupBoxRoomInfo->setMinimumWidth(190);
    ui->groupBoxCurrentTime->setMinimumWidth(260);
    ui->groupBoxTopActions->setMinimumWidth(420);

    if (ui->label_time) {
        ui->label_time->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (ui->label_time_week) {
        ui->label_time_week->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    ui->groupBoxTreatmentPlan->setMinimumWidth(520);
    ui->groupBoxTreatmentPlan->setMaximumHeight(QWIDGETSIZE_MAX);
    ui->groupBoxTreatmentPlan->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->groupBoxTreatmentFlow->setMinimumHeight(190);
    ui->groupBoxTreatmentFlow->setMaximumHeight(QWIDGETSIZE_MAX);
    ui->groupBoxTreatmentFlow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QSizePolicy flowPolicy = ui->groupBoxTreatmentFlow->sizePolicy();
    flowPolicy.setHeightForWidth(true);
    ui->groupBoxTreatmentFlow->setSizePolicy(flowPolicy);

    ui->groupBoxOperationQueue->setMinimumWidth(440);
    ui->groupBoxOperationQueue->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->groupBoxOperationQueue->setMaximumHeight(QWIDGETSIZE_MAX);
    ui->groupBoxOperationQueue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    ui->groupBoxSystemStatus->setMinimumHeight(76);
    ui->groupBoxSystemStatus->setMaximumHeight(QWIDGETSIZE_MAX);
    ui->groupBoxSystemStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->groupBoxBedPosition->setMinimumWidth(360);
    ui->groupBoxBedPosition->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->groupBoxBedPosition->setMaximumHeight(QWIDGETSIZE_MAX);
    ui->groupBoxBedPosition->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->todays_pushButton->setMinimumSize(132, 40);
    ui->theme_pushButton->setMinimumSize(132, 40);
    ui->logout_pushButton->setMinimumSize(108, 40);
    ui->todays_pushButton->setIcon(QIcon(QStringLiteral(":/icons/icons/schedule_action.svg")));
    ui->theme_pushButton->setIcon(QIcon(QStringLiteral(":/icons/icons/theme.svg")));
    ui->logout_pushButton->setIcon(QIcon(QStringLiteral(":/icons/icons/logout.svg")));

    for (QPushButton *button : {ui->todays_pushButton, ui->theme_pushButton, ui->logout_pushButton}) {
        if (button) {
            button->setIconSize(QSize(18, 18));
        }
    }

    ui->confirmPlan_pushButton->setMinimumSize(132, 38);
    setTreatmentRole(ui->confirmPlan_pushButton, QStringLiteral("primaryActionButtonStyle"));
    ui->confirmPlan_cancel_pushButton->setMinimumSize(132, 38);
    ui->confirmPlan_cancel_pushButton->hide();
    ui->confirmPlan_cancel_pushButton->setEnabled(false);

    ui->beamApply_pushButton->setFixedSize(118, 42);
    setTreatmentRole(ui->beamApply_pushButton, QStringLiteral("primaryActionButtonStyle"));
    ui->beamApply_cancel_pushButton->setFixedSize(118, 42);
    ui->beamApply_cancel_pushButton->hide();
    ui->beamApply_cancel_pushButton->setEnabled(false);

    if (ui->label_title_ops) {
        ui->label_title_ops->setText(tr("束流调度"));
        setTreatmentRole(ui->label_title_ops, QStringLiteral("treatmentStyle8"));
    }
    if (ui->labelBeamWaitTitle) {
        ui->labelBeamWaitTitle->setText(tr("预计等待时间"));
        ui->labelBeamWaitTitle->setAlignment(Qt::AlignCenter);
        setTreatmentRole(ui->labelBeamWaitTitle, QStringLiteral("treatmentStyle9"));
    }
    ui->label_30->setMinimumHeight(34);
    ui->label_30->setAlignment(Qt::AlignCenter);
    setTreatmentRole(ui->label_30, QStringLiteral("treatmentStyle10"));



    const std::initializer_list<QPair<QLabel*, QLabel*>> statusItems = {
        {ui->label_34, ui->label_9},
        {ui->label_36, ui->label_35},
        {ui->label_37, ui->label_46},
        {ui->label_48, ui->label_47}
    };
    for (const auto &item : statusItems) {
        QLabel *statusText = item.first;
        QLabel *statusDot = item.second;
        if (statusDot) {
            statusDot->show();
            statusDot->setFixedSize(14, 14);
        }
        if (statusText) {
            statusText->setMinimumWidth(88);
            statusText->setFixedHeight(30);
            statusText->setAlignment(Qt::AlignCenter);
            setTreatmentRole(statusText, QStringLiteral("statusItemStyle"));
        }
    }

    if (ui->label_icon_doctor) {
        ui->label_icon_doctor->setFixedSize(18, 18);
    }

    if (ui->horizontalLayout) {
        ui->horizontalLayout->setContentsMargins(10, 8, 10, 8);
        ui->horizontalLayout->setSpacing(10);
    }

    if (ui->verticalLayout_plan) {
        ui->verticalLayout_plan->setContentsMargins(10, 10, 10, 10);
        ui->verticalLayout_plan->setSpacing(8);
        ui->verticalLayout_plan->setStretch(0, 0);
        ui->verticalLayout_plan->setStretch(1, 0);
        ui->verticalLayout_plan->setStretch(2, 0);
        ui->verticalLayout_plan->setStretch(3, 1);
    }

    if (ui->widgetPlanSummary) {
        ui->widgetPlanSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    if (ui->horizontalLayout_7) {
        ui->horizontalLayout_7->setContentsMargins(36, 0, 36, 0);
        ui->horizontalLayout_7->setSpacing(12);
    }

    if (ui->horizontalLayout_6) {
        ui->horizontalLayout_6->setContentsMargins(8, 8, 8, 8);
        ui->horizontalLayout_6->setSpacing(0);
    }

    if (ui->horizontalLayout_4) {
        ui->horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        ui->horizontalLayout_4->setSpacing(0);
    }

    if (ui->verticalLayout_ops) {
        ui->verticalLayout_ops->setContentsMargins(12, 10, 12, 12);
        ui->verticalLayout_ops->setSpacing(8);
}

    if (ui->horizontalLayout_3) {
        ui->horizontalLayout_3->setContentsMargins(12, 0, 12, 2);
        ui->horizontalLayout_3->setSpacing(12);
    }
    if (ui->verticalLayout_statusBox) {
        ui->verticalLayout_statusBox->setContentsMargins(10, 8, 10, 8);
        ui->verticalLayout_statusBox->setSpacing(6);
    }

    constexpr int bedPositionFieldWidth = 112;

    if (ui->verticalLayout_bedBox) {
        ui->verticalLayout_bedBox->setContentsMargins(10, 8, 10, 10);
        ui->verticalLayout_bedBox->setSpacing(4);
    }
    if (ui->gridLayout_2) {
        ui->gridLayout_2->setContentsMargins(12, 0, 12, 10);
        ui->gridLayout_2->setHorizontalSpacing(10);
        ui->gridLayout_2->setVerticalSpacing(7);
        ui->gridLayout_2->setColumnMinimumWidth(0, 28);
        ui->gridLayout_2->setColumnMinimumWidth(1, bedPositionFieldWidth);
        ui->gridLayout_2->setColumnMinimumWidth(2, bedPositionFieldWidth);
        ui->gridLayout_2->setColumnStretch(0, 0);
        ui->gridLayout_2->setColumnStretch(1, 1);
        ui->gridLayout_2->setColumnStretch(2, 1);
        for (int row = 0; row <= 6; ++row) {
            ui->gridLayout_2->setRowStretch(row, 0);
        }
        ui->gridLayout_2->setRowStretch(7, 1);
        ui->gridLayout_2->setAlignment(Qt::AlignTop);
    }

    if (ui->verticalLayout_preview) {
        ui->verticalLayout_preview->setContentsMargins(0, 0, 0, 0);
        ui->verticalLayout_preview->setSpacing(0);
    }

    if (ui->verticalLayout_previewPanel) {
        ui->verticalLayout_previewPanel->setContentsMargins(10, 8, 10, 10);
        ui->verticalLayout_previewPanel->setSpacing(8);
    }

    if (ui->groupBoxBedPreview) {
        ui->groupBoxBedPreview->setMinimumWidth(360);
        ui->groupBoxBedPreview->setMaximumWidth(QWIDGETSIZE_MAX);
        ui->groupBoxBedPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    if (ui->horizontalLayout_bed) {
        ui->horizontalLayout_bed->setContentsMargins(0, 0, 0, 0);
        ui->horizontalLayout_bed->setSpacing(0);
    }

    if (ui->qmlStepContainer) {
        ui->qmlStepContainer->setMinimumHeight(118);
        ui->qmlStepContainer->setMaximumHeight(124);
        ui->qmlStepContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    if (ui->tableWidget) {
        ui->tableWidget->setAlternatingRowColors(true);
        ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget->verticalHeader()->setVisible(false);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    if (ui->tableWidget_2) {
        ui->tableWidget_2->setAlternatingRowColors(true);
        ui->tableWidget_2->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget_2->verticalHeader()->setVisible(false);
        ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    if (ui->tableWidget_3) {
        ui->tableWidget_3->setAlternatingRowColors(true);
        ui->tableWidget_3->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->tableWidget_3->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableWidget_3->verticalHeader()->setVisible(false);
        ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    for (QLabel *header : {ui->label_38, ui->label_39}) {
        if (header) {
            header->setFixedSize(bedPositionFieldWidth, 24);
            header->setAlignment(Qt::AlignCenter);
            setTreatmentRole(header, QStringLiteral("treatmentStyle11"));
            ui->gridLayout_2->setAlignment(header, Qt::AlignCenter);
        }
    }

    for (QLabel *axisLabel : {ui->label_40, ui->label_41, ui->label_42,
                              ui->label_43, ui->label_44, ui->label_45}) {
        if (axisLabel) {
            axisLabel->setFixedWidth(28);
            axisLabel->setAlignment(Qt::AlignCenter);
            setTreatmentRole(axisLabel, QStringLiteral("treatmentStyle12"));
            ui->gridLayout_2->setAlignment(axisLabel, Qt::AlignCenter);
        }
    }





    for (QLabel *label : {ui->label_posa, ui->label_posb, ui->label_posc,
                          ui->label_posx, ui->label_posy, ui->label_posz}) {
        if (label) {
            label->setFixedSize(bedPositionFieldWidth, 34);
            label->setAlignment(Qt::AlignCenter);
            if (label->text() == QStringLiteral("0.0")) {
                label->setText(QStringLiteral("0.00"));
            }
            setTreatmentRole(label, QStringLiteral("positionValueStyle"));
            label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            ui->gridLayout_2->setAlignment(label, Qt::AlignCenter);
        }
    }

    for (QLineEdit *edit : {ui->lineEdit_a, ui->lineEdit_b, ui->lineEdit_c,
                            ui->lineEdit_x, ui->lineEdit_y, ui->lineEdit_z}) {
        if (edit) {
            edit->setFixedSize(bedPositionFieldWidth, 34);
            edit->setAlignment(Qt::AlignCenter);
            if (edit->text().isEmpty()) {
                edit->setText(QStringLiteral("0.00"));
            }
            setTreatmentRole(edit, QStringLiteral("positionEditStyle"));
            edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            ui->gridLayout_2->setAlignment(edit, Qt::AlignCenter);
        }
    }


    for (QLabel *value : {ui->labelPrescriptionDose, ui->labelFractionDose,
                          ui->labelDoseDeliveryProgressValue}) {
        value->setWordWrap(true);
        setTreatmentRole(value, QStringLiteral("treatmentStyle13"));
    }
    setTreatmentRole(ui->labelTopPrescriptionDose, QStringLiteral("treatmentStyle14"));


    for (QLabel *label : {ui->labelPlanIdTitle, ui->labelPlanFractionIdTitle,
                          ui->labelPlanSiteTargetTitle, ui->labelPlanIsocenterTitle,
                          ui->labelPlanQaReleaseTitle}) {
        if (label) {
            label->setMinimumWidth(108);
            setTreatmentRole(label, QStringLiteral("planNameStyle"));
            label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    for (QLabel *label : {ui->labelPlanIdValue, ui->labelPlanFractionIdValue,
                          ui->labelPlanSiteTargetValue, ui->labelPlanIsocenterValue,
                          ui->labelPlanQaReleaseValue}) {
        if (label) {
            label->setMinimumHeight(34);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
            setTreatmentRole(label, QStringLiteral("planValueStyle"));
            label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    if (ui->gridLayout_planSummary) {
        ui->gridLayout_planSummary->setColumnStretch(0, 0);
        ui->gridLayout_planSummary->setColumnStretch(1, 1);
    }

    if (ui->planDetail_pushButton) {
        ui->planDetail_pushButton->setCursor(Qt::PointingHandCursor);
        ui->planDetail_pushButton->setProperty("sessionReadOnly", true);
        connect(ui->planDetail_pushButton, &QPushButton::clicked,
                this, &TreatmentWindow::showCurrentPlanDetail);
    }
}

void TreatmentWindow::setupClinicalWorkspace()
{
    if (m_clinicalBody) return;
    ui->centralwidget->setProperty("clinicalWorkspace", true);
    auto *legacy = new QWidget(ui->centralwidget);
    legacy->setObjectName(QStringLiteral("clinicalLegacyHost"));
    auto *legacyLayout = new QVBoxLayout(legacy);
    for (QLayout *layout : {static_cast<QLayout *>(ui->horizontalLayout_topArea),
                            static_cast<QLayout *>(ui->mainGrid)}) {
        ui->verticalLayout_root->removeItem(layout);
        layout->setParent(nullptr);
        legacyLayout->addLayout(layout);
    }
    legacy->hide();
    auto *root = ui->verticalLayout_root;
    root->setContentsMargins(9, 7, 9, 9);
    root->setSpacing(7);

    const auto panel = [](const QString &name, QWidget *parent) {
        auto *widget = new QWidget(parent);
        widget->setObjectName(name);
        widget->setProperty("clinicalRole", "surface");
        widget->setAttribute(Qt::WA_StyledBackground);
        return widget;
    };
    const auto label = [](const QString &text, const char *role, QWidget *parent) {
        auto *value = new QLabel(text, parent);
        value->setTextFormat(Qt::PlainText);
        value->setWordWrap(true);
        value->setMinimumWidth(0);
        value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        value->setProperty("clinicalRole", role);
        return value;
    };
    const auto move = [](QWidget *widget, QBoxLayout *layout) {
        widget->setParent(layout->parentWidget());
        widget->setMinimumWidth(0);
        widget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(widget);
        widget->show();
    };
    // Presentation-only medical monitoring card header: no equipment state is set here.
    const auto monitorCardBar = [](QWidget *parent, const QString &text,
                                   const QString &symbol, bool teal = false) {
        auto *bar = new QWidget(parent);
        bar->setObjectName(QStringLiteral("clinicalMonitorCardBar"));
        bar->setProperty("cardTone", teal ? "teal" : "blue");
        bar->setAttribute(Qt::WA_StyledBackground);
        bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        bar->setFixedHeight(42);
        auto *row = new QHBoxLayout(bar);
        row->setContentsMargins(14, 0, 12, 0);
        row->setSpacing(9);
        auto *glyph = new QLabel(symbol, bar);
        glyph->setObjectName(QStringLiteral("clinicalMonitorCardGlyph"));
        glyph->setFixedWidth(23);
        glyph->setAlignment(Qt::AlignCenter);
        auto *caption = new QLabel(text, bar);
        caption->setObjectName(QStringLiteral("clinicalMonitorCardCaption"));
        caption->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        row->addWidget(glyph);
        row->addWidget(caption, 1);
        return bar;
    };
    const auto monitorSectionBar = [](QWidget *parent, const QString &text, const QString &symbol) {
        auto *bar = new QWidget(parent);
        bar->setObjectName(QStringLiteral("clinicalMonitorSectionBar"));
        bar->setAttribute(Qt::WA_StyledBackground);
        bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        bar->setFixedHeight(31);
        auto *row = new QHBoxLayout(bar);
        row->setContentsMargins(9, 0, 8, 0);
        row->setSpacing(7);
        auto *glyph = new QLabel(symbol, bar);
        glyph->setObjectName(QStringLiteral("clinicalMonitorSectionGlyph"));
        glyph->setFixedWidth(17);
        glyph->setAlignment(Qt::AlignCenter);
        auto *caption = new QLabel(text, bar);
        caption->setObjectName(QStringLiteral("clinicalMonitorSectionCaption"));
        caption->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        row->addWidget(glyph);
        row->addWidget(caption, 1);
        return bar;
    };
    const auto scroll = [](QWidget *content, const QString &name) {
        auto *area = new QScrollArea;
        area->setObjectName(name);
        area->setWidgetResizable(true);
        area->setFrameShape(QFrame::NoFrame);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        area->setWidget(content);
        return area;
    };

    auto *header = panel(QStringLiteral("clinicalHeader"), ui->centralwidget);
    header->setMinimumHeight(56);
    header->setMaximumHeight(72);
    auto *headerGrid = new QGridLayout(header);
    headerGrid->setContentsMargins(12, 4, 12, 4);
    headerGrid->setColumnStretch(0, 0);
    headerGrid->setColumnStretch(1, 1);
    headerGrid->setColumnStretch(2, 0);
    auto *brand = label(tr("SIMM 治疗控制系统\n治疗模式"), "title", header);
    brand->setObjectName(QStringLiteral("clinicalBrand"));
    brand->setMinimumWidth(230);
    headerGrid->addWidget(brand, 0, 0);
    // Keep these data-bound labels off the toolbar, including later visibility updates.
    ui->label_room->setParent(legacy);
    ui->label_room->hide();
    m_initializationLabel = label(tr("正在初始化 · 等待后台完成通知"), "section", legacy);
    m_initializationLabel->setObjectName(QStringLiteral("initializationStatus"));
    m_initializationLabel->setAlignment(Qt::AlignCenter);
    m_initializationLabel->setWordWrap(true);
    auto *session = new QWidget(header);
    auto *sessionLayout = new QVBoxLayout(session);
    sessionLayout->setContentsMargins(0, 0, 0, 0);
    sessionLayout->setSpacing(4);
    auto *identity = new QHBoxLayout;
    identity->addStretch();
    for (QLabel *value : {ui->label_6, ui->label_user}) {
        value->setParent(session);
        value->setProperty("clinicalRole", "muted");
        identity->addWidget(value);
    }
    identity->setSpacing(12);
    sessionLayout->addLayout(identity);
    auto *commands = new QHBoxLayout;
    commands->setSpacing(6);
    commands->addStretch();
    for (QPushButton *button : {ui->todays_pushButton, m_queryWorkflowButton,
                                m_resetWorkflowButton, ui->theme_pushButton, ui->logout_pushButton}) {
        if (!button) continue;
        button->setParent(session);
        commands->addWidget(button);
        button->setMinimumHeight(38);
    }
    ui->todays_pushButton->setText(tr("排程"));
    ui->theme_pushButton->setText(tr("主题"));
    ui->logout_pushButton->setText(tr("注销"));
    for (QPushButton *button : {ui->todays_pushButton, ui->theme_pushButton, ui->logout_pushButton})
        button->setMinimumWidth(76);
    auto *sessionInfo = new QToolButton(session);
    sessionInfo->setObjectName(QStringLiteral("clinicalSessionInfo"));
    sessionInfo->setProperty("sessionReadOnly", true);
    sessionInfo->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    sessionInfo->setToolTip(tr("登录及版本信息"));
    sessionInfo->setPopupMode(QToolButton::InstantPopup);
    auto *sessionMenu = new QMenu(sessionInfo);
    sessionInfo->setMenu(sessionMenu);
    connect(sessionMenu, &QMenu::aboutToShow, this, [this, sessionMenu] {
        sessionMenu->clear();
        for (const QString &text : {tr("当前用户：%1").arg(m_currentUsername),
                                    ui->label_time->text(),
                                    tr("版本：%1").arg(QCoreApplication::applicationVersion())})
            sessionMenu->addAction(text)->setEnabled(false);
    });
    commands->addWidget(sessionInfo);
    sessionLayout->addLayout(commands);
    headerGrid->addWidget(session, 0, 2);
    root->addWidget(header);
    move(ui->groupBoxSystemStatus, root);
    auto *faultDisplay = new FaultDisplay(m_webSocketClient, [this] { return m_roomId; }, ui->centralwidget);
    root->addWidget(faultDisplay);
    ui->groupBoxTreatmentFlow->setParent(ui->centralwidget);
    ui->groupBoxTreatmentFlow->setMinimumSize(0, 0);
    ui->groupBoxTreatmentFlow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->verticalLayout_flowBox->setContentsMargins(10, 6, 10, 6);
    ui->verticalLayout_flowBox->removeItem(ui->horizontalLayout_title_flow);
    ui->horizontalLayout_title_flow->setParent(nullptr);
    legacyLayout->addLayout(ui->horizontalLayout_title_flow);
    ui->horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
    ui->label_title_flow->hide();
    ui->label_icon_flow->hide();
    root->addWidget(ui->groupBoxTreatmentFlow);

    m_clinicalBody = new QSplitter(Qt::Horizontal, ui->centralwidget);
    m_clinicalBody->setObjectName(QStringLiteral("clinicalColumns"));
    auto *columns = static_cast<QSplitter *>(m_clinicalBody);
    columns->setChildrenCollapsible(false);
    columns->setHandleWidth(8);
    root->addWidget(columns, 1);
    auto *left = panel(QStringLiteral("clinicalPatientColumn"), columns);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(5, 3, 5, 5);
    leftLayout->setSpacing(9);
    leftLayout->setAlignment(Qt::AlignTop);
    identity->removeWidget(ui->label_6);
    identity->removeWidget(ui->label_user);
    for (QLabel *value : {ui->label_6, ui->label_user}) {
        value->setParent(left);
        value->setWordWrap(true);
        leftLayout->addWidget(value);
    }
    for (QGroupBox *box : {ui->groupBoxPatientInfo, ui->groupBoxPrescriptionDose}) {
        move(box, leftLayout);
        box->setMinimumHeight(0);
    }
    ui->verticalLayout_plan->removeWidget(ui->spotMapWidget);
    auto *planPage = new QWidget;
    auto *planLayout = new QVBoxLayout(planPage);
    planLayout->setContentsMargins(0, 0, 0, 0);
    planLayout->setAlignment(Qt::AlignTop);
    move(ui->groupBoxTreatmentPlan, planLayout);
    ui->verticalLayout_plan->setAlignment(Qt::AlignTop);
    auto *patientScroll = scroll(planPage, QStringLiteral("clinicalPatientScroll"));
    patientScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    patientScroll->setMinimumHeight(350);
    patientScroll->setMaximumHeight(480);
    leftLayout->addWidget(patientScroll);
    ui->groupBoxTreatmentPlan->setMinimumHeight(0);
    ui->widgetPlanSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const QVector<QLabel *> names = {ui->labelPlanIdTitle, ui->labelPlanFractionIdTitle,
        ui->labelPlanSiteTargetTitle, ui->labelPlanIsocenterTitle, ui->labelPlanQaReleaseTitle};
    const QVector<QLabel *> values = {ui->labelPlanIdValue, ui->labelPlanFractionIdValue,
        ui->labelPlanSiteTargetValue, ui->labelPlanIsocenterValue, ui->labelPlanQaReleaseValue};
    for (int index = 0; index < names.size(); ++index) {
        ui->gridLayout_planSummary->removeWidget(names.at(index));
        ui->gridLayout_planSummary->removeWidget(values.at(index));
        for (QLabel *value : {names.at(index), values.at(index)}) {
            value->setMinimumWidth(0);
            value->setTextFormat(Qt::PlainText);
            value->setWordWrap(true);
            value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        }
        names.at(index)->setProperty("clinicalRole", "muted");
        ui->gridLayout_planSummary->addWidget(names.at(index), index * 2, 0);
        auto *readback = new ClinicalReadbackView(ui->widgetPlanSummary);
        readback->setProperty("sourceLabel", values.at(index)->objectName());
        readback->setReadback(values.at(index)->text());
        values.at(index)->hide();
        values.at(index)->setParent(legacy);
        ui->gridLayout_planSummary->addWidget(readback, index * 2 + 1, 0);
    }
    ui->gridLayout_planSummary->setColumnStretch(1, 0);
    ui->horizontalLayout_7->setContentsMargins(0, 4, 0, 0);
    ui->horizontalLayout_7->setSpacing(6);
    ui->confirmPlan_pushButton->setText(tr("确认计划"));
    ui->confirmPlan_pushButton->setMinimumWidth(110);
    ui->planDetail_pushButton->setMinimumWidth(76);
    m_fieldSummary = new QTreeWidget(left);
    m_fieldSummary->setObjectName(QStringLiteral("clinicalFieldSummary"));
    m_fieldSummary->setColumnCount(1);
    m_fieldSummary->setHeaderHidden(true);
    m_fieldSummary->setRootIsDecorated(false);
    m_fieldSummary->setWordWrap(true);
    m_fieldSummary->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_fieldSummary->header()->setSectionResizeMode(QHeaderView::Stretch);
    m_fieldSummary->setMinimumHeight(180);
    m_fieldSummary->setMaximumHeight(260);
    new QTreeWidgetItem(m_fieldSummary, {tr("等待射野计划")});
    auto *fieldTitle = label(tr("射野信息"), "section", left);
    leftLayout->addWidget(fieldTitle);
    leftLayout->addWidget(m_fieldSummary);
    // Don't place an expanding spacer here: after the field widgets are moved
    // into the plan scroll page, it would separate the identity and plan cards.
    // Add the stretch AFTER the plan card, just before the bottom actions.
    auto *planActions = new QHBoxLayout;
    for (QPushButton *button : {ui->confirmPlan_pushButton, ui->planDetail_pushButton}) {
        button->setParent(left);
        planActions->addWidget(button);
    }
    leftLayout->addLayout(planActions);
    // Outer card: operator info + patient info share one larger card.
    auto *identityCard = new QGroupBox(tr("人员与患者信息"), left);
    identityCard->setObjectName(QStringLiteral("clinicalIdentityCard"));
    identityCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *identityLayout = new QVBoxLayout(identityCard);
    identityCard->setTitle(QString());
    identityLayout->setContentsMargins(8, 7, 8, 9);
    identityLayout->setSpacing(6);
    identityLayout->addWidget(monitorCardBar(identityCard, tr("人员与患者信息"),
                                             QStringLiteral("✚")));

    // Use titled group boxes to make the three clinical information areas explicit.
    auto *operatorSection = new QGroupBox(left);
    operatorSection->setObjectName(QStringLiteral("clinicalOperatorGroup"));
    operatorSection->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *operatorLayout = new QVBoxLayout(operatorSection);
    operatorLayout->setContentsMargins(7, 5, 7, 5);
    operatorLayout->setSpacing(5);
    // The operator area has no legacy icon heading. Build one to match
    // the existing patient and treatment-plan icon headings.
    auto *operatorHeader = new QWidget(operatorSection);
    operatorHeader->setObjectName(QStringLiteral("clinicalMonitorSectionBar"));
    operatorHeader->setAttribute(Qt::WA_StyledBackground);
    auto *operatorHeadingRow = new QHBoxLayout(operatorHeader);
    operatorHeadingRow->setContentsMargins(9, 1, 9, 1);
    operatorHeadingRow->setSpacing(6);
    auto *operatorHeadingIcon = new QLabel(operatorSection);
    operatorHeadingIcon->setObjectName(QStringLiteral("clinicalOperatorHeadingIcon"));
    operatorHeadingIcon->setFixedSize(18, 18);
    const QPixmap operatorPixmap = QIcon(QStringLiteral(":/icons/icons/therapist_new.svg"))
                                     .pixmap(QSize(18, 18));
    if (!operatorPixmap.isNull()) {
        operatorHeadingIcon->setPixmap(operatorPixmap);
    } else {
        operatorHeadingIcon->setText(QStringLiteral("●"));
        operatorHeadingIcon->setProperty("clinicalRole", "groupHeadingIcon");
    }
    auto *operatorHeading = label(tr("操作员信息"), "groupHeading", operatorSection);
    operatorHeading->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    operatorHeadingRow->addWidget(operatorHeadingIcon);
    operatorHeadingRow->addWidget(operatorHeading, 1);
    operatorHeader->setFixedHeight(31);
    operatorLayout->addWidget(operatorHeader);
    // Operator photo placeholder + authenticated identity. Keep actual login
    // widgets and network state authoritative; no photo/id is fabricated.
    auto *operatorInfoRow = new QHBoxLayout;
    operatorInfoRow->setContentsMargins(0, 2, 0, 0);
    operatorInfoRow->setSpacing(12);
    auto *photo = new QLabel(tr("照片\n待提供"), operatorSection);
    photo->setObjectName(QStringLiteral("clinicalOperatorPhoto"));
    photo->setTextFormat(Qt::PlainText);
    photo->setAlignment(Qt::AlignCenter);
    photo->setWordWrap(true);
    photo->setFixedSize(96, 112);
    photo->setToolTip(tr("预留操作员照片位置。照片数据接入后可使用 setPixmap() 更新。"));
    operatorInfoRow->addWidget(photo, 0, Qt::AlignTop);

    auto *operatorInfoGrid = new QGridLayout;
    operatorInfoGrid->setContentsMargins(0, 0, 0, 0);
    operatorInfoGrid->setHorizontalSpacing(8);
    operatorInfoGrid->setVerticalSpacing(10);
    operatorInfoGrid->setColumnStretch(0, 0);
    operatorInfoGrid->setColumnStretch(1, 1);
    const auto addOperatorCaption = [operatorSection, &label, operatorInfoGrid]
        (const QString &text, int row) {
        QLabel *caption = label(text, "operatorCaption", operatorSection);
        caption->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        operatorInfoGrid->addWidget(caption, row, 0);
    };

    leftLayout->removeWidget(ui->label_6);
    leftLayout->removeWidget(ui->label_user);
    ui->label_6->setParent(operatorSection);
    ui->label_6->setProperty("clinicalRole", "operatorValue");
    ui->label_6->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->label_6->setWordWrap(true);
    ui->label_6->setMinimumWidth(0);
    ui->label_6->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    addOperatorCaption(tr("姓名"), 0);
    operatorInfoGrid->addWidget(ui->label_6, 0, 1);

    // Preserve the role field for existing backend/UI logic, but do not show it
    // as an additional row in the requested three-field identity card.
    ui->label_user->setParent(legacy);
    ui->label_user->hide();
    addOperatorCaption(tr("工号"), 1);
    auto *employeeNoValue = label(tr("未提供"), "operatorValue", operatorSection);
    employeeNoValue->setObjectName(QStringLiteral("clinicalEmployeeNoValue"));
    employeeNoValue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const QString employeeNo = operatorEmployeeNo(m_userInfo);
    if (m_startupState.authenticated && !employeeNo.isEmpty())
        employeeNoValue->setText(employeeNo);
    operatorInfoGrid->addWidget(employeeNoValue, 1, 1);

    ui->label_7->setParent(operatorSection);
    ui->label_7->setProperty("clinicalRole", "operatorValue");
    ui->label_7->setProperty("clinicalOperatorRoom", false);
    ui->label_7->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->label_7->setWordWrap(true);
    ui->label_7->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    addOperatorCaption(tr("治疗室"), 2);
    operatorInfoGrid->addWidget(ui->label_7, 2, 1);
    operatorInfoRow->addLayout(operatorInfoGrid, 1);
    operatorLayout->addLayout(operatorInfoRow);
    identityLayout->addWidget(operatorSection);
    // Reparenting a QWidget hides it; make the two existing .ui labels visible.
    ui->label_6->show();
    ui->label_7->show();

    auto *patientSection = new QGroupBox(left);
    patientSection->setObjectName(QStringLiteral("clinicalPatientGroup"));
    patientSection->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *patientLayout = new QVBoxLayout(patientSection);
    patientLayout->setContentsMargins(7, 5, 7, 5);
    patientLayout->setSpacing(4);
    patientLayout->addWidget(monitorSectionBar(patientSection, tr("患者信息"),
                                                QStringLiteral("●")));
    // The source .ui heading is hidden only to prevent a duplicate label.
    if (auto *patientHeading = findChild<QLabel *>(QStringLiteral("label_title_patient")))
        patientHeading->hide();
    if (auto *patientIcon = findChild<QLabel *>(QStringLiteral("label_icon_patient")))
        patientIcon->hide();
    ui->groupBoxPatientInfo->setTitle(QString()); // Keep the original patient data bindings.

    // Patient photo placeholder to match the operator's photo card.
    // Display-only; no patient image or identity is synthesized.
    auto *patientIdentityRow = new QHBoxLayout;
    patientIdentityRow->setContentsMargins(0, 3, 0, 2);
    patientIdentityRow->setSpacing(12);
    patientLayout->addLayout(patientIdentityRow);
    auto *patientPhoto = new QLabel(tr("照片\n待提供"), patientSection);
    patientPhoto->setObjectName(QStringLiteral("clinicalPatientPhoto"));
    patientPhoto->setTextFormat(Qt::PlainText);
    patientPhoto->setAlignment(Qt::AlignCenter);
    patientPhoto->setWordWrap(true);
    patientPhoto->setFixedSize(96, 112);
    patientPhoto->setToolTip(tr("预留患者照片位置；未接入经核验的患者照片前，不显示任何模拟照片。"));
    patientIdentityRow->addWidget(patientPhoto, 0, Qt::AlignTop);

    leftLayout->removeWidget(ui->groupBoxPatientInfo);
    ui->groupBoxPatientInfo->setMinimumHeight(0);
    move(ui->groupBoxPatientInfo, patientIdentityRow);

    // Prescription-dose fields remain below the patient identity row.
    leftLayout->removeWidget(ui->groupBoxPrescriptionDose);
    ui->groupBoxPrescriptionDose->setMinimumHeight(0);
    move(ui->groupBoxPrescriptionDose, patientLayout);
    identityLayout->addWidget(patientSection);

    leftLayout->insertWidget(0, identityCard);

    auto *planSection = new QGroupBox(left);
    planSection->setObjectName(QStringLiteral("clinicalPlanGroup"));
    planSection->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *planSectionLayout = new QVBoxLayout(planSection);
    planSection->setTitle(QString());
    planSectionLayout->setContentsMargins(8, 7, 8, 8);
    planSectionLayout->setSpacing(6);
    planSectionLayout->addWidget(monitorCardBar(planSection, tr("治疗计划"),
                                                 QStringLiteral("▦")));
    if (auto *planHeading = findChild<QLabel *>(QStringLiteral("label_title_plan")))
        planHeading->hide();
    if (auto *planIcon = findChild<QLabel *>(QStringLiteral("label_icon_plan")))
        planIcon->hide();
    ui->groupBoxTreatmentPlan->setTitle(QString()); // Existing icon + title is the only plan heading.

    // Append field information directly under the treatment-plan content so it
    // appears as part of the treatment plan section instead of an external block.
    planLayout->setSpacing(8);
    fieldTitle->setParent(planPage);
    fieldTitle->setContentsMargins(0, 6, 0, 0);
    planLayout->addWidget(fieldTitle);
    m_fieldSummary->setParent(planPage);
    m_fieldSummary->setMaximumHeight(QWIDGETSIZE_MAX);
    m_fieldSummary->setMinimumHeight(180);
    planLayout->addWidget(m_fieldSummary);

    leftLayout->removeWidget(patientScroll);
    patientScroll->setParent(planSection);
    planSectionLayout->addWidget(patientScroll);
    // Keep the treatment-plan card immediately below the personnel/patient
    // card. Only the remaining space before the bottom action buttons expands.
    leftLayout->insertWidget(1, planSection);
    leftLayout->insertStretch(leftLayout->count() - 1, 1);
    left->setMinimumWidth(380);
    left->setMaximumWidth(520);
    columns->addWidget(left);

    // Main workflow/plan section occupies the whole lower-left column.
    // Use a real QGroupBox to visually separate it from safety and dispatch.
    auto *center = new QGroupBox(tr("计划核对与流程监控"), columns);
    center->setObjectName(QStringLiteral("clinicalExecutionColumn"));
    center->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *centerLayout = new QVBoxLayout(center);
    center->setTitle(QString());
    centerLayout->setContentsMargins(7, 6, 7, 8);
    centerLayout->setSpacing(6);
    // Separate, full-width header for the workflow card; the flow itself is untouched.
    centerLayout->addWidget(monitorCardBar(center, tr("流程状态"),
                                            QStringLiteral("▤")));
    auto *stageHeading = label(tr("计划核对"), "section", center);
    stageHeading->setObjectName(QStringLiteral("clinicalStageHeading"));
    centerLayout->addWidget(stageHeading);
    stageHeading->hide();
    move(findChild<QGroupBox *>(QStringLiteral("groupBoxDeviceReadiness")), centerLayout);
    ui->labelDeviceStatusCaption->setText(tr("设备状态"));
    ui->labelDeviceStatusCaption->setProperty("clinicalRole", "section");
    auto *auxiliary = new QWidget(center);
    auto *auxiliaryGrid = new QGridLayout(auxiliary);
    auxiliaryGrid->setObjectName(QStringLiteral("clinicalAuxiliaryLayout"));
    auxiliaryGrid->setContentsMargins(0, 0, 0, 0);
    auxiliaryGrid->setHorizontalSpacing(16);
    int auxiliaryIndex = 0;
    for (const QString &name : {QStringLiteral("g01Homed"), QStringLiteral("g02Homed"),
                                QStringLiteral("acceleratorReady"), QStringLiteral("ddsFault")}) {
        auto *pair = new QWidget(auxiliary);
        pair->setObjectName(QStringLiteral("clinicalSafetyIndicator%1").arg(auxiliaryIndex));
        pair->setProperty("clinicalSafetyItem", true);
        pair->setAttribute(Qt::WA_StyledBackground);
        auto *row = new QHBoxLayout(pair);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(6);
        for (const QString &suffix : {QStringLiteral("Lamp"), QStringLiteral("Text")}) {
            if (auto *widget = findChild<QWidget *>(name + suffix)) {
                widget->setParent(pair);
                widget->show();
                row->addWidget(widget);
            }
        }
        row->addStretch();
        auxiliaryGrid->addWidget(pair, 0, auxiliaryIndex);
        auxiliaryGrid->setColumnStretch(auxiliaryIndex, 1);
        ++auxiliaryIndex;
    }
    if (auto *item = ui->gridLayout_deviceStatus->itemAtPosition(2, 0)) {
        if (auto *oldRow = item->layout()) {
            ui->gridLayout_deviceStatus->removeItem(oldRow);
            delete oldRow;
        }
    }
    // Real readback indicators move into the top safety panel below.
    m_executionTabs = new QTabWidget(center);
    m_executionTabs->setObjectName(QStringLiteral("clinicalExecutionTabs"));
    auto *overview = new WorkflowOverview;
    m_executionTabs->addTab(scroll(overview, QStringLiteral("clinicalOverviewScroll")), tr("流程监控"));
    auto *planPreview = new QWidget;
    auto *planPreviewLayout = new QVBoxLayout(planPreview);
    planPreviewLayout->setContentsMargins(0, 4, 0, 0);
    planPreviewLayout->setSpacing(10);
    planPreviewLayout->setAlignment(Qt::AlignTop);
    auto *planMetrics = new QWidget(planPreview);
    auto *planMetricsLayout = new QGridLayout(planMetrics);
    planMetricsLayout->setContentsMargins(0, 4, 0, 4);
    for (int index = 0; index < 3; ++index) {
        const QStringList names = {tr("射野数"), tr("能层数"), tr("Spot 数")};
        const QStringList objects = {QStringLiteral("clinicalBeamCount"), QStringLiteral("clinicalLayerCount"),
                                     QStringLiteral("clinicalSpotCount")};
        auto *caption = label(names.at(index), "muted", planMetrics);
        auto *value = label(tr("未提供"), "planMetric", planMetrics);
        value->setObjectName(objects.at(index));
        planMetricsLayout->addWidget(caption, 0, index);
        planMetricsLayout->addWidget(value, 1, index);
        planMetricsLayout->setColumnStretch(index, 1);
    }
    planPreviewLayout->addWidget(planMetrics);
    auto *planWaiting = label(tr("等待治疗计划"), "empty", planPreview);
    planWaiting->setObjectName(QStringLiteral("clinicalPlanWaiting"));
    planWaiting->setAlignment(Qt::AlignCenter);
    planWaiting->setMinimumHeight(200);
    planPreviewLayout->addWidget(planWaiting);
    auto *planMapHost = new QWidget(planPreview);
    planMapHost->setObjectName(QStringLiteral("clinicalPlanMapHost"));
    auto *planMapLayout = new QVBoxLayout(planMapHost);
    planMapLayout->setContentsMargins(0, 0, 0, 0);
    planPreviewLayout->addWidget(planMapHost, 1);
    m_executionTabs->addTab(scroll(planPreview, QStringLiteral("clinicalPlanPreviewScroll")), tr("计划核对"));
    auto *bedPage = new QWidget;
    auto *bedLayout = new QVBoxLayout(bedPage);
    bedLayout->setContentsMargins(0, 0, 0, 0);
    bedLayout->setAlignment(Qt::AlignTop);
    move(ui->groupBoxBedPreview, bedLayout);
    ui->groupBoxBedPreview->setMinimumHeight(420);
    ui->groupBoxBedPreview->setMaximumHeight(560);
    bedLayout->addWidget(label(tr("坐标容差：未配置 · 计划值及角度单位接口待确认"), "muted", bedPage));
    m_executionTabs->addTab(scroll(bedPage, QStringLiteral("clinicalBedScroll")), tr("摆位 / 配准"));
    auto *deliveryPage = new QWidget;
    auto *deliveryLayout = new QVBoxLayout(deliveryPage);
    deliveryLayout->setContentsMargins(0, 0, 0, 0);
    deliveryLayout->setSpacing(10);
    auto *waiting = label(tr("等待治疗任务"), "empty", deliveryPage);
    waiting->setObjectName(QStringLiteral("clinicalWaiting"));
    waiting->setAlignment(Qt::AlignCenter);
    waiting->setMinimumHeight(200);
    deliveryLayout->addWidget(waiting);
    auto *execution = new QWidget(deliveryPage);
    auto *executionGrid = new QGridLayout(execution);
    executionGrid->setObjectName(QStringLiteral("clinicalDeliveryGrid"));
    executionGrid->setContentsMargins(0, 0, 0, 0);
    executionGrid->setSpacing(12);
    auto *live = findChild<QGroupBox *>(QStringLiteral("groupBoxLiveDose"));
    live->setParent(execution);
    executionGrid->addWidget(live, 0, 0, Qt::AlignTop);
    deliveryLayout->addWidget(execution);
    auto *liveGrid = deliveryPage->findChild<QGridLayout *>(QStringLiteral("gridLayoutLiveDose"));
    liveGrid->removeWidget(ui->doseLayerProgressCard);
    ui->doseLayerProgressCard->hide();
    liveGrid->removeWidget(ui->doseBeamMuCard);
    liveGrid->addWidget(ui->doseBeamMuCard, 3, 0, 1, 2);
    ui->labelDoseDeliveryProgressName->setText(tr("整场计量 / 目标"));
    ui->spotMapWidget->setParent(execution);
    executionGrid->addWidget(ui->spotMapWidget, 1, 0);
    ui->spotMapWidget->setMinimumHeight(330);
    deliveryLayout->addWidget(label(tr("靶区轮廓：未接入"), "muted", deliveryPage));
    auto *channels = new QWidget(deliveryPage);
    channels->setObjectName(QStringLiteral("clinicalDoseChannels"));
    auto *channelGrid = new QGridLayout(channels);
    channelGrid->setContentsMargins(0, 8, 0, 8);
    channelGrid->setColumnStretch(0, 1);
    channelGrid->setColumnStretch(1, 1);
    for (int index = 0; index < 2; ++index) {
        channelGrid->addWidget(label(index == 0 ? tr("主剂量通道") : tr("备剂量通道"), "section", channels), 0, index);
        channelGrid->addWidget(label(tr("— 未接入"), "metric", channels), 1, index);
    }
    channelGrid->addWidget(label(tr("通道一致性：未接入 · 偏差阈值未配置"), "muted", channels), 2, 0, 1, 2);
    channelGrid->addWidget(label(tr("已治疗时间：未接入    剩余治疗时间：未接入"), "muted", channels), 3, 0, 1, 2);
    deliveryLayout->addWidget(channels);
    m_executionTabs->addTab(scroll(deliveryPage, QStringLiteral("clinicalDeliveryScroll")), tr("执行 / 剂量"));
    centerLayout->addWidget(m_executionTabs, 1);
    center->setMinimumWidth(580);
    columns->addWidget(center);

    auto *right = panel(QStringLiteral("clinicalDispatchColumn"), columns);
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(8);
    rightLayout->setAlignment(Qt::AlignTop);

    // Reference-style monitoring cards; the original readback widgets are reused.
    auto *doseCard = new QGroupBox(tr("剂量监控"), right);
    doseCard->setObjectName(QStringLiteral("clinicalDoseCard"));
    auto *doseCardLayout = new QVBoxLayout(doseCard);
    doseCardLayout->setContentsMargins(8, 12, 8, 8);
    doseCardLayout->setSpacing(6);
    executionGrid->removeWidget(live);
    move(live, doseCardLayout);
    rightLayout->addWidget(doseCard);
    live->setProperty("clinicalDoseSidebar", true);

    auto *queueCard = new QGroupBox(tr("准入排队 / 束流调度"), right);
    queueCard->setObjectName(QStringLiteral("clinicalQueueCard"));
    auto *queueCardLayout = new QVBoxLayout(queueCard);
    queueCardLayout->setContentsMargins(8, 12, 8, 8);
    queueCardLayout->setSpacing(6);
    move(ui->groupBoxOperationQueue, queueCardLayout);
    // Read-only WatchAdmission data. Queue state never grants beam permission.
    auto *admissionLabel = new QLabel(tr("准入排队：等待调度回读"), queueCard);
    admissionLabel->setObjectName(QStringLiteral("clinicalAdmissionTelemetryLabel"));
    admissionLabel->setWordWrap(true);
    admissionLabel->setTextFormat(Qt::PlainText);
    queueCardLayout->addWidget(admissionLabel);
    rightLayout->addWidget(queueCard);
    ui->verticalLayout_ops->removeItem(ui->verticalSpacer_ops);
    delete ui->verticalSpacer_ops;
    ui->verticalSpacer_ops = nullptr;
    ui->verticalLayout_ops->setAlignment(Qt::AlignTop);
    ui->groupBoxOperationQueue->setMinimumHeight(0);
    ui->beamApply_pushButton->setMinimumWidth(160);
    ui->beamApply_pushButton->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->beamApply_pushButton->setMinimumHeight(46);
    ui->label_30->setText(tr("未接入"));
    ui->label_30->setToolTip(tr("等待时间由后端提供；不使用本地写死的示例倒计时。"));
    if (auto *roomGrid = qobject_cast<QGridLayout *>(ui->beamRoomLabel1->parentWidget()->layout())) {
        const QVector<QLabel *> rooms = {ui->beamRoomLabel1, ui->beamRoomLabel2, ui->beamRoomLabel9};
        for (int index = 0; index < rooms.size(); ++index) {
            roomGrid->removeWidget(rooms.at(index));
            rooms.at(index)->setMinimumHeight(58);
            rooms.at(index)->setWordWrap(true);
            roomGrid->addWidget(rooms.at(index), index, 0);
        }
        for (int index = 0; index < 3; ++index) roomGrid->setColumnStretch(index, index == 0 ? 1 : 0);
    }
    rightLayout->addWidget(label(tr("当前供束角度：未接入"), "muted", right));
    auto *hardwareToggle = new QToolButton(right);
    hardwareToggle->setObjectName(QStringLiteral("clinicalHardwareToggle"));
    hardwareToggle->setProperty("sessionReadOnly", true);
    hardwareToggle->setText(tr("硬件模块 / 门控 · 未接入"));
    hardwareToggle->setCheckable(true);
    hardwareToggle->setArrowType(Qt::RightArrow);
    hardwareToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    rightLayout->addWidget(hardwareToggle);
    auto *hardware = new QWidget(right);
    auto *hardwareLayout = new QVBoxLayout(hardware);
    hardwareLayout->setContentsMargins(0, 0, 0, 0);
    for (const QString &text : {tr("控制盒指示灯：未接入"), tr("控制盒蜂鸣器：未接入"),
                                tr("呼吸门控：未接入"), tr("门控硬件联动：未接入")})
        hardwareLayout->addWidget(label(text, "reserved", hardware));
    rightLayout->addWidget(hardware);
    hardware->hide();
    connect(hardwareToggle, &QToolButton::toggled, this, [hardwareToggle, hardware](bool expanded) {
        hardware->setVisible(expanded);
        hardwareToggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    });
    auto *supply = findChild<QLabel *>(QStringLiteral("labelBeamDeliveryState"));
    move(supply, doseCardLayout);
    doseCardLayout->removeWidget(supply);
    doseCardLayout->insertWidget(0, supply);
    supply->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    supply->setProperty("clinicalRole", "supply");
    auto *connectionTitle = label(tr("中间层连接"), "section", right);
    rightLayout->addWidget(connectionTitle);
    auto *connection = label(tr("待连接"), "muted", right);
    connection->setObjectName(QStringLiteral("clinicalConnection"));
    rightLayout->addWidget(connection);
    auto *debug = new QToolButton(right);
    debug->setObjectName(QStringLiteral("clinicalDebugDetails"));
    debug->setProperty("sessionReadOnly", true);
    debug->setText(tr("调试明细"));
    debug->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    debug->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    debug->setPopupMode(QToolButton::InstantPopup);
    auto *menu = new QMenu(debug);
    debug->setMenu(menu);
    // Keep diagnostic gRPC history available, without a telemetry-detail entry.
    menu->addAction(tr("gRPC 通知记录"), this, [this] {
        auto *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle(tr("gRPC 通知记录（只读）"));
        auto *layout = new QVBoxLayout(dialog);
        auto *text = new QPlainTextEdit(dialog);
        text->setReadOnly(true);
        text->setPlainText(m_grpcCallHistory.join(QLatin1Char('\n')));
        layout->addWidget(text);
        dialog->resize(760, 560);
        dialog->show();
    });
    auto *logsCard = new QGroupBox(tr("系统日志 / 调试"), right);
    logsCard->setObjectName(QStringLiteral("clinicalLogsCard"));
    auto *logsLayout = new QVBoxLayout(logsCard);
    logsLayout->setContentsMargins(8, 12, 8, 8);
    logsLayout->setSpacing(4);
    move(debug, logsLayout);
    rightLayout->addStretch(1);
    rightLayout->addWidget(logsCard);
    auto *rightScroll = scroll(right, QStringLiteral("clinicalDispatchScroll"));
    rightScroll->setMinimumWidth(380);
    rightScroll->setMaximumWidth(500);
    columns->addWidget(rightScroll);
    columns->setStretchFactor(0, 0);
    columns->setStretchFactor(1, 1);
    columns->setStretchFactor(2, 0);
    columns->setSizes({360, 1000, 400});
    connect(columns, &QSplitter::splitterMoved, this, [this] { updateClinicalDeviceLayout(); });
    rightLayout->removeWidget(connectionTitle);
    rightLayout->removeWidget(connection);
    // Move these data-bound widgets after the top safety layout is created.

    // Reuse the existing live interlock indicators without changing control gates.
    ui->groupBoxSystemStatus->show();
    ui->groupBoxBedPosition->hide();
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    for (QLabel *value : {ui->label_posa, ui->label_posb, ui->label_posc,
                          ui->label_posx, ui->label_posy, ui->label_posz}) {
        value->setFont(mono);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        value->setText(QStringLiteral("—"));
    }
    for (QLineEdit *edit : {ui->lineEdit_a, ui->lineEdit_b, ui->lineEdit_c,
                            ui->lineEdit_x, ui->lineEdit_y, ui->lineEdit_z}) {
        edit->setFont(mono);
        edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    for (QLabel *value : {ui->patient_name, ui->patient_id}) {
        value->setWordWrap(true);
        value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        value->setProperty("clinicalRole", "identity");
    }
    for (QLabel *value : {ui->label_38, ui->label_39}) {
        value->setMinimumSize(108, 46);
        value->setMaximumHeight(QWIDGETSIZE_MAX);
        value->setWordWrap(true);
    }
    auto *position = findChild<QWidget *>(QStringLiteral("bedPositionPanel"));
    position->setMinimumWidth(328);
    position->setMaximumWidth(352);
    ui->label_38->setText(tr("实际值\n原接口单位"));
    ui->label_39->setText(tr("设置值\n原接口单位"));
    ui->label_39->setToolTip(tr("保留可编辑的设置位置；尚未确认计划床坐标字段，不能等同为计划值。"));
    // Keep the workflow visible; detailed graphics share a persistent detail window.
    auto *overviewScroll = qobject_cast<QScrollArea *>(m_executionTabs->widget(0));
    overviewScroll->takeWidget();
    m_executionTabs->removeTab(0);
    delete overviewScroll;
    centerLayout->removeWidget(m_executionTabs);
    auto *detailDialog = new QDialog(this);
    detailDialog->setObjectName(QStringLiteral("clinicalDetailDialog"));
    detailDialog->setWindowTitle(tr("治疗详情"));
    detailDialog->resize(1120, 760);
    auto *detailLayout = new QVBoxLayout(detailDialog);
    m_executionTabs->setParent(detailDialog);
    detailLayout->addWidget(m_executionTabs);
    auto *closeDetails = new QPushButton(tr("关闭"), detailDialog);
    detailLayout->addWidget(closeDetails, 0, Qt::AlignRight);
    connect(closeDetails, &QPushButton::clicked, detailDialog, &QDialog::hide);

    root->removeWidget(ui->groupBoxTreatmentFlow);
    move(ui->groupBoxTreatmentFlow, centerLayout);
    centerLayout->removeWidget(ui->groupBoxTreatmentFlow);
    // Put the five-stage progress and alerts immediately AFTER the blue
    // "流程状态" card title, not above it.
    centerLayout->insertWidget(1, ui->groupBoxTreatmentFlow);
    auto *detailTools = new QHBoxLayout;
    detailTools->setSpacing(6);
    detailTools->addStretch();
    // Only expose the execution/spot-detail entry in the workflow toolbar.
    // Keep the existing plan/bed tabs and state updates intact internally.
    auto *executionDetailsButton = new QPushButton(tr("束斑 / 执行详情"), center);
    executionDetailsButton->setObjectName(QStringLiteral("clinicalExecutionDetailsButton"));
    executionDetailsButton->setProperty("sessionReadOnly", true);
    detailTools->addWidget(executionDetailsButton);
    connect(executionDetailsButton, &QPushButton::clicked, this, [this, detailDialog] {
        auto *deliveryScroll = findChild<QScrollArea *>(
            QStringLiteral("clinicalDeliveryScroll"));
        if (deliveryScroll && m_executionTabs->indexOf(deliveryScroll) >= 0)
            m_executionTabs->setCurrentWidget(deliveryScroll);
        detailDialog->show();
        detailDialog->raise();
        detailDialog->activateWindow();
    });
    // The card order is: blue workflow title, five-stage progress/alerts,
    // read-only detail navigation, then the existing workflow canvas.
    centerLayout->insertLayout(2, detailTools);
    auto *canvas = new QWidget(center);
    canvas->setObjectName(QStringLiteral("clinicalWorkflowCanvas"));
    auto *canvasGrid = new QGridLayout(canvas);
    canvasGrid->setObjectName(QStringLiteral("clinicalWorkflowCanvasGrid"));
    canvasGrid->setContentsMargins(0, 4, 0, 4);
    canvasGrid->setSpacing(8);
    auto *deviceRail = new QWidget(canvas);
    deviceRail->setObjectName(QStringLiteral("clinicalDeviceRail"));
    auto *deviceLayout = new QVBoxLayout(deviceRail);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    move(findChild<QGroupBox *>(QStringLiteral("groupBoxDeviceReadiness")), deviceLayout);
    deviceLayout->addStretch();

    auto *bedCard = new QGroupBox(tr("治疗床（六自由度）"), canvas);
    bedCard->setObjectName(QStringLiteral("clinicalBedCard"));
    bedCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    auto *bedCardLayout = new QVBoxLayout(bedCard);
    bedCardLayout->setContentsMargins(8, 12, 8, 8);
    bedCardLayout->setSpacing(4);
    position->setMinimumWidth(0);
    position->setMaximumWidth(QWIDGETSIZE_MAX);
    move(position, bedCardLayout);
    bedCard->setMinimumWidth(240);
    bedCard->setMaximumWidth(300);
    for (QLabel *heading : {ui->label_38, ui->label_39}) heading->setMinimumWidth(84);

    overview->setParent(canvas); // Do not alter WorkflowOverview's geometry or painting.
    canvasGrid->addWidget(bedCard, 0, 0, Qt::AlignVCenter);
    canvasGrid->addWidget(overview, 0, 1, Qt::AlignCenter);
    canvasGrid->addWidget(deviceRail, 0, 2, Qt::AlignVCenter);
    position->show();
    overview->show();
    auto *workflowScroll = scroll(canvas, QStringLiteral("clinicalWorkflowScroll"));
    workflowScroll->setWidgetResizable(true);
    centerLayout->addWidget(workflowScroll, 1);

    // Top safety card reuses existing live system and gantry/interlock readbacks.
    // No safety state is fabricated and no interlock behavior is modified.
    auto *workspace = panel(QStringLiteral("clinicalMainWorkspace"), columns);
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(6);
    auto *safetyCard = new QGroupBox(tr("安全联锁与物理指示"), workspace);
    safetyCard->setObjectName(QStringLiteral("clinicalSafetyCard"));
    auto *safetyLayout = new QVBoxLayout(safetyCard);
    safetyCard->setTitle(QString());
    safetyLayout->setContentsMargins(6, 5, 6, 5);
    safetyLayout->setSpacing(5);
    // Safety/interlocks are a standalone card, not part of the workflow card.
    safetyLayout->addWidget(monitorCardBar(safetyCard, tr("安全联锁与物理指示"),
                                            QStringLiteral("✚")));
    move(ui->groupBoxSystemStatus, safetyLayout);
    move(auxiliary, safetyLayout);
    // On startup, the alert may have been created before the safety card.
    // Move that SAME live label here; do not duplicate or change alarm logic.
    if (auto *attention = findChild<QLabel *>(QStringLiteral("labelTreatmentAttention"))) {
        const bool explicitlyVisible = !attention->isHidden();
        ui->verticalLayout_flowBox->removeWidget(attention);
        attention->setParent(safetyCard);
        safetyLayout->addWidget(attention);
        attention->setVisible(explicitlyVisible);
    }
    move(faultDisplay, safetyLayout);
    // Hide the middleware connection display, not its status tracking.
    // Both labels remain findable for the existing status-refresh code.
    connectionTitle->setParent(legacy);
    connectionTitle->hide();
    connection->setParent(legacy);
    connection->hide();
    // Two independent cards are vertically stacked in the center column.
    // This wrapper is only a transparent layout widget, never a titled card.
    auto *safetyWorkflowStack = new QWidget(workspace);
    safetyWorkflowStack->setObjectName(QStringLiteral("clinicalSafetyWorkflowStack"));
    safetyWorkflowStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *safetyWorkflowLayout = new QVBoxLayout(safetyWorkflowStack);
    safetyWorkflowLayout->setContentsMargins(0, 0, 0, 0);
    safetyWorkflowLayout->setSpacing(9);
    safetyWorkflowLayout->addWidget(safetyCard, 0);
    safetyWorkflowLayout->addWidget(center, 1);

    // Middle column: safety card above workflow card. Right column: dispatch.
    auto *workSplit = new QSplitter(Qt::Horizontal, workspace);
    workSplit->setObjectName(QStringLiteral("clinicalLowerColumns"));
    workSplit->setChildrenCollapsible(false);
    workSplit->setHandleWidth(8);
    auto *dispatchFrame = new QGroupBox(tr("治疗监控与束流调度"), workspace);
    dispatchFrame->setObjectName(QStringLiteral("clinicalDispatchFrame"));
    dispatchFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto *dispatchFrameLayout = new QVBoxLayout(dispatchFrame);
    dispatchFrame->setTitle(QString());
    dispatchFrameLayout->setContentsMargins(7, 7, 7, 8);
    dispatchFrameLayout->setSpacing(6);
    dispatchFrameLayout->addWidget(monitorCardBar(dispatchFrame,
                                                  tr("治疗监控与束流调度"),
                                                  QStringLiteral("◈"), true));
    rightScroll->setParent(dispatchFrame);
    dispatchFrameLayout->addWidget(rightScroll, 1);
    dispatchFrame->setMinimumWidth(392);
    dispatchFrame->setMaximumWidth(530);

    workSplit->addWidget(safetyWorkflowStack);
    workSplit->addWidget(dispatchFrame);
    workSplit->setStretchFactor(0, 1);
    workSplit->setStretchFactor(1, 0);
    workSplit->setSizes({1500, 460});
    workspaceLayout->addWidget(workSplit, 1);
    columns->addWidget(workspace);
    columns->setStretchFactor(0, 0);
    columns->setStretchFactor(1, 1);
    columns->setSizes({480, 1950});
    connect(workSplit, &QSplitter::splitterMoved, this, [this] { updateClinicalDeviceLayout(); });
    m_clinicalFaultTimer = new QTimer(this);
    m_clinicalFaultTimer->setSingleShot(true);
    m_clinicalFaultTimer->setInterval(600);
    connect(m_clinicalFaultTimer, &QTimer::timeout, this, &TreatmentWindow::showClinicalFaults);
    updateClinicalDeviceLayout();
    updateClinicalWorkspace();
}

void TreatmentWindow::updateClinicalDeviceLayout()
{
    if (!m_clinicalBody) return;
    auto *center = findChild<QWidget *>(QStringLiteral("clinicalExecutionColumn"));
    const bool narrowWindow = this->width() < 1600;
    auto *patient = findChild<QWidget *>(QStringLiteral("clinicalPatientColumn"));
    auto *dispatch = findChild<QWidget *>(QStringLiteral("clinicalDispatchScroll"));
    if (!center || !center->layout() || !patient || !dispatch) return;
    const int patientMaximum = narrowWindow ? 400 : 520;
    const int dispatchMaximum = narrowWindow ? 400 : 500;
    if (patient->maximumWidth() != patientMaximum) patient->setMaximumWidth(patientMaximum);
    if (dispatch->maximumWidth() != dispatchMaximum) dispatch->setMaximumWidth(dispatchMaximum);
    if (auto *frame = findChild<QGroupBox *>(QStringLiteral("clinicalDispatchFrame"))) {
        const int frameMaximum = dispatchMaximum + 24;
        if (frame->maximumWidth() != frameMaximum) frame->setMaximumWidth(frameMaximum);
    }
    const int width = center->width();
    const bool compact = width < 1000;
    center->layout()->setSpacing(compact ? 4 : 10);
    if (auto *auxiliary = findChild<QGridLayout *>(QStringLiteral("clinicalAuxiliaryLayout")))
        auxiliary->setVerticalSpacing(compact ? 2 : 6);
    if (auto *position = findChild<QGridLayout *>(QStringLiteral("clinicalPositionGrid")))
        position->setVerticalSpacing(compact ? 4 : 7);
    ui->label_preview_title->setVisible(!compact);
    ui->groupBoxBedPreview->setMinimumHeight(compact ? 360 : 420);
    if (auto *grid = findChild<QGridLayout *>(QStringLiteral("clinicalWorkflowCanvasGrid"))) {
        const bool wide = width >= 1200;
        auto *canvas = grid->parentWidget();
        if (!canvas->property("wideLayout").isValid() || canvas->property("wideLayout").toBool() != wide) {
            canvas->setProperty("wideLayout", wide);
            auto *position = findChild<QGroupBox *>(QStringLiteral("clinicalBedCard"));
            auto *ring = findChild<QWidget *>(QStringLiteral("workflowOverview"));
            auto *rail = findChild<QWidget *>(QStringLiteral("clinicalDeviceRail"));
            grid->removeWidget(position);
            grid->removeWidget(ring);
            grid->removeWidget(rail);
            for (int col = 0; col < 3; ++col) grid->setColumnStretch(col, 0);
            if (wide) {
                grid->addWidget(position, 0, 0, Qt::AlignVCenter);
                grid->addWidget(ring, 0, 1, Qt::AlignCenter);
                grid->addWidget(rail, 0, 2, Qt::AlignVCenter);
                grid->setColumnStretch(1, 1);
                rail->setMaximumWidth(260);
            } else {
                grid->addWidget(ring, 0, 0, 1, 2, Qt::AlignHCenter);
                grid->addWidget(position, 1, 0, Qt::AlignTop);
                grid->addWidget(rail, 1, 1, Qt::AlignTop);
                grid->setColumnStretch(1, 1);
                rail->setMaximumWidth(QWIDGETSIZE_MAX);
            }
        }
    }
    const int columnCount = 1;
    const QVector<QLabel *> devices = {ui->labelCouchDeviceStatus, ui->labelDrDeviceStatus,
        ui->labelIgrtDeviceStatus, ui->labelFilterDeviceStatus, ui->labelPlcDeviceStatus};
    int index = 0;
    ui->gridLayout_deviceStatus->removeWidget(ui->labelDeviceStatusCaption);
    ui->gridLayout_deviceStatus->addWidget(ui->labelDeviceStatusCaption, 0, 0, 1, columnCount);
    for (QLabel *device : devices) {
        ui->gridLayout_deviceStatus->removeWidget(device);
        const bool available = device != ui->labelDrDeviceStatus || RoomCapabilities::hasDrLift(m_roomId);
        device->setVisible(available);
        if (!available) continue;
        device->setProperty("clinicalDevice", true);
        if (device->property("clinicalCompact").toBool() != compact) {
            device->setProperty("clinicalCompact", compact);
            device->style()->unpolish(device);
            device->style()->polish(device);
        }
        device->setMinimumHeight(54);
        device->setMinimumWidth(0);
        device->setWordWrap(true);
        device->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        if (device->property("deviceName").isValid())
            device->setText(device->property("deviceName").toString() + QStringLiteral(" · ")
                + device->property("deviceState").toString() + (compact ? QString()
                    : QLatin1Char('\n') + device->property("deviceConnection").toString()));
        device->setTextFormat(Qt::PlainText);
        ui->gridLayout_deviceStatus->addWidget(device, index / columnCount + 1, index % columnCount);
        ++index;
    }
    for (int column = 0; column < 5; ++column)
        ui->gridLayout_deviceStatus->setColumnStretch(column, column < columnCount ? 1 : 0);
    if (auto *grid = findChild<QGridLayout *>(QStringLiteral("clinicalDeliveryGrid"))) {
        const bool sideBySide = false;
        auto *live = findChild<QGroupBox *>(QStringLiteral("groupBoxLiveDose"));
        const bool mapInDelivery = ui->spotMapWidget->parentWidget() == grid->parentWidget();
        const bool mapOnly = true;
        const QString layoutKey = QString::number(sideBySide) + QLatin1Char('/') + QString::number(mapOnly)
            + QLatin1Char('/') + QString::number(mapInDelivery);
        if (auto *metrics = findChild<QWidget *>(QStringLiteral("widgetBeamCoreMetrics"))) {
            auto *metricGrid = qobject_cast<QGridLayout *>(metrics->layout());
            const bool stacked = true;
            if (!metricGrid->property("stacked").isValid()
                || metricGrid->property("stacked").toBool() != stacked) {
                metricGrid->setProperty("stacked", stacked);
                const QVector<QLabel *> captions = {ui->labelDoseDeliveryProgressName,
                    findChild<QLabel *>(QStringLiteral("clinicalRateCaption")), ui->labelLayerProgressName};
                const QVector<QLabel *> values = {ui->labelDoseDeliveryProgressValue,
                    findChild<QLabel *>(QStringLiteral("labelBeamDoseRateValue")), ui->labelLayerProgressValue};
                for (int index = 0; index < values.size(); ++index) {
                    metricGrid->removeWidget(captions.at(index));
                    metricGrid->removeWidget(values.at(index));
                    const int row = stacked && index > 0 ? 2 : 0;
                    const int column = stacked ? qMax(0, index - 1) : index;
                    const int span = stacked && index == 0 ? 2 : 1;
                    metricGrid->addWidget(captions.at(index), row, column, 1, span);
                    metricGrid->addWidget(values.at(index), row + 1, column, 1, span);
                }
                metricGrid->setColumnStretch(0, stacked ? 1 : 2);
                metricGrid->setColumnStretch(1, 1);
                metricGrid->setColumnStretch(2, stacked ? 0 : 1);
            }
        }
        if (grid->property("layoutKey").toString() == layoutKey) return;
        grid->setProperty("layoutKey", layoutKey);
        // Dose monitoring stays in the right-hand sidebar.
        if (mapInDelivery) {
            grid->removeWidget(ui->spotMapWidget);
            grid->addWidget(ui->spotMapWidget, mapOnly ? 0 : sideBySide ? 0 : 1,
                            mapOnly ? 0 : sideBySide ? 1 : 0, 1, mapOnly ? 2 : 1);
        }
        grid->setColumnStretch(0, mapInDelivery && sideBySide && !mapOnly ? 2 : 1);
        grid->setColumnStretch(1, mapInDelivery && sideBySide && !mapOnly ? 3 : 0);
    }
}

void TreatmentWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (m_clinicalBody) QTimer::singleShot(0, this, &TreatmentWindow::updateClinicalDeviceLayout);
}

void TreatmentWindow::updateClinicalWorkspace()
{
    if (!m_clinicalBody) return;
    for (QTextEdit *edit : ui->widgetPlanSummary->findChildren<QTextEdit *>()) {
        if (auto *source = findChild<QLabel *>(edit->property("sourceLabel").toString()))
            static_cast<ClinicalReadbackView *>(edit)->setReadback(source->text().trimmed().isEmpty()
                ? tr("未提供") : source->text());
    }
    const bool connected = m_webSocketClient && m_webSocketClient->isConnected();
    updateBeamRoomOverview();
    if (!connected || m_workflowSyncRequired) {
        ui->label_11->setText(tr("当前分配：%1（上次反馈，待核实）\n您的排位：未接入")
            .arg(m_activeBeamRoomId.isEmpty() ? tr("未知") : m_activeBeamRoomId));
        for (QLabel *room : std::as_const(m_beamRoomLabels)) {
            room->setText(tr("治疗室%1\n占用状态待同步").arg(room->property("roomNumber").toInt()));
            setTreatmentRole(room, QStringLiteral("idleStyle"));
        }
    }
    ui->label_7->setText(m_roomId.trimmed().isEmpty() ? tr("未分配") : m_roomId);
    ui->label_room->setText(tr("流程：%1").arg(m_workflowReauthPending ? tr("等待登录") : m_workflowSyncRequired
        ? tr("现场状态待同步") : treatmentStateText(m_treatmentState)));
    findChild<QLabel *>(QStringLiteral("clinicalConnection"))->setText(
        connected ? tr("已连接") : tr("连接中断 · 现场读数待核实"));
    auto *supply = findChild<QLabel *>(QStringLiteral("labelBeamDeliveryState"));
    const qint64 maxAgeMs = m_settings->value(QStringLiteral("Logout/BeamStatusMaxAgeMs"), 5000).toLongLong();
    const bool delivering = connected && !m_workflowSyncRequired && m_logoutState.deliveryFresh(maxAgeMs)
        && (m_logoutState.deliveryResult == 1 || (m_logoutState.liveBeam.isBool()
            && m_logoutState.liveBeam.toBool() && m_logoutState.beamAge.isValid()
            && m_logoutState.beamAge.elapsed() < maxAgeMs));
    if (supply->property("clinicalBeamOn").toBool() != delivering) {
        supply->setProperty("clinicalBeamOn", delivering);
        supply->style()->unpolish(supply);
        supply->style()->polish(supply);
    }
    const int step = treatmentStateToStep(m_treatmentState);
    const bool planPhase = !m_workflowSyncRequired && m_treatmentState != TreatmentState::ErrorStopped && step == 0;
    const bool bedPhase = !m_workflowSyncRequired && m_treatmentState != TreatmentState::ErrorStopped
        && step >= 1 && step <= 2;
    const int defaultPage = planPhase ? 0 : bedPhase ? 1 : 2;
    if (auto *widget = findChild<QWidget *>(QStringLiteral("workflowOverview"))) {
        QString state = m_backendWorkflowState;
        if (!m_startupState.initialized) state = QStringLiteral("INITIALIZING");
        else if (!m_startupState.authenticated) state = QStringLiteral("WAITING_LOGIN");
        else if (m_workflowSyncRequired) state = QStringLiteral("SYNC_PENDING");
        else if (m_treatmentState == TreatmentState::ErrorStopped) state = QStringLiteral("ERROR_STOPPED");
        else if (state.isEmpty()) {
            switch (m_treatmentState) {
            case TreatmentState::Idle: state = "WAITING_OIS_PATIENT_CALL"; break;
            case TreatmentState::PlanConfirm: state = "WAITING_FRONTEND_PLAN_CONFIRM"; break;
            case TreatmentState::Preparing: state = "WAITING_DEVICE_AUTOPOS_DONE"; break;
            case TreatmentState::ReadyToApplyBeam: state = "WAITING_FRONTEND_BEAM_APPLY"; break;
            case TreatmentState::BeamRequested: state = "WAITING_DEVICE_BEAM_READY"; break;
            case TreatmentState::BeamReady: state = "BEAM_READY"; break;
            case TreatmentState::Treating: state = "TREATMENT_IN_PROGRESS"; break;
            case TreatmentState::Paused: state = "TREATMENT_PAUSED"; break;
            case TreatmentState::BeamFinished: state = "BEAM_FINISHED"; break;
            case TreatmentState::ReportConfirm: state = "WAITING_CHART_GENERATION"; break;
            case TreatmentState::Completed: state = "COMPLETED"; break;
            case TreatmentState::ErrorStopped: break;
            }
        }
        const QString action = state == "INITIALIZING" ? tr("正在初始化")
            : state == "WAITING_LOGIN" ? tr("等待操作员登录")
            : state == "SYNC_PENDING" ? tr("等待现场同步") : currentWorkflowActionText();
        static_cast<WorkflowOverview *>(widget)->setState(state, action);
    }
    if (auto *heading = findChild<QLabel *>(QStringLiteral("clinicalStageHeading"))) {
        heading->setText(m_workflowSyncRequired ? tr("现场状态待同步")
            : m_treatmentState == TreatmentState::ErrorStopped ? tr("异常现场 / 已有回读")
            : planPhase ? tr("计划核对 / 射野预览") : bedPhase ? tr("设备准备 / 摆位配准")
            : step == 4 ? tr("结束 / 治疗记录") : tr("出束执行 / 计量反馈"));
    }
    auto *mapHost = findChild<QWidget *>(QStringLiteral("clinicalPlanMapHost"));
    auto *deliveryGrid = findChild<QGridLayout *>(QStringLiteral("clinicalDeliveryGrid"));
    QWidget *mapParent = planPhase ? mapHost : deliveryGrid->parentWidget();
    const bool mapMoved = ui->spotMapWidget->parentWidget() != mapParent;
    if (mapMoved) {
        ui->spotMapWidget->setParent(mapParent);
        if (planPhase) static_cast<QVBoxLayout *>(mapHost->layout())->addWidget(ui->spotMapWidget);
        else deliveryGrid->addWidget(ui->spotMapWidget, 1, 0);
    }
    ui->spotMapWidget->setMaximumHeight(planPhase ? 640 : QWIDGETSIZE_MAX);
    const QString key = QString::number(static_cast<int>(m_treatmentState))
        + (m_workflowSyncRequired ? QStringLiteral("/sync") : QString());
    if (m_executionTabs->property("workflowView").toString() != key) {
        m_executionTabs->setProperty("workflowView", key);
        auto *details = findChild<QDialog *>(QStringLiteral("clinicalDetailDialog"));
        if (!details || !details->isVisible()) m_executionTabs->setCurrentIndex(defaultPage);
    }
    const bool idle = m_treatmentState == TreatmentState::Idle && m_currentPlanPayload.isEmpty();
    findChild<QLabel *>(QStringLiteral("clinicalWaiting"))->setVisible(idle);
    ui->spotMapWidget->setVisible(!idle);
    mapHost->setVisible(!idle);
    if (auto *waiting = findChild<QLabel *>(QStringLiteral("clinicalPlanWaiting"))) waiting->setVisible(idle);
    const bool monitorPhase = m_workflowSyncRequired || m_treatmentState == TreatmentState::ErrorStopped
        || step >= 2;
    auto *live = findChild<QGroupBox *>(QStringLiteral("groupBoxLiveDose"));
    const bool visibilityChanged = live->isHidden();
    live->show();
    if (visibilityChanged || mapMoved) updateClinicalDeviceLayout();
    findChild<QWidget *>(QStringLiteral("clinicalDoseChannels"))->setVisible(monitorPhase);
    if (m_currentPlanPayload != m_clinicalDisplayedPlan) {
        m_clinicalDisplayedPlan = m_currentPlanPayload;
        m_fieldSummary->clear();
        const PlanDisplayInfo plan = parsePlanDisplayInfo(m_currentPlanPayload);
        qint64 totalLayers = 0, totalSpots = 0;
        bool layersKnown = !plan.beams.isEmpty(), spotsKnown = !plan.beams.isEmpty();
        for (const auto &value : plan.beams) {
            const auto beam = value.toObject();
            const int layers = readIntValue(beam, {"energy_layers"});
            const int spots = readIntValue(beam, {"total_scan_spots"});
            layersKnown = layersKnown && layers >= 0;
            spotsKnown = spotsKnown && spots >= 0;
            if (layers >= 0) totalLayers += layers;
            if (spots >= 0) totalSpots += spots;
        }
        findChild<QLabel *>(QStringLiteral("clinicalBeamCount"))->setText(plan.beams.isEmpty()
            ? tr("未提供") : QString::number(plan.beams.size()));
        findChild<QLabel *>(QStringLiteral("clinicalLayerCount"))->setText(layersKnown ? QString::number(totalLayers) : tr("未提供"));
        findChild<QLabel *>(QStringLiteral("clinicalSpotCount"))->setText(spotsKnown ? QString::number(totalSpots) : tr("未提供"));
        for (const QJsonValue &item : plan.beams) {
            const QJsonObject beam = item.toObject();
            const auto count = [&beam](const char *key) {
                const int value = readIntValue(beam, {key});
                return value >= 0 ? QString::number(value) : tr("未提供");
            };
            const QString number = count("beam_number");
            auto *field = new QTreeWidgetItem(m_fieldSummary, {tr("射野 %1  %2").arg(number,
                beam.value(QStringLiteral("beam_name")).toString())});
            new QTreeWidgetItem(field, {tr("能层 %1 · Spot %2")
                .arg(count("energy_layers"), count("total_scan_spots"))});
            const QJsonValue mu = beam.value(QStringLiteral("beam_meterset"));
            new QTreeWidgetItem(field, {mu.isDouble() ? tr("计划计量 %1 MU").arg(mu.toDouble(), 0, 'f', 2)
                                                       : tr("计划计量：未提供")});
            const QJsonValue angle = beam.value(QStringLiteral("gantry_angle_deg"));
            new QTreeWidgetItem(field, {angle.isDouble() && std::isfinite(angle.toDouble())
                ? tr("治疗头角度：%1°").arg(angle.toDouble(), 0, 'f', 2)
                : tr("治疗头角度：未提供")});
            field->setExpanded(true);
        }
        if (plan.beams.isEmpty()) new QTreeWidgetItem(m_fieldSummary, {tr("等待射野计划")});
    }
}

void TreatmentWindow::queueClinicalFaults(const QStringList &issues)
{
    m_clinicalFaults = issues;
    QSet<QString> current;
    for (const QString &issue : issues) current.insert(issue);
    m_announcedClinicalFaults.intersect(current);
    if (current.isEmpty()) {
        if (m_clinicalFaultDialog) m_clinicalFaultDialog->close();
        return;
    }
    current.subtract(m_announcedClinicalFaults);
    if (!current.isEmpty() && m_clinicalFaultTimer && !m_clinicalFaultTimer->isActive())
        m_clinicalFaultTimer->start();
}

void TreatmentWindow::showClinicalFaults()
{
    if (m_closing || !isVisible() || m_workflowReauthPending || m_loginTransitionPending) return;
    if (QApplication::activeModalWidget()) {
        m_clinicalFaultTimer->start(1000);
        return;
    }
    QStringList newFaults;
    for (const QString &issue : std::as_const(m_clinicalFaults)) {
        if (!m_announcedClinicalFaults.contains(issue)) newFaults.append(issue);
    }
    if (newFaults.isEmpty()) return;
    for (const QString &issue : newFaults) m_announcedClinicalFaults.insert(issue);
    if (!m_clinicalFaultDialog) {
        auto *dialog = new QDialog(this, Qt::Tool);
        m_clinicalFaultDialog = dialog;
        dialog->setWindowTitle(tr("设备异常提醒"));
        dialog->setObjectName(QStringLiteral("clinicalFaultDialog"));
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setAttribute(Qt::WA_ShowWithoutActivating);
        auto *layout = new QVBoxLayout(dialog);
        auto *text = new QPlainTextEdit(dialog);
        text->setObjectName(QStringLiteral("clinicalFaultText"));
        text->setReadOnly(true);
        text->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        text->setMaximumBlockCount(2000);
        layout->addWidget(text);
        auto *ack = new QPushButton(tr("已知晓"), dialog);
        connect(ack, &QPushButton::clicked, dialog, &QDialog::close);
        layout->addWidget(ack, 0, Qt::AlignRight);
        dialog->resize(560, 320);
    }
    m_clinicalFaultDialog->findChild<QPlainTextEdit *>(QStringLiteral("clinicalFaultText"))->setPlainText(
        m_clinicalFaults.join(QStringLiteral("\n\n")) + tr("\n\n已知晓仅关闭提示，不复位故障、不改变设备状态。"));
    m_clinicalFaultDialog->show();
}

void TreatmentWindow::initTables()
{
    auto setupTable = [](QTableWidget *table) {
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setShowGrid(true);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setMinimumHeight(36);
    };

    setupTable(ui->tableWidget);
    setupTable(ui->tableWidget_2);
    setupTable(ui->tableWidget_3);

    ui->tableWidget->setColumnCount(1);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << tr("射野 MU"));

    ui->tableWidget_2->setColumnCount(1);
    ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << tr("场实现"));

    ui->tableWidget_3->setColumnCount(1);
    ui->tableWidget_3->setHorizontalHeaderLabels(QStringList() << tr("场实现"));
}

void TreatmentWindow::initStatusIndicators()
{
    ui->gridLayout_deviceStatus->setColumnStretch(0, 1);
    ui->gridLayout_deviceStatus->setColumnStretch(1, 1);

    setStatusDot(ui->label_9, StatusTone::Neutral, tr("等待设备状态"));
    setStatusDot(ui->label_35, StatusTone::Neutral, tr("等待连接状态"));
    setStatusDot(ui->label_46, StatusTone::Neutral, tr("等待束流联锁状态"));
    setStatusDot(ui->label_47, StatusTone::Neutral, tr("等待治疗床联锁状态"));

    setStatusPill(ui->labelCouchDeviceStatus,
                  tr("治疗床"), tr("等待数据"), StatusTone::Neutral);
    setStatusPill(ui->labelDrDeviceStatus,
                  tr("DR"), tr("等待数据"), StatusTone::Neutral);
    setStatusPill(ui->labelIgrtDeviceStatus,
                  tr("IGRT"), tr("等待数据"), StatusTone::Neutral);
    setStatusPill(ui->labelFilterDeviceStatus,
                  tr("脊形过滤器"), tr("等待数据"), StatusTone::Neutral);
    setStatusPill(ui->labelPlcDeviceStatus,
                  tr("PLC"), tr("等待数据"), StatusTone::Neutral);
    m_currentDeviceActionText = tr("等待设备状态");
    m_currentDeviceIssues.clear();
    updateTreatmentActionDisplay();
}

void TreatmentWindow::setupDoseInfoPanel()
{
    if (!ui->verticalLayout_bedBox || !m_doseInfoValueLabels.isEmpty()) {
        return;
    }

    // Replace only the treatment MU progress bar's visual type; existing code
    // continues to update the same ui->progressBarDoseDelivery pointer.
    auto *oldMuProgress = ui->progressBarDoseDelivery;
    auto *roundMuProgress = new CircularDoseProgressBar(oldMuProgress->parentWidget());
    roundMuProgress->setObjectName(oldMuProgress->objectName());
    roundMuProgress->setRange(oldMuProgress->minimum(), oldMuProgress->maximum());
    roundMuProgress->setValue(oldMuProgress->value());
    roundMuProgress->setFormat(oldMuProgress->format());
    roundMuProgress->setToolTip(oldMuProgress->toolTip());
    ui->horizontalLayoutDoseDeliveryProgress->removeWidget(oldMuProgress);
    ui->horizontalLayoutDoseDeliveryProgress->addWidget(roundMuProgress, 0, Qt::AlignHCenter);
    ui->progressBarDoseDelivery = roundMuProgress;
    delete oldMuProgress;

    ui->groupBoxBedPosition->setTitle(QString());
    ui->verticalLayout_bedBox->setContentsMargins(12, 8, 12, 12);
    ui->verticalLayout_bedBox->setSpacing(8);
    ui->verticalLayout_bedBox->setAlignment(Qt::AlignTop);

    if (ui->gridLayoutDoseInfo) {
        ui->gridLayoutDoseInfo->setVerticalSpacing(8);
        ui->gridLayoutDoseInfo->setAlignment(Qt::AlignTop);
        ui->gridLayoutDoseInfo->setColumnStretch(0, 1);
        ui->gridLayoutDoseInfo->setColumnStretch(1, 1);

        auto *doseSectionLabel = new QLabel(tr("剂量信息"), ui->groupBoxBedPosition);
        doseSectionLabel->setObjectName(QStringLiteral("labelDoseSectionTitle"));
        setTreatmentRole(doseSectionLabel, QStringLiteral("treatmentStyle15"));
        doseSectionLabel->setMinimumHeight(qMax(22, doseSectionLabel->fontMetrics().lineSpacing() + 4));
        doseSectionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto *beamSectionLabel = new QLabel(tr("束流结构信息"), ui->groupBoxBedPosition);
        beamSectionLabel->setObjectName(QStringLiteral("labelBeamSectionTitle"));
        setTreatmentRole(beamSectionLabel, QStringLiteral("treatmentStyle16"));
        beamSectionLabel->setMinimumHeight(qMax(22, beamSectionLabel->fontMetrics().lineSpacing() + 4));
        beamSectionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        ui->gridLayoutDoseInfo->addWidget(doseSectionLabel, 0, 0, 1, 2);
        ui->gridLayoutDoseInfo->addWidget(ui->doseDeviationCard, 1, 0, 1, 2);
        auto *liveLayout = findChild<QGridLayout *>(QStringLiteral("gridLayoutLiveDose"));
        if (liveLayout) {
            doseSectionLabel->setText(tr("出束监测"));
            doseSectionLabel->setObjectName(QStringLiteral("labelLiveDoseTitle"));
            ui->gridLayoutDoseInfo->removeWidget(doseSectionLabel);
            for (QWidget *card : {ui->doseDeviationCard, ui->doseDeliveryProgressCard, ui->doseBeamMuCard,
                                  ui->doseLayerProgressCard})
                ui->gridLayoutDoseInfo->removeWidget(card);
            liveLayout->addWidget(doseSectionLabel, 0, 0);
            liveLayout->addWidget(ui->doseDeliveryProgressCard, 2, 0, 1, 2, Qt::AlignTop);
            liveLayout->addWidget(ui->doseBeamMuCard, 3, 0, Qt::AlignTop);
            liveLayout->addWidget(ui->doseLayerProgressCard, 3, 1, Qt::AlignTop);
            liveLayout->addWidget(ui->doseDeviationCard, 4, 0, 1, 2, Qt::AlignTop);
            ui->horizontalLayoutDoseDeliveryProgress->removeWidget(ui->progressBarDoseDelivery);
            ui->verticalLayoutDoseDeliveryProgress->removeWidget(ui->labelDoseDeliveryProgressName);
            ui->horizontalLayoutDoseDeliveryProgress->insertWidget(0, ui->labelDoseDeliveryProgressName, 1);
            ui->verticalLayoutDoseDeliveryProgress->addWidget(
                ui->progressBarDoseDelivery, 0, Qt::AlignHCenter);
        } else {
            ui->gridLayoutDoseInfo->addWidget(ui->doseDeliveryProgressCard, 2, 0, 1, 2);
        }
        const int detailRow = liveLayout ? 0 : 3;
        ui->gridLayoutDoseInfo->addWidget(beamSectionLabel, detailRow, 0, 1, 2);
        ui->gridLayoutDoseInfo->addWidget(ui->dosePrescriptionCard, detailRow + 1, 0);
        ui->gridLayoutDoseInfo->addWidget(ui->doseFractionCard, detailRow + 1, 1);
        ui->gridLayoutDoseInfo->addWidget(ui->doseCurrentFractionCard, detailRow + 2, 0);
        ui->gridLayoutDoseInfo->addWidget(ui->doseCompletedCard, detailRow + 2, 1);
        if (!liveLayout) {
            ui->gridLayoutDoseInfo->addWidget(ui->doseBeamMuCard, 6, 0, 1, 2);
            ui->gridLayoutDoseInfo->addWidget(ui->doseLayerProgressCard, 7, 0, 1, 2);
        }
        ui->gridLayoutDoseInfo->addWidget(ui->doseBeamInfoCard, liveLayout ? 3 : 8, 0, 1, 2);
    }

    ui->labelDosePanelTitle->hide();
    ui->labelDosePanelTitle->setFixedHeight(0);
    ui->labelDosePanelTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    ui->labelDoseDeviationName->setText(tr("治疗回读"));
    ui->labelDosePrescriptionName->setText(tr("治疗室 / 设备"));
    ui->labelDoseFractionName->setText(tr("扫描模式 / 粒子"));
    ui->labelDoseCurrentFractionName->setText(tr("Beam 数量"));
    ui->labelDoseCompletedName->setText(tr("能量层 / Spot"));
    ui->labelDoseBeamMuName->setText(tr("束流计量进度"));
    ui->labelLayerProgressName->setText(tr("能层进度"));
    ui->labelDoseDeliveryProgressName->setText(tr("剂量进度"));

    const QVector<QLabel*> fieldNameLabels = {
        ui->labelDoseDeviationName,
        ui->labelDosePrescriptionName,
        ui->labelDoseFractionName,
        ui->labelDoseCurrentFractionName,
        ui->labelDoseCompletedName,
        ui->labelDoseBeamMuName,
        ui->labelLayerProgressName,
        ui->labelBeamDoseInfoName,
        ui->labelDoseDeliveryProgressName
    };
    for (QLabel *label : fieldNameLabels) {
        if (!label) {
            continue;
        }
        label->setMinimumWidth(0);
        label->setMinimumHeight(qMax(20, label->fontMetrics().lineSpacing() + 3));
        label->setWordWrap(true);
        QSizePolicy labelPolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        labelPolicy.setHeightForWidth(true);
        label->setSizePolicy(labelPolicy);
    }

    const QVector<QLabel*> singleLineValueLabels = {
        ui->labelPrescriptionDose,
        ui->labelFractionDose,
        ui->labelCurrentFractionDose,
        ui->labelCompletedDose,
        ui->labelBeamMuDose
    };
    for (QLabel *label : singleLineValueLabels) {
        if (!label) {
            continue;
        }
        label->setMinimumWidth(0);
        label->setMinimumHeight(qMax(38, label->fontMetrics().lineSpacing() + 18));
        label->setMaximumHeight(QWIDGETSIZE_MAX);
        label->setWordWrap(true);
        QSizePolicy labelPolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        labelPolicy.setHeightForWidth(true);
        label->setSizePolicy(labelPolicy);
    }

    const int feedbackHeight = qMax(34, ui->labelDoseDeviation->fontMetrics().lineSpacing() + 14);
    ui->labelDoseDeviation->setMinimumWidth(0);
    ui->labelDoseDeviation->setMinimumHeight(feedbackHeight);
    ui->labelDoseDeviation->setMaximumHeight(QWIDGETSIZE_MAX);
    ui->labelDoseDeviation->setWordWrap(true);
    QSizePolicy feedbackPolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    feedbackPolicy.setHeightForWidth(true);
    ui->labelDoseDeviation->setSizePolicy(feedbackPolicy);
    ui->labelDoseDeviation->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->doseDeviationCard->setMinimumHeight(
        ui->labelDoseDeviationName->minimumHeight() + feedbackHeight + 5);
    ui->doseDeviationCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    const int beamInfoHeight = qMax(48, ui->labelBeamDoseInfo->fontMetrics().lineSpacing() * 2 + 18);
    ui->labelBeamDoseInfo->setMinimumWidth(0);
    ui->labelBeamDoseInfo->setMinimumHeight(beamInfoHeight);
    ui->labelBeamDoseInfo->setMaximumHeight(QWIDGETSIZE_MAX);
    ui->labelBeamDoseInfo->setWordWrap(true);
    ui->labelBeamDoseInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    const int progressHeight = qMax(32, ui->progressBarLayerProgress->fontMetrics().lineSpacing() + 14);
    // The circular MU gauge has its own square dimensions; retain the old
    // compact height only for the other two horizontal progress bars.
    for (QProgressBar *progressBar : {ui->progressBarBeamMeterset,
                                      ui->progressBarLayerProgress}) {
        if (progressBar) {
            progressBar->setMinimumHeight(progressHeight);
            progressBar->setMaximumHeight(progressHeight);
            progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    ui->labelLayerProgressValue->setMinimumWidth(
        qMax(96, ui->labelLayerProgressValue->fontMetrics().horizontalAdvance(QStringLiteral("100 / 100")) + 24));
    ui->labelLayerProgressValue->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->labelLayerProgressValue->setFixedHeight(progressHeight);
    ui->labelLayerProgressValue->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    ui->labelDoseDeliveryProgressValue->setMinimumWidth(
        qMax(132, ui->labelDoseDeliveryProgressValue->fontMetrics().horizontalAdvance(QStringLiteral("100.00 / 100.00 Gy")) + 24));
    ui->labelDoseDeliveryProgressValue->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->labelDoseDeliveryProgressValue->setFixedHeight(progressHeight);
    ui->labelDoseDeliveryProgressValue->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    for (QWidget *card : {ui->dosePrescriptionCard, ui->doseFractionCard,
                          ui->doseCurrentFractionCard, ui->doseCompletedCard,
                          ui->doseBeamMuCard, ui->doseLayerProgressCard,
                          ui->doseBeamInfoCard, ui->doseDeliveryProgressCard}) {
        if (card) {
            card->setMinimumWidth(0);
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }
    }

    for (QWidget *card : {ui->doseDeviationCard, ui->doseDeliveryProgressCard,
                          ui->doseBeamMuCard, ui->doseLayerProgressCard}) {
        if (card->parentWidget() && card->parentWidget()->objectName() == QStringLiteral("groupBoxLiveDose")) {
            QSizePolicy cardPolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            cardPolicy.setHeightForWidth(true);
            card->setSizePolicy(cardPolicy);
        }
    }

    m_doseInfoValueLabels = {
        ui->labelPrescriptionDose,
        ui->labelFractionDose,
        ui->labelCurrentFractionDose,
        ui->labelCompletedDose,
        ui->labelBeamMuDose,
        ui->labelDoseDeviation,
        ui->labelBeamDoseInfo
    };

    if (auto *liveLayout = findChild<QGridLayout *>(QStringLiteral("gridLayoutLiveDose"))) {
        auto *live = liveLayout->parentWidget();
        ui->labelDoseDeviationName->hide();
        ui->doseDeviationCard->setMinimumHeight(feedbackHeight);
        auto *state = new QLabel(live);
        state->setObjectName(QStringLiteral("labelBeamDeliveryState"));
        state->setWordWrap(true);
        state->setTextFormat(Qt::PlainText);
        state->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        liveLayout->addWidget(state, 0, 1);

        // Reuse the existing bound values; only their visual hierarchy changes.
        ui->horizontalLayoutDoseDeliveryProgress->removeWidget(ui->labelDoseDeliveryProgressName);
        ui->horizontalLayoutDoseDeliveryProgress->removeWidget(ui->labelDoseDeliveryProgressValue);
        ui->verticalLayoutDoseLayerProgress->removeWidget(ui->labelLayerProgressName);
        ui->horizontalLayoutLayerProgress->removeWidget(ui->labelLayerProgressValue);
        auto *metrics = new QWidget(live);
        metrics->setObjectName(QStringLiteral("widgetBeamCoreMetrics"));
        auto *metricsLayout = new QGridLayout(metrics);
        metricsLayout->setContentsMargins(0, 4, 0, 4);
        metricsLayout->setHorizontalSpacing(12);
        metricsLayout->setVerticalSpacing(4);
        metricsLayout->setSizeConstraint(QLayout::SetMinimumSize);
        metricsLayout->setColumnStretch(0, 2);
        metricsLayout->setColumnStretch(1, 1);
        metricsLayout->setColumnStretch(2, 1);
        ui->labelDoseDeliveryProgressName->setText(tr("累计计量 / 目标"));
        ui->labelLayerProgressName->setText(tr("当前能层 / 总层数"));
        auto *rateName = new QLabel(tr("剂量率"), metrics);
        rateName->setObjectName(QStringLiteral("clinicalRateCaption"));
        auto *rateValue = new QLabel(QStringLiteral("- Gy/s"), metrics);
        rateValue->setObjectName(QStringLiteral("labelBeamDoseRateValue"));
        const QVector<QLabel *> captions = {ui->labelDoseDeliveryProgressName, rateName,
                                            ui->labelLayerProgressName};
        const QVector<QLabel *> values = {ui->labelDoseDeliveryProgressValue, rateValue,
                                          ui->labelLayerProgressValue};
        for (int column = 0; column < values.size(); ++column) {
            for (QLabel *label : {captions.at(column), values.at(column)}) {
                label->setTextFormat(Qt::PlainText);
                label->setMinimumWidth(0);
                label->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
                label->setWordWrap(true);
                QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                policy.setHeightForWidth(true);
                label->setSizePolicy(policy);
                label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            }
            values.at(column)->setMinimumHeight(38);
            setTreatmentRole(captions.at(column), QStringLiteral("beamMetricCaption"));
            setTreatmentRole(values.at(column), QStringLiteral("beamMetricValue"));
            metricsLayout->addWidget(captions.at(column), 0, column);
            metricsLayout->addWidget(values.at(column), 1, column);
        }
        liveLayout->addWidget(metrics, 1, 0, 1, 2);
        auto *layerCaption = new QLabel(tr("能层进度"), ui->doseLayerProgressCard);
        setTreatmentRole(layerCaption, QStringLiteral("beamMetricCaption"));
        ui->verticalLayoutDoseLayerProgress->insertWidget(0, layerCaption);
        ui->labelDoseBeamMuName->setText(tr("当前层计量进度"));

        // The telemetry detail expander is intentionally removed from the UI.
        // Keep a hidden monitor object because the existing dose/telemetry update
        // path finds it by objectName. It must never become visible here.
        auto *monitor = new QLabel(live);
        monitor->setObjectName(QStringLiteral("labelDispatchMonitor"));
        monitor->setTextFormat(Qt::PlainText);
        monitor->setWordWrap(true);
        monitor->setProperty("statusTone", 0);
        setTreatmentRole(monitor, QStringLiteral("dispatchMonitor"));
        monitor->hide();
    }
    updateBeamMonitorSummary();
}

void TreatmentWindow::updateBeamMonitorSummary()
{
    if (auto *rate = findChild<QLabel *>(QStringLiteral("labelBeamDoseRateValue"))) {
        const QJsonValue value = m_latestTelemetryPayload.value(QStringLiteral("dose_rate"));
        const bool valid = !m_workflowSyncRequired && value.isDouble()
            && std::isfinite(value.toDouble()) && value.toDouble() >= 0.0;
        setLabelTextIfChanged(rate, valid ? tr("%1 Gy/s").arg(value.toDouble(), 0, 'g', 6)
                                         : QStringLiteral("- Gy/s"));
        rate->setToolTip(m_workflowSyncRequired ? tr("现场待同步，剂量率暂不作为实时回读显示")
            : m_latestTelemetryPayload.value(QStringLiteral("dose_rate_is_snapshot")).toBool()
            ? tr("现场快照读数，等待实时遥测更新") : tr("剂量率回读：dose_rate（Gy/s）"));
    }
    if (auto *state = findChild<QLabel *>(QStringLiteral("labelBeamDeliveryState"))) {
        QString text = m_workflowSyncRequired ? tr("供束：未知 | 现场待同步")
            : liveBeamDeliveryText(m_latestTelemetryPayload, m_logoutState,
                m_settings->value(QStringLiteral("Logout/BeamStatusMaxAgeMs"), 5000).toLongLong());
        if (m_clinicalBody) text.replace(QStringLiteral(" | "), QStringLiteral("\n"));
        setLabelTextIfChanged(state, text);
    }
    if (auto *monitor = findChild<QLabel *>(QStringLiteral("labelDispatchMonitor"))) {
        QString text = dispatchMonitorText(m_latestTelemetryPayload);
        const QJsonValue raw = m_latestTelemetryPayload.value(QStringLiteral("cumulative_dose"));
        if (raw.isDouble()) text += QLatin1Char('\n')
            + tr("快照累计剂量：%1 cGy（不作为MU）").arg(raw.toDouble(), 0, 'g', 8);
        setLabelTextIfChanged(monitor, text);
        monitor->setProperty("statusTone", 0);
        setTreatmentRole(monitor, QStringLiteral("dispatchMonitor"));
    }
}

void TreatmentWindow::setupBeamRoomOverview()
{
    if (m_beamRoomLabels.isEmpty()) {
        m_beamRoomLabels = {
            ui->beamRoomLabel1,
            ui->beamRoomLabel2,
            ui->beamRoomLabel9
        };

        const QVector<int> roomNumbers = {1, 2, 9};
        for (int index = 0; index < m_beamRoomLabels.size(); ++index) {
            QLabel *roomLabel = m_beamRoomLabels.at(index);
            if (!roomLabel) {
                continue;
            }
            roomLabel->setProperty("roomNumber", roomNumbers.at(index));
            roomLabel->setMinimumHeight(44);
            roomLabel->setAlignment(Qt::AlignCenter);
            roomLabel->setTextFormat(Qt::RichText);
            roomLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }

    if (!ui->verticalLayout_ops) {
        return;
    }

    if (ui->label_title_ops) {
        ui->label_title_ops->setText(tr("束流调度"));
        setTreatmentRole(ui->label_title_ops, QStringLiteral("treatmentStyle17"));
    }

    if (ui->label_10) {
        ui->label_10->setText(tr("束流分配"));
        setTreatmentRole(ui->label_10, QStringLiteral("treatmentStyle18"));
    }

    if (ui->label_11) {
        ui->label_11->setText(tr("<b>当前束流</b><br><span style='font-size:12px;'>未分配 · 等待治疗室申请</span>"));
        ui->label_11->setMinimumHeight(54);
        ui->label_11->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->label_11->setTextFormat(Qt::RichText);
        setTreatmentRole(ui->label_11, QStringLiteral("treatmentStyle19"));
    }
}

void TreatmentWindow::initTheme()
{
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TreatmentWindow::onThemeChanged, Qt::UniqueConnection);

    onThemeChanged(ThemeManager::instance().currentTheme());
}

void TreatmentWindow::onThemeChanged(Theme newTheme)
{
    Q_UNUSED(newTheme)
    setTreatmentRole(ui->label_title_status, QStringLiteral("statusTitle"));
    setTreatmentRole(ui->labelDeviceStatusCaption, QStringLiteral("statusTitle"));

    for (QWidget *widget : findChildren<QWidget*>()) {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }

    style()->unpolish(this);
    style()->polish(this);
    ui->spotMapWidget->update();
    // The custom-drawn MU gauge must repaint when switching light/dark mode.
    if (ui->progressBarDoseDelivery) ui->progressBarDoseDelivery->update();
    update();
}

void TreatmentWindow::initWebSocket()
{
    connectWebSocketSignals();
}

void TreatmentWindow::initBedView()
{
    auto *previewOverlayContainer = new QWidget(ui->groupBoxBedPreview);
    previewOverlayContainer->setObjectName(QStringLiteral("bedPreviewOverlayContainer"));
    previewOverlayContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *overlayLayout = new QGridLayout(previewOverlayContainer);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);

    m_bedView = new QQuickWidget(previewOverlayContainer);
    m_bedView->rootContext()->setContextProperty("bedController", m_bedController);
    m_bedView->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_bedView->setSource(QUrl("qrc:/models/Bed3DView.qml"));

    auto *positionPanel = new QWidget(previewOverlayContainer);
    positionPanel->setObjectName(QStringLiteral("bedPositionPanel"));
    positionPanel->setMinimumWidth(280);
    positionPanel->setMaximumWidth(320);
    positionPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    positionPanel->setAttribute(Qt::WA_StyledBackground, true);
    setTreatmentRole(positionPanel, QStringLiteral("treatmentStyle20"));

    auto *positionLayout = new QVBoxLayout(positionPanel);
    positionLayout->setContentsMargins(10, 10, 10, 10);
    positionLayout->setSpacing(6);

    auto *titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(6);
    titleLayout->addWidget(ui->label_icon_bed);
    titleLayout->addWidget(ui->label_title_bed);
    titleLayout->addStretch(1);
    positionLayout->addLayout(titleLayout);

    auto *positionGrid = new QGridLayout;
    positionGrid->setObjectName(QStringLiteral("clinicalPositionGrid"));
    positionGrid->setContentsMargins(0, 0, 0, 0);
    positionGrid->setHorizontalSpacing(8);
    positionGrid->setVerticalSpacing(7);
    positionGrid->addWidget(ui->label_38, 0, 1);
    positionGrid->addWidget(ui->label_39, 0, 2);

    QLabel *axisLabels[] = {
        ui->label_40, ui->label_41, ui->label_42,
        ui->label_43, ui->label_44, ui->label_45
    };
    QLabel *positionLabels[] = {
        ui->label_posa, ui->label_posb, ui->label_posc,
        ui->label_posx, ui->label_posy, ui->label_posz
    };
    QLineEdit *positionEdits[] = {
        ui->lineEdit_a, ui->lineEdit_b, ui->lineEdit_c,
        ui->lineEdit_x, ui->lineEdit_y, ui->lineEdit_z
    };

    for (int index = 0; index < 6; ++index) {
        const int row = index + 1;
        positionGrid->addWidget(axisLabels[index], row, 0);
        positionGrid->addWidget(positionLabels[index], row, 1);
        positionGrid->addWidget(positionEdits[index], row, 2);
    }
    positionGrid->setColumnStretch(0, 0);
    positionGrid->setColumnStretch(1, 1);
    positionGrid->setColumnStretch(2, 1);
    positionLayout->addLayout(positionGrid);

    overlayLayout->addWidget(m_bedView, 0, 0);
    overlayLayout->addWidget(positionPanel, 0, 0, Qt::AlignTop | Qt::AlignRight);
    positionPanel->raise();

    ui->horizontalLayout_bed->setSpacing(0);
    ui->horizontalLayout_bed->addWidget(previewOverlayContainer, 1);
}

void TreatmentWindow::initStepView()
{
    if (!ui->qmlStepContainer) return;

    if (!m_stepView) {
        m_stepView = new QQuickWidget(ui->qmlStepContainer);
        m_stepView->rootContext()->setContextProperty(QStringLiteral("themeManager"), &ThemeManager::instance());
        m_stepView->setResizeMode(QQuickWidget::SizeRootObjectToView);
        m_stepView->setAttribute(Qt::WA_AlwaysStackOnTop);
        m_stepView->setClearColor(Qt::transparent);
        m_stepView->setSource(QUrl(QStringLiteral("qrc:/models/StepProgressBar.qml")));
    }

    auto *box = qobject_cast<QBoxLayout *>(ui->qmlStepContainer->layout());
    if (!box) {
        box = new QVBoxLayout(ui->qmlStepContainer);
        ui->qmlStepContainer->setLayout(box);
    }
    box->setContentsMargins(0, 0, 0, 0);
    box->addWidget(m_stepView);

    ui->qmlStepContainer->setMinimumHeight(64);
    ui->qmlStepContainer->setMaximumHeight(82);
    updateStepBar();
}

void TreatmentWindow::initConnectionState()
{
    if (!m_webSocketClient) {
        qWarning() << "WebSocketClient 为空";
        return;
    }

    if (m_webSocketClient->isConnected()) {
        onWebSocketConnected();
        return;
    }

    const bool autoConnect =
        m_settings->value(QStringLiteral("Middleware/AutoConnect"), true).toBool();
    if (autoConnect) {
        connectToMiddlewareWebSocket();
    }
}

void TreatmentWindow::initClock()
{
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000); // 1秒刷新
    connect(m_clockTimer, &QTimer::timeout, this, &TreatmentWindow::updateTimeLabel);

    updateTimeLabel();   // 先立即显示一次
    m_clockTimer->start();
}

void TreatmentWindow::updateDeviceStatusIndicators(const QJsonObject &payload)
{
    QJsonObject status = firstObjectValue(
        payload,
        {QStringLiteral("status_update"),
         QStringLiteral("statusUpdate"),
         QStringLiteral("status"),
         QStringLiteral("device_status"),
         QStringLiteral("deviceStatus")});
    if (status.isEmpty()) {
        status = payload;
    }

    const QJsonObject couch = firstObjectValue(
        status, {QStringLiteral("couch"), QStringLiteral("couch_status")});
    const QJsonObject dr = RoomCapabilities::hasDrLift(m_roomId)
        ? firstObjectValue(status, {QStringLiteral("dr"), QStringLiteral("dr_state")})
        : QJsonObject();
    const QJsonObject igrt = firstObjectValue(
        status, {QStringLiteral("igrt"), QStringLiteral("igrt_status")});
    const QJsonObject filter = firstObjectValue(
        status,
        {QStringLiteral("ridge_filter_stand"),
         QStringLiteral("ridgeFilterStand"),
         QStringLiteral("filter_state")});
    const QJsonObject plc = firstObjectValue(
        status, {QStringLiteral("plc"), QStringLiteral("plc_state")});

    if (couch.isEmpty() && dr.isEmpty() && igrt.isEmpty()
        && filter.isEmpty() && plc.isEmpty()) {
        return;
    }

    bool anyFault = false;
    bool beamEstopUnknown = false;
    bool missingSafetyFeedback = false;
    bool anyDevice = false;
    bool allConnected = true;
    bool anyDisconnected = false;
    QStringList activeActions;
    QStringList issues;

    if (!couch.isEmpty()) {
        anyDevice = true;
        const QJsonValue connection = TelemetryValue::binaryFlag(couch.value("connected"));
        const bool connected = connection.toBool();
        const bool estop = firstBoolValue(couch, {QStringLiteral("estop")});
        const bool remoteMode = firstBoolValue(
            couch, {QStringLiteral("remote_mode"), QStringLiteral("remoteMode")});
        const bool ready = firstBoolValue(
            couch, {QStringLiteral("system_ready"), QStringLiteral("systemReady")});
        const bool moving = firstBoolValue(
            couch,
            {QStringLiteral("moving"),
             QStringLiteral("is_moving"),
             QStringLiteral("isMoving"),
             QStringLiteral("axis_moving"),
             QStringLiteral("axisMoving")});
        allConnected = allConnected && connected;
        anyDisconnected = anyDisconnected || (connection.isBool() && !connected);
        anyFault = anyFault || estop;
        if (!connected) issues.append(connection.isBool() ? tr("治疗床连接异常（动作未知）")
                                                        : tr("治疗床连接状态未知（动作未知）"));
        else {
            if (estop) issues.append(tr("治疗床急停"));
            if (moving) activeActions.append(tr("治疗床：正在运动"));
        }
        if (!connected) {
            setStatusPill(ui->labelCouchDeviceStatus,
                          tr("治疗床"), connection.isBool() ? tr("未连接") : tr("连接状态未知"),
                          connection.isBool() ? StatusTone::Error : StatusTone::Neutral);
        } else if (estop) {
            setStatusPill(ui->labelCouchDeviceStatus,
                          tr("治疗床"), tr("已连接 / 急停"), StatusTone::Error);
        } else if (!remoteMode) {
            setStatusPill(ui->labelCouchDeviceStatus,
                          tr("治疗床"), tr("已连接 / 本地模式"), StatusTone::Warning);
        } else if (moving) {
            setStatusPill(ui->labelCouchDeviceStatus,
                          tr("治疗床"), tr("已连接 / 运动中"), StatusTone::Information);
        } else if (ready) {
            setStatusPill(ui->labelCouchDeviceStatus,
                          tr("治疗床"), tr("已连接 / 就绪"), StatusTone::Success);
        } else {
            setStatusPill(ui->labelCouchDeviceStatus,
                          tr("治疗床"), tr("已连接 / 未就绪"), StatusTone::Warning);
        }
    }

    if (!dr.isEmpty()) {
        anyDevice = true;
        const QJsonValue connection = TelemetryValue::binaryFlag(dr.value("connected"));
        const bool connected = connection.toBool();
        const bool systemError = firstBoolValue(
            dr, {QStringLiteral("system_error"), QStringLiteral("systemError")});
        const bool axisError = firstBoolValue(
            dr, {QStringLiteral("axis_error"), QStringLiteral("axisError")});
        const bool moving = firstBoolValue(
            dr, {QStringLiteral("axis_moving"), QStringLiteral("axisMoving")});
        const bool atWorkPosition = firstBoolValue(
            dr,
            {QStringLiteral("axis_at_work_position"),
             QStringLiteral("axisAtWorkPosition")});
        const bool atZeroPosition = firstBoolValue(
            dr,
            {QStringLiteral("axis_at_zero_position"),
             QStringLiteral("axisAtZeroPosition")});

        allConnected = allConnected && connected;
        anyDisconnected = anyDisconnected || (connection.isBool() && !connected);
        anyFault = anyFault || systemError || axisError;
        if (!connected) issues.append(connection.isBool() ? tr("DR连接异常（动作未知）")
                                                        : tr("DR连接状态未知（动作未知）"));
        else {
            if (systemError) issues.append(tr("DR系统故障"));
            if (axisError) issues.append(tr("DR轴故障"));
            if (moving) activeActions.append(tr("DR：正在运动"));
        }
        if (!connected) {
            setStatusPill(ui->labelDrDeviceStatus,
                          tr("DR"), connection.isBool() ? tr("未连接") : tr("连接状态未知"),
                          connection.isBool() ? StatusTone::Error : StatusTone::Neutral);
        } else if (systemError || axisError) {
            setStatusPill(ui->labelDrDeviceStatus,
                          tr("DR"), tr("已连接 / 故障"), StatusTone::Error,
                          dr.value(QStringLiteral("system_error_message_zh")).toString());
        } else if (moving) {
            setStatusPill(ui->labelDrDeviceStatus,
                          tr("DR"), tr("已连接 / 运动中"), StatusTone::Information);
        } else if (atWorkPosition) {
            setStatusPill(ui->labelDrDeviceStatus,
                          tr("DR"), tr("已连接 / 工作位"), StatusTone::Success);
        } else if (atZeroPosition) {
            setStatusPill(ui->labelDrDeviceStatus,
                          tr("DR"), tr("已连接 / 零位"), StatusTone::Success);
        } else {
            setStatusPill(ui->labelDrDeviceStatus,
                          tr("DR"), tr("已连接 / 未到位"), StatusTone::Warning);
        }
    }

    if (!igrt.isEmpty()) {
        anyDevice = true;
        const QJsonValue connection = TelemetryValue::binaryFlag(igrt.value("connected"));
        const bool connected = connection.toBool();
        const QJsonObject errorStatus = firstObjectValue(
            igrt, {QStringLiteral("error_status"), QStringLiteral("errorStatus")});
        const int errorCode = errorStatus.value(QStringLiteral("error_code")).toInt(
            errorStatus.value(QStringLiteral("errorCode")).toInt());
        const QString errorMessage =
            errorStatus.value(QStringLiteral("error_message")).toString(
                errorStatus.value(QStringLiteral("errorMessage")).toString());
        const bool hasError = !errorStatus.isEmpty()
            && (errorCode != 0 || !errorMessage.isEmpty());

        allConnected = allConnected && connected;
        anyDisconnected = anyDisconnected || (connection.isBool() && !connected);
        anyFault = anyFault || hasError;
        if (!connected) issues.append(connection.isBool() ? tr("IGRT连接异常（动作未知）")
                                                        : tr("IGRT连接状态未知（动作未知）"));
        else if (hasError) issues.append(tr("IGRT异常"));
        if (!connected) {
            setStatusPill(ui->labelIgrtDeviceStatus,
                          tr("IGRT"), connection.isBool() ? tr("未连接") : tr("连接状态未知"),
                          connection.isBool() ? StatusTone::Error : StatusTone::Neutral);
        } else if (hasError) {
            setStatusPill(ui->labelIgrtDeviceStatus,
                          tr("IGRT"), tr("已连接 / 异常"), StatusTone::Error,
                          errorMessage);
        } else {
            setStatusPill(ui->labelIgrtDeviceStatus,
                          tr("IGRT"), tr("已连接 / 正常"), StatusTone::Success);
        }
    }

    if (!filter.isEmpty()) {
        anyDevice = true;
        const QJsonValue connection = TelemetryValue::binaryFlag(filter.value("connected"));
        const bool connected = connection.toBool();
        const int motorFaultWord =
            filter.value(QStringLiteral("motor_fault_word_40134")).toInt(
                filter.value(QStringLiteral("motorFaultWord40134")).toInt());
        const bool motorFault = motorFaultWord != 0;
        const QStringList inPositionKeys = {
            QStringLiteral("cabinet1_axis1_in_position"),
            QStringLiteral("cabinet1_axis2_in_position"),
            QStringLiteral("cabinet1_axis3_in_position"),
            QStringLiteral("cabinet2_axis1_in_position"),
            QStringLiteral("cabinet2_axis2_in_position"),
            QStringLiteral("cabinet2_axis3_in_position"),
            QStringLiteral("cabinet3_axis1_in_position"),
            QStringLiteral("cabinet3_axis2_in_position"),
            QStringLiteral("cabinet3_axis3_in_position")
        };
        bool hasPositionState = false;
        bool allInPosition = true;
        for (const QString &key : inPositionKeys) {
            bool inPosition = false;
            if (tryBoolValue(filter,
                             {key, lowerCamelFromSnake(key)},
                             &inPosition)) {
                hasPositionState = true;
                allInPosition = allInPosition && inPosition;
            }
        }
        const int insertCommand =
            filter.value(QStringLiteral("control_word_40002")).toInt(
                filter.value(QStringLiteral("controlWord40002")).toInt());
        const int exitCommand =
            filter.value(QStringLiteral("control_word_40003")).toInt(
                filter.value(QStringLiteral("controlWord40003")).toInt());
        const int jog1Command =
            filter.value(QStringLiteral("control_word_40004")).toInt(
                filter.value(QStringLiteral("controlWord40004")).toInt());
        const int jog2Command =
            filter.value(QStringLiteral("control_word_40005")).toInt(
                filter.value(QStringLiteral("controlWord40005")).toInt());

        allConnected = allConnected && connected;
        anyDisconnected = anyDisconnected || (connection.isBool() && !connected);
        anyFault = anyFault || motorFault;
        if (!connected) issues.append(connection.isBool() ? tr("脊形过滤器连接异常（动作未知）")
                                                        : tr("脊形过滤器连接状态未知（动作未知）"));
        else if (motorFault) issues.append(tr("脊形过滤器电机故障"));
        if (!connected) {
            setStatusPill(ui->labelFilterDeviceStatus,
                          tr("脊形过滤器"), connection.isBool() ? tr("未连接") : tr("连接状态未知"),
                          connection.isBool() ? StatusTone::Error : StatusTone::Neutral);
        } else if (motorFault) {
            setStatusPill(ui->labelFilterDeviceStatus,
                          tr("脊形过滤器"), tr("已连接 / 故障"), StatusTone::Error);
        } else if (hasPositionState && allInPosition) {
            setStatusPill(ui->labelFilterDeviceStatus,
                          tr("脊形过滤器"), tr("已连接 / 到位"), StatusTone::Success);
        } else {
            setStatusPill(ui->labelFilterDeviceStatus,
                          tr("脊形过滤器"),
                          hasPositionState ? tr("已连接 / 未到位")
                                           : tr("已连接 / 状态未知"),
                          hasPositionState ? StatusTone::Warning : StatusTone::Neutral);
        }
        if (connected) {
            // Control words report requests, not confirmation of physical motion.
            if (insertCommand != 0) activeActions.append(tr("脊形过滤器：插入指令有效"));
            if (exitCommand != 0) activeActions.append(tr("脊形过滤器：退出指令有效"));
            if (jog1Command != 0 || jog2Command != 0)
                activeActions.append(tr("脊形过滤器：点动指令有效"));
        }
    }

    if (!plc.isEmpty()) {
        anyDevice = true;
        const QJsonValue connection = TelemetryValue::binaryFlag(plc.value("connected"));
        const bool connected = connection.toBool();
        const bool beamReady = firstBoolValue(
            plc, {QStringLiteral("beam_ready"), QStringLiteral("beamReady")});
        const bool beamOn = firstBoolValue(
            plc, {QStringLiteral("beam_on"), QStringLiteral("beamOn")});
        const bool layerChange = firstBoolValue(
            plc, {QStringLiteral("layer_change"), QStringLiteral("layerChange")});
        const bool paused = firstBoolValue(
            plc, {QStringLiteral("pause_active"), QStringLiteral("pauseActive")});
        const bool resumed = firstBoolValue(
            plc, {QStringLiteral("resume_active"), QStringLiteral("resumeActive")});
        const bool stopped = firstBoolValue(
            plc, {QStringLiteral("stop_active"), QStringLiteral("stopActive")});
        const QJsonValue beamEstopReleased = TelemetryValue::binaryFlag(
            plc, {QStringLiteral("tcp_beam_estop"), QStringLiteral("tcpBeamEstop")});
        const bool hasBeamEstop = beamEstopReleased.isBool();
        // The PLC emergency-stop feedback is active-low; missing data is unknown.
        const bool beamEstop = hasBeamEstop && !beamEstopReleased.toBool();
        beamEstopUnknown = connected && !hasBeamEstop;
        const QJsonValue ddsInterlockValue = TelemetryValue::binaryFlag(
            plc, {QStringLiteral("dds_interlock"), QStringLiteral("ddsInterlock")});
        const QJsonValue bedInterlockValue = TelemetryValue::binaryFlag(
            plc, {QStringLiteral("medicalbed_interlock"),
                  QStringLiteral("medicalbedInterlock")});
        const bool hasDdsInterlock = ddsInterlockValue.isBool();
        const bool hasBedInterlock = bedInterlockValue.isBool();
        const bool ddsInterlock = hasDdsInterlock && ddsInterlockValue.toBool();
        const bool bedInterlock = hasBedInterlock && bedInterlockValue.toBool();
        missingSafetyFeedback = connected
            && (!hasBeamEstop || !hasDdsInterlock || !hasBedInterlock);
        const bool plcFault = beamEstop || ddsInterlock || bedInterlock;

        allConnected = allConnected && connected;
        anyDisconnected = anyDisconnected || (connection.isBool() && !connected);
        anyFault = anyFault || plcFault;
        if (!connected) issues.append(connection.isBool() ? tr("PLC连接异常（动作未知）")
                                                        : tr("PLC连接状态未知（动作未知）"));
        else {
            if (beamEstop) issues.append(tr("PLC束流急停"));
            if (beamEstopUnknown) issues.append(tr("PLC束流急停状态未知"));
            if (!hasDdsInterlock) issues.append(tr("PLC DDS联锁状态未知"));
            if (!hasBedInterlock) issues.append(tr("PLC治疗床联锁状态未知"));
            if (ddsInterlock) issues.append(tr("PLC反馈DDS联锁触发"));
            if (bedInterlock) issues.append(tr("治疗床联锁触发"));
            if (stopped) activeActions.append(tr("PLC：停止状态有效"));
            if (paused) activeActions.append(tr("PLC：暂停状态有效"));
            if (beamOn) activeActions.append(tr("PLC：出束反馈有效"));
            if (layerChange) activeActions.append(tr("PLC：换层反馈有效"));
            if (resumed) activeActions.append(tr("PLC：恢复反馈有效"));
        }

        if (!connected) {
            setStatusDot(ui->label_46, StatusTone::Error, tr("PLC 连接断开，束流联锁状态不可用"));
            setStatusDot(ui->label_47, StatusTone::Error, tr("PLC 连接断开，治疗床联锁状态不可用"));
        } else {
            setStatusDot(ui->label_46,
                         (beamEstop || ddsInterlock) ? StatusTone::Error
                             : (beamEstopUnknown || !hasDdsInterlock) ? StatusTone::Neutral
                                                                       : StatusTone::Success,
                         beamEstop ? tr("束流急停有效")
                                   : ddsInterlock ? tr("DDS 联锁有效")
                                     : beamEstopUnknown ? tr("等待束流急停反馈（tcp_beam_estop）")
                                     : !hasDdsInterlock ? tr("等待DDS联锁反馈")
                                                        : tr("束流联锁正常"));
            setStatusDot(ui->label_47,
                         bedInterlock ? StatusTone::Error
                             : hasBedInterlock ? StatusTone::Success : StatusTone::Neutral,
                         bedInterlock ? tr("治疗床联锁有效")
                             : hasBedInterlock ? tr("治疗床联锁正常")
                                               : tr("等待治疗床联锁反馈"));
        }

        if (!connected) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), connection.isBool() ? tr("未连接") : tr("连接状态未知"),
                          connection.isBool() ? StatusTone::Error : StatusTone::Neutral);
        } else if (plcFault) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"),
                          beamEstop ? tr("已连接 / 急停")
                                    : tr("已连接 / 联锁"),
                          StatusTone::Error);
        } else if (missingSafetyFeedback) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 安全状态未知"), StatusTone::Neutral,
                          tr("等待急停/DDS联锁/治疗床联锁的完整反馈"));
        } else if (stopped) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 已停止"), StatusTone::Warning);
        } else if (paused) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 已暂停"), StatusTone::Warning);
        } else if (beamOn) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 出束中"), StatusTone::Information);
        } else if (layerChange) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 换层中"), StatusTone::Information);
        } else if (resumed) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 恢复中"), StatusTone::Information);
        } else if (beamReady) {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 束流就绪"), StatusTone::Success);
        } else {
            setStatusPill(ui->labelPlcDeviceStatus,
                          tr("PLC"), tr("已连接 / 待命"), StatusTone::Neutral);
        }
    }

    if (anyDevice) {
        setStatusDot(ui->label_35,
                     anyDisconnected ? StatusTone::Error
                         : allConnected ? StatusTone::Success : StatusTone::Neutral,
                     anyDisconnected ? tr("存在设备连接断开")
                         : allConnected ? tr("设备连接正常") : tr("存在设备连接状态未知"));
        setStatusDot(ui->label_9,
                     anyFault ? StatusTone::Error
                         : (missingSafetyFeedback || !allConnected) ? StatusTone::Neutral : StatusTone::Success,
                     anyFault ? tr("存在设备故障、急停或联锁")
                         : missingSafetyFeedback ? tr("PLC安全回读不完整，不能确认正常")
                             : !allConnected ? tr("存在设备状态不可用")
                                            : tr("未发现设备报警"));

    }

    m_currentDeviceActionText = activeActions.isEmpty()
        ? tr("暂无有效动作反馈") : activeActions.join(QLatin1Char('\n'));
    m_currentDeviceIssues = issues;
    updateTreatmentActionDisplay();
}

void TreatmentWindow::updateDoseInfoPanel(const QJsonObject &payload)
{
    updateBeamMonitorSummary();
    if (m_latestTelemetryPayload.isEmpty()) {
        updateGantryIndicators(this, QJsonObject());
        updateDdsInterlock(ui->groupBoxSystemStatus, QJsonObject());
        for (const QString &name : {QStringLiteral("labelDispatchMonitor"),
                                    QStringLiteral("labelDispatchInterlocks"),
                                    QStringLiteral("labelDispatchDevices")}) {
          if (auto *monitor = findChild<QLabel *>(name)) {
            const int section = name == QStringLiteral("labelDispatchInterlocks") ? 1
                : name == QStringLiteral("labelDispatchDevices") ? 2 : 0;
            setLabelTextIfChanged(monitor, dispatchMonitorText(QJsonObject(), section));
            monitor->setProperty("statusTone", 0);
            setTreatmentRole(monitor, QStringLiteral("dispatchMonitor"));
          }
        }
    }
    if (m_doseInfoValueLabels.size() < 7) {
        return;
    }

    if (payload.isEmpty()) {
        m_lastDoseTelemetrySnapshot.clear();
        for (QLabel *label : m_doseInfoValueLabels) {
            setLabelTextIfChanged(label, QStringLiteral("-"));
            label->setToolTip(QString());
        }
        ui->progressBarDoseDelivery->setToolTip(QString());
        ui->progressBarDoseDelivery->setProperty("clinicalHasMuProgress", false);
        ui->progressBarDoseDelivery->update();
        setProgressBarIfChanged(ui->progressBarLayerProgress, 0, 100, 0, QStringLiteral("-"));
        setLabelTextIfChanged(ui->labelLayerProgressValue, QStringLiteral("- / -"));
        setProgressBarIfChanged(ui->progressBarBeamMeterset, 0, 1000, 0, tr("暂无计划束流计量"));
        ui->labelBeamMuDose->hide();
        ui->labelDoseBeamMuName->setText(tr("当前层计量进度"));
        setProgressBarIfChanged(ui->progressBarDoseDelivery, 0, 1000, 0, tr("暂无计划剂量"));
        setLabelTextIfChanged(ui->labelDoseDeliveryProgressValue, QStringLiteral("- / - MU"));
        if (ui->labelTopPrescriptionDose) {
            setLabelTextIfChanged(ui->labelTopPrescriptionDose, QStringLiteral("-"));
        }
        if (ui->labelTopPrescriptionDetail) {
            setLabelTextIfChanged(ui->labelTopPrescriptionDetail, tr("等待计划"));
        }
        if (m_clinicalBody) updateClinicalWorkspace();
        return;
    }

    const PlanDisplayInfo plan = parsePlanDisplayInfo(payload);
    const QJsonObject summary = plan.summary;
    const QJsonArray beams = plan.beams;
    const QString prescriptionText = plan.prescriptionText;
    const double fractionDose = plan.fractionDose;

    int totalLayers = 0;
    int totalSpots = 0;
    for (const QJsonValue &beamValue : beams) {
        const QJsonObject beam = beamValue.toObject();
        totalLayers += readIntValue(beam, {"energy_layers"}, 0);
        totalSpots += readIntValue(beam, {"total_scan_spots", "spot_count"}, 0);
    }

    const QJsonObject firstBeam = beams.isEmpty() ? QJsonObject() : beams.first().toObject();
    const QString machineText = summary.value(QStringLiteral("machine")).toString(
        firstBeam.value(QStringLiteral("machine")).toString(QStringLiteral("-")));
    const QString scanModeText = summary.value(QStringLiteral("scan_mode")).toString(
        firstBeam.value(QStringLiteral("scan_mode")).toString(QStringLiteral("-")));
    const QString radiationTypeText = firstBeam.value(QStringLiteral("radiation_type")).toString(QStringLiteral("-"));
    const int beamCount = summary.value(QStringLiteral("beam_count")).toInt(beams.size());

    const QString layerSpotText = (totalLayers > 0 || totalSpots > 0)
        ? tr("能层：%1，Spot：%2").arg(totalLayers).arg(totalSpots)
        : QStringLiteral("-");
    int currentLayer = readIntValue(m_latestTelemetryPayload, {"current_layer"});
    if (currentLayer < 0) {
        currentLayer = readIntValue(payload, {"current_layer"});
    }

    int currentBeamNumber = readIntValue(
        m_latestTelemetryPayload, {"current_beam", "current_beam_number", "beam_number"});
    if (currentBeamNumber < 0) {
        currentBeamNumber = readIntValue(
            payload, {"current_beam", "current_beam_number", "beam_number"});
    }

    QJsonObject currentBeam = firstBeam;
    if (currentBeamNumber >= 0) {
        for (const QJsonValue &beamValue : beams) {
            const QJsonObject beam = beamValue.toObject();
            if (readIntValue(beam, {"beam_number"}) == currentBeamNumber) {
                currentBeam = beam;
                break;
            }
        }
    }

    double currentLayerPlannedMeterset = -1.0;
    const QJsonArray currentBeamLayers = currentBeam.value(QStringLiteral("layers")).toArray();
    if (currentLayer >= 0 && !currentBeamLayers.isEmpty()) {
        // WebSocket current_layer is a 0-based index, including layer zero.
        qsizetype currentLayerPosition = -1;
        for (qsizetype layerPosition = 0;
             layerPosition < currentBeamLayers.size();
             ++layerPosition) {
            const QJsonValue layerValue = currentBeamLayers.at(layerPosition);
            const QJsonObject layer = layerValue.toObject();
            if (readIntValue(layer, {"layer_index"}) == currentLayer) {
                currentLayerPosition = layerPosition;
                break;
            }
        }
        if (currentLayerPosition < 0 && currentLayer < currentBeamLayers.size()) {
            currentLayerPosition = currentLayer;
        }
        currentLayerPlannedMeterset =
            plannedMetersetForLayer(currentBeamLayers, currentLayerPosition);
    }

    // layer_sum_dose is absent from the supplied interface specification.
    // Do not infer it is an MU value or use it as beam-on evidence.
    const double cumulativeMeterset = -1.0;

    const QJsonValue beamOnFlag = TelemetryValue::binaryFlag(
        m_latestTelemetryPayload.value(QStringLiteral("beam_on")));
    const QJsonValue deliveringFlag = TelemetryValue::binaryFlag(
        m_latestTelemetryPayload.value(QStringLiteral("is_delivering")));
    const bool isDelivering = beamOnFlag.isBool() ? beamOnFlag.toBool()
        : (deliveringFlag.isBool() && deliveringFlag.toBool());
    const bool treatmentHasStarted = isDelivering
        || m_treatmentState == TreatmentState::Treating
        || m_treatmentState == TreatmentState::BeamFinished
        || m_treatmentState == TreatmentState::ReportConfirm
        || m_treatmentState == TreatmentState::Completed
        || cumulativeMeterset > 0.0;
    // Label layer number in human 1-based format, but retain the underlying
    // 0-based index from the authoritative message.
    int displayLayer = currentLayer >= 0 ? currentLayer + 1 : -1;
    if (totalLayers > 0 && !treatmentHasStarted
        && !payload.contains(QStringLiteral("state"))) displayLayer = 0;
    const bool hasLayerProgress = displayLayer >= 0 && totalLayers > 0;

    // These legacy per-layer MU fields are not documented in the supplied spec.
    // Keep the meter readout unknown rather than borrow whole-treatment sum_dose.
    const double deliveredMeterset = -1.0;
    const QString beamMetersetText = currentLayer >= 0
        ? tr("当前层 %1：实际 %2 / 计划 %3")
              .arg(currentLayer + 1)
              .arg(deliveredMeterset >= 0.0
                       ? QString::number(deliveredMeterset, 'f', 1)
                       : QStringLiteral("-"))
              .arg(currentLayerPlannedMeterset >= 0.0
                       ? QString::number(currentLayerPlannedMeterset, 'f', 1)
                       : QStringLiteral("-"))
        : tr("等待当前能层");
    int beamMetersetProgressValue = 0;
    QString beamMetersetProgressFormat;
    if (currentLayerPlannedMeterset > 0.0 && deliveredMeterset >= 0.0) {
        const double beamMetersetPercent = qBound(
            0.0, (deliveredMeterset / currentLayerPlannedMeterset) * 100.0, 100.0);
        beamMetersetProgressValue = qRound(beamMetersetPercent * 10.0);
        beamMetersetProgressFormat = tr("%1%").arg(beamMetersetPercent, 0, 'f', 1);
    } else if (currentLayerPlannedMeterset > 0.0) {
        beamMetersetProgressFormat = tr("等待当前层计量回读");
    } else if (currentLayer >= 0) {
        beamMetersetProgressFormat = tr("当前层未提供计划计量");
    } else {
        beamMetersetProgressFormat = tr("等待当前能层");
    }

    double doseDeviation = readDoubleValue(m_latestTelemetryPayload, {"dose_deviation", "dose_deviation_percent"});
    if (doseDeviation < 0.0) {
        doseDeviation = readDoubleValue(payload, {"dose_deviation", "dose_deviation_percent"});
    }
    const QString compensationStatus = payload.value("interruption_compensation_status").toString(
        payload.value("compensation_status").toString(
            TelemetryValue::binaryFlag(m_latestTelemetryPayload.value("interruption_status")).toBool(false)
                ? tr("中断")
                : tr("待治疗反馈")));

    QStringList deviationLines;
    if (compensationStatus != tr("待治疗反馈"))
        deviationLines << tr("状态：%1").arg(compensationStatus);
    const double cGy = readDoubleValue(m_latestTelemetryPayload, {"cumulative_dose"});
    deviationLines << (cGy >= 0.0
        ? tr("%1累计剂量：%2 cGy")
            .arg(m_latestTelemetryPayload.value(QStringLiteral("cumulative_dose_is_snapshot")).toBool()
                ? tr("快照") : tr("回读"))
            .arg(cGy, 0, 'f', 2)
        : tr("累积剂量：暂无 cGy 回读"));
    if (doseDeviation >= 0.0) {
        deviationLines << tr("剂量偏差：%1%").arg(doseDeviation, 0, 'f', 1);
    }
    const QString deviationText = deviationLines.join(QLatin1Char('\n'));

    QStringList beamDoseLines;
    QJsonArray referencedBeams;
    const QJsonArray fractionGroups = summary.value("fraction_groups").toArray();
    if (!fractionGroups.isEmpty()) {
        referencedBeams = fractionGroups.first().toObject().value("referenced_beams").toArray();
    }
    for (const QJsonValue &beamValue : referencedBeams) {
        const QJsonObject beam = beamValue.toObject();
        const int beamNumber = readIntValue(beam, {"beam_number"}, beamDoseLines.size() + 1);
        const double beamDose = readDoubleValue(beam, {"beam_dose_gy"});
        const double meterset = readDoubleValue(beam, {"beam_meterset"});
        if (beamDose < 0.0 && meterset < 0.0) {
            continue;
        }
        QString line = tr("Beam %1：").arg(beamNumber);
        if (beamDose >= 0.0) {
            line += gyText(beamDose, 2);
        }
        if (meterset >= 0.0) {
            line += tr("，meterset %1").arg(meterset, 0, 'f', 1);
        }
        beamDoseLines << line;
    }
    if (beamDoseLines.isEmpty()) {
        for (const QJsonValue &beamValue : beams) {
            const QJsonObject beam = beamValue.toObject();
            const int beamNumber = readIntValue(beam, {"beam_number"}, beamDoseLines.size() + 1);
            const double meterset = readDoubleValue(beam, {"beam_meterset", "meterset"});
            const int layers = readIntValue(beam, {"energy_layers"});
            const int spots = readIntValue(beam, {"total_scan_spots"});
            QStringList values;
            if (meterset >= 0.0) {
                values << tr("meterset %1").arg(meterset, 0, 'f', 1);
            }
            if (layers >= 0) {
                values << tr("能层 %1").arg(layers);
            }
            if (spots >= 0) {
                values << tr("Spot %1").arg(spots);
            }
            if (!values.isEmpty()) {
                beamDoseLines << tr("Beam %1：%2").arg(beamNumber).arg(values.join(QStringLiteral("，")));
            }
        }
    }
    if (beamDoseLines.size() > 3) {
        beamDoseLines = beamDoseLines.mid(0, 3);
        beamDoseLines << tr("...");
    }

    if (ui->labelTopPrescriptionDose) {
        setLabelTextIfChanged(ui->labelTopPrescriptionDose, prescriptionText);
    }
    if (ui->labelTopPrescriptionDetail) {
        setLabelTextIfChanged(ui->labelTopPrescriptionDetail,
                              fractionDose >= 0.0
                                  ? tr("本次：%1 Gy / fraction").arg(fractionDose, 0, 'f', 2)
                                  : tr("本次分次剂量：-"));
    }

    setLabelTextIfChanged(m_doseInfoValueLabels.at(0), machineText);
    setLabelTextIfChanged(m_doseInfoValueLabels.at(1), tr("%1 / %2").arg(scanModeText, radiationTypeText));
    setLabelTextIfChanged(m_doseInfoValueLabels.at(2), beamCount > 0 ? tr("%1 个").arg(beamCount) : QStringLiteral("-"));
    setLabelTextIfChanged(m_doseInfoValueLabels.at(3), layerSpotText);
    setLabelTextIfChanged(m_doseInfoValueLabels.at(4), beamMetersetText);
    ui->labelBeamMuDose->hide();
    ui->labelDoseBeamMuName->setText(currentLayer >= 0
        ? tr("当前层计量：%1 / %2")
            .arg(deliveredMeterset >= 0.0 ? QString::number(deliveredMeterset, 'f', 1) : QStringLiteral("-"),
                 currentLayerPlannedMeterset >= 0.0 ? QString::number(currentLayerPlannedMeterset, 'f', 1) : QStringLiteral("-"))
        : tr("当前层计量进度"));
    ui->progressBarBeamMeterset->setToolTip(beamMetersetText);
    setLabelTextIfChanged(m_doseInfoValueLabels.at(5), deviationText);
    setLabelTextIfChanged(m_doseInfoValueLabels.at(6),
                          beamDoseLines.isEmpty()
                              ? QStringLiteral("-")
                              : beamDoseLines.join(QLatin1Char('\n')));
    // Keep recovery values in the existing Beam detail area, explicitly as a snapshot.
    if (payload.contains(QStringLiteral("state"))
        && (payload.contains(QStringLiteral("current_delivered_mu"))
            || payload.contains(QStringLiteral("target_mu")))) {
        const auto snapshotNumber = [&payload](const char *key) {
            const QJsonValue v = payload.value(QLatin1String(key));
            return v.isDouble() ? QString::number(v.toDouble(), 'g', 8) : QStringLiteral("-");
        };
        QString detail = m_doseInfoValueLabels.at(6)->text();
        if (detail == QStringLiteral("-")) detail.clear();
        if (!detail.isEmpty()) detail += QLatin1Char('\n');
        detail += tr("恢复快照：总 MU %1 / %2；能量 %3")
            .arg(snapshotNumber("current_delivered_mu"), snapshotNumber("target_mu"), snapshotNumber("energy"));
        setLabelTextIfChanged(m_doseInfoValueLabels.at(6), detail);
        m_doseInfoValueLabels.at(6)->setToolTip(tr("治疗ID：%1\n快照累计剂量：%2 cGy（不能当作MU）")
            .arg(payload.value("treatment_id").toString(QStringLiteral("-")), snapshotNumber("cumulative_dose")));
    } else {
        m_doseInfoValueLabels.at(6)->setToolTip(QString());
    }
    setProgressBarIfChanged(ui->progressBarBeamMeterset,
                            0,
                            1000,
                            beamMetersetProgressValue,
                            beamMetersetProgressFormat);

    const int progressMaximum = totalLayers > 0 ? totalLayers : 100;
    const int progressValue = hasLayerProgress ? qBound(0, displayLayer, totalLayers) : 0;
    const QString progressFormat = hasLayerProgress
        ? tr("%1%").arg(qRound((100.0 * displayLayer) / totalLayers))
        : QStringLiteral("-");
    setProgressBarIfChanged(ui->progressBarLayerProgress, 0, progressMaximum, progressValue, progressFormat);

    const QString layerProgressText = totalLayers > 0
        ? (hasLayerProgress ? tr("%1 / %2").arg(displayLayer).arg(totalLayers)
                            : tr("- / %1").arg(totalLayers))
        : (displayLayer >= 0 ? tr("%1 / -").arg(displayLayer) : QStringLiteral("- / -"));
    setLabelTextIfChanged(ui->labelLayerProgressValue, layerProgressText);

    int doseProgressValue = 0;
    QString doseProgressFormat = tr("暂无累计剂量回读");
    QString doseProgressText = plan.plannedMu > 0.0
        ? tr("- / %1 MU").arg(plan.plannedMu, 0, 'f', 1)
        : QStringLiteral("- / - MU");
    const QJsonValue currentMu = m_latestTelemetryPayload.value(QStringLiteral("current_delivered_mu"));
    // Target MU may arrive only in TASK_PLAN_CONFIRM, not in a telemetry frame.
    const QJsonValue targetMu = m_latestTelemetryPayload.contains(QStringLiteral("target_mu"))
        ? m_latestTelemetryPayload.value(QStringLiteral("target_mu"))
        : payload.value(QStringLiteral("target_mu"));
    if (currentMu.isDouble() || targetMu.isDouble()) {
        const bool hasProgress = currentMu.isDouble() && targetMu.isDouble()
            && currentMu.toDouble() >= 0 && targetMu.toDouble() > 0;
        const bool fromSnapshot = m_latestTelemetryPayload.value(QStringLiteral("workflow_mu_is_snapshot")).toBool();
        const QString source = fromSnapshot ? tr("恢复快照") : tr("遥测");
        doseProgressText = tr("%1 / %2 MU")
            .arg(currentMu.isDouble() ? QString::number(currentMu.toDouble(), 'g', 8) : QStringLiteral("-"),
                 targetMu.isDouble() ? QString::number(targetMu.toDouble(), 'g', 8) : QStringLiteral("-"));
        if (hasProgress) {
            const double percent = qBound(0.0, currentMu.toDouble() / targetMu.toDouble(), 1.0) * 100.0;
            doseProgressValue = qRound(percent * 10.0);
            doseProgressFormat = tr("%1：%2%（MU）").arg(source).arg(percent, 0, 'f', 1);
        } else {
            doseProgressFormat = tr("%1：MU进度待确认").arg(source);
        }
        ui->progressBarDoseDelivery->setToolTip(tr("按已交付MU/目标MU显示，不是Gy剂量换算。"));
    } else {
        ui->progressBarDoseDelivery->setToolTip(QString());
    }
    const bool hasConfirmedMuRatio = currentMu.isDouble() && targetMu.isDouble()
        && std::isfinite(currentMu.toDouble()) && std::isfinite(targetMu.toDouble())
        && currentMu.toDouble() >= 0.0 && targetMu.toDouble() > 0.0;
    ui->progressBarDoseDelivery->setProperty("clinicalHasMuProgress", hasConfirmedMuRatio);
    setProgressBarIfChanged(ui->progressBarDoseDelivery,
                            0,
                            1000,
                            doseProgressValue,
                            doseProgressFormat);
    ui->progressBarDoseDelivery->update();
    setLabelTextIfChanged(ui->labelDoseDeliveryProgressValue, doseProgressText);
    if (m_clinicalBody) updateClinicalWorkspace();
}

void TreatmentWindow::updateBeamRoomOverview(const QString &activeRoomId)
{
    if (m_beamRoomLabels.isEmpty()) {
        return;
    }

    if (!activeRoomId.trimmed().isEmpty()) {
        m_activeBeamRoomId = activeRoomId.trimmed();
    }

    int activeRoomNumber = -1;
    const QRegularExpression numberPattern(QStringLiteral("(\\d+)"));
    const QRegularExpressionMatch match = numberPattern.match(m_activeBeamRoomId);
    if (match.hasMatch()) {
        activeRoomNumber = match.captured(1).toInt();
    }




    for (QLabel *roomLabel : std::as_const(m_beamRoomLabels)) {
        if (!roomLabel) {
            continue;
        }

        const int roomNumber = roomLabel->property("roomNumber").toInt();
        const bool isActive = roomNumber == activeRoomNumber;
        setTreatmentRole(roomLabel, isActive ? QStringLiteral("activeStyle") : QStringLiteral("idleStyle"));
        roomLabel->setTextFormat(Qt::PlainText);
        roomLabel->setText(tr("治疗室%1\n%2").arg(roomNumber)
            .arg(isActive ? tr("束流已分配") : tr("未报告占用状态")));
    }

    if (ui->label_11) {
        if (activeRoomNumber == 1 || activeRoomNumber == 2 || activeRoomNumber == 9) {
            ui->label_11->setText(tr("当前分配：治疗室%1\n您的排位：未接入").arg(activeRoomNumber));
        } else {
            ui->label_11->setText(tr("当前分配：未收到反馈\n您的排位：未接入"));
        }
        ui->label_11->setTextFormat(Qt::PlainText);
        ui->label_11->setWordWrap(true);
    }
}

void TreatmentWindow::updateUserInfoUi()
{
    const bool hasDrLift = RoomCapabilities::hasDrLift(m_roomId);
    int deviceColumn = 0;
    for (QLabel *label : {ui->labelCouchDeviceStatus, ui->labelDrDeviceStatus,
                          ui->labelIgrtDeviceStatus, ui->labelFilterDeviceStatus,
                          ui->labelPlcDeviceStatus}) {
        ui->gridLayout_deviceStatus->removeWidget(label);
        const bool available = label != ui->labelDrDeviceStatus || hasDrLift;
        label->setVisible(available);
        if (available) ui->gridLayout_deviceStatus->addWidget(label, 1, deviceColumn++);
    }
    for (int column = 0; column < 5; ++column)
        ui->gridLayout_deviceStatus->setColumnStretch(column, column < deviceColumn ? 1 : 0);
    ui->gridLayout_deviceStatus->removeWidget(ui->labelDeviceStatusCaption);
    ui->gridLayout_deviceStatus->addWidget(ui->labelDeviceStatusCaption, 0, 0, 1, deviceColumn);
    if (QLayoutItem *item = ui->gridLayout_deviceStatus->itemAtPosition(2, 0)) {
        if (QLayout *gantryRow = item->layout()) {
            ui->gridLayout_deviceStatus->removeItem(gantryRow);
            ui->gridLayout_deviceStatus->addLayout(gantryRow, 2, 0, 1, deviceColumn);
        }
    }

    // 操作员姓名栏直接显示本次成功登录的用户名，
    // 不再从 userInfo 的 display_name/name 字段取显示名。
    const QString role = m_userInfo.value("role").toString("普通用户");

    // 这些控件名请和 .ui 里一致

    ui->label_6->setText(
        m_startupState.authenticated ? m_currentUsername : tr("未登录"));
    ui->label_user->setText(m_startupState.authenticated ? tr("角色：%1").arg(role) : QString());
    if (auto *employee = findChild<QLabel *>(QStringLiteral("clinicalEmployeeNoValue"))) {
        const QString employeeNo = operatorEmployeeNo(m_userInfo);
        employee->setText(m_startupState.authenticated && !employeeNo.isEmpty()
                              ? employeeNo : tr("未提供"));
    }

    ui->label_7->setText(m_roomId.trimmed().isEmpty() ? tr("未分配") : m_roomId);
    ui->label_room->setText(tr("状态：可用"));
    if (m_clinicalBody) {
        updateClinicalDeviceLayout();
        updateClinicalWorkspace();
    }
}

void TreatmentWindow::updateTodaySchedule(const QJsonArray &entries)
{
    m_todayScheduleEntries = entries;

    qDebug() << "缓存今日排程数量:" << m_todayScheduleEntries.size();

    if (m_todayScheduleDialog) {
        m_todayScheduleDialog->updateScheduleEntries(m_todayScheduleEntries);
    }
}

void TreatmentWindow::updateTimeLabel()
{
    // 例如：2026-04-03 15:42:10
    const QString nowText = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    ui->label_time->setText(nowText);
}

void TreatmentWindow::updateStepBar()
{
    if (m_stepView) {
        if (QObject *root = m_stepView->rootObject()) {
            root->setProperty("currentStep", treatmentStateToStep(m_treatmentState));
        }
    }
    const int step = treatmentStateToStep(m_treatmentState);
    for (int index = 0; index < 5; ++index) {
        if (auto *label = findChild<QLabel *>(QStringLiteral("clinicalStep%1").arg(index))) {
            label->setProperty("phaseState", m_workflowSyncRequired
                || m_treatmentState == TreatmentState::ErrorStopped || m_treatmentState == TreatmentState::Idle ? "unknown"
                : index < step ? "done" : index == step ? "current" : "waiting");
            label->style()->unpolish(label);
            label->style()->polish(label);
        }
    }
}

QString TreatmentWindow::currentWorkflowActionText() const
{
    if (m_workflowReauthPending) return tr("等待登录");
    if (m_workflowSyncRequired) return tr("等待现场状态同步");
    const QMap<QString, QString> backendActions = {
        {QStringLiteral("WAITING_LOGIN"), tr("等待登录")},
        {QStringLiteral("WAITING_FRONTEND_PLAN_CONFIRM"), tr("等待确认治疗计划")},
        {QStringLiteral("WAITING_DEVICE_PLAN_ACK"), tr("等待设备确认治疗计划")},
        {QStringLiteral("WAITING_DEVICE_AUTOPOS_DONE"), tr("自动摆位/影像比对")},
        {QStringLiteral("WAITING_FRONTEND_BEAM_APPLY"), tr("摆位完成，等待申请出束")},
        {QStringLiteral("WAITING_DEVICE_BEAM_READY"), tr("硬件准备束流中")},
        {QStringLiteral("TREATMENT_IN_PROGRESS"), tr("正在出束治疗")},
        {QStringLiteral("TREATMENT_PAUSED"), tr("治疗暂停")},
        {QStringLiteral("ERROR_STOPPED"), tr("流程异常中止，等待处理")},
        {QStringLiteral("WAITING_CHART_GENERATION"), tr("治疗结束，生成报表")}
    };
    if (backendActions.contains(m_backendWorkflowState))
        return backendActions.value(m_backendWorkflowState);
    // Normal task events update the local state without requiring another snapshot.
    switch (m_treatmentState) {
    case TreatmentState::Idle: return tr("等待后台任务");
    case TreatmentState::PlanConfirm: return tr("等待确认治疗计划");
    case TreatmentState::Preparing: return tr("自动摆位/影像比对");
    case TreatmentState::ReadyToApplyBeam: return tr("摆位完成，等待申请出束");
    case TreatmentState::BeamRequested: return tr("硬件准备束流中");
    case TreatmentState::BeamReady: return tr("束流已就绪，等待出束反馈");
    case TreatmentState::Treating: return tr("正在出束治疗");
    case TreatmentState::Paused: return tr("治疗暂停");
    case TreatmentState::BeamFinished: return tr("治疗结束，生成报表");
    case TreatmentState::ReportConfirm: return tr("等待确认治疗报告");
    case TreatmentState::Completed: return tr("本次流程完成");
    case TreatmentState::ErrorStopped: return tr("流程异常中止，等待处理");
    }
    return tr("等待流程状态");
}

void TreatmentWindow::clearGrpcCallDisplay()
{
    m_lastGrpcCall.clear();
    m_grpcCallHistory.clear();
}

void TreatmentWindow::handleGrpcCall(const QJsonObject &data)
{
    if (m_closing || m_workflowReauthPending || m_resetPending) return;
    if (!data.value(QStringLiteral("payload")).isObject()) return;
    const QJsonObject payload = data.value(QStringLiteral("payload")).toObject();
    if (!telemetryRoomMatches(data, payload, m_roomId)) return;
    const QString component = payload.value(QStringLiteral("component")).toString().trimmed();
    const QString method = payload.value(QStringLiteral("method")).toString().trimmed();
    if (component != QStringLiteral("Device") && component != QStringLiteral("Dispatch")
        && component != QStringLiteral("OIS")) return;
    static const QRegularExpression methodName(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    if (method.size() > 128 || !methodName.match(method).hasMatch()) return;

    const QString call = tr("%1 → %2").arg(component, method);
    // Consecutive duplicates need no extra rendering or unbounded history.
    if (call == m_lastGrpcCall) return;
    m_lastGrpcCall = call;
    m_grpcCallHistory.append(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))
                             + QLatin1Char(' ') + call);
    constexpr int maximumHistory = 20;
    while (m_grpcCallHistory.size() > maximumHistory) m_grpcCallHistory.removeFirst();
    updateTreatmentActionDisplay();
}

void TreatmentWindow::updateTreatmentActionDisplay()
{
    if (!ui->labelCurrentTreatmentAction) {
        return;
    }

    QString actionText = m_currentDeviceActionText.trimmed();
    if (actionText.isEmpty()) {
        actionText = tr("等待设备状态");
    }
    actionText.replace(QLatin1Char('\n'), QStringLiteral("；"));

    ui->labelCurrentTreatmentAction->setWordWrap(true);
    ui->labelCurrentTreatmentAction->setMinimumHeight(36);
    QSizePolicy actionPolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    actionPolicy.setHeightForWidth(true);
    ui->labelCurrentTreatmentAction->setSizePolicy(actionPolicy);
    setTreatmentRole(ui->labelCurrentTreatmentAction, QStringLiteral("treatmentStyle21"));

    // Only hide the redundant treatment-state/current-action *visible* text.
    // State transitions and the workflow overview's status rendering are unchanged.
    const QString workflowAction = currentWorkflowActionText();
    QString displayText;
    if (actionText != workflowAction && !m_currentDeviceActionText.trimmed().isEmpty()
        && actionText != tr("暂无有效动作反馈") && actionText != tr("等待设备状态"))
        displayText = tr("设备反馈：%1").arg(actionText);
    if (!m_lastGrpcCall.isEmpty()) {
        if (!displayText.isEmpty()) displayText += QLatin1Char('\n');
        displayText += tr("最近 gRPC 调用：%1（仅调用通知）").arg(m_lastGrpcCall);
    }
    auto *attention = static_cast<WorkflowTextLabel *>(findChild<QLabel *>(QStringLiteral("labelTreatmentAttention")));
    if (!attention) {
        // The safety/interlock card owns the visible warning. Before the new
        // workspace is built, fall back to the legacy flow layout temporarily.
        auto *safetyCard = findChild<QGroupBox *>(QStringLiteral("clinicalSafetyCard"));
        attention = new WorkflowTextLabel(safetyCard ? safetyCard : ui->groupBoxTreatmentFlow);
        attention->setObjectName(QStringLiteral("labelTreatmentAttention"));
        attention->setTextFormat(Qt::PlainText);
        attention->setWordWrap(true);
        attention->setSizePolicy(actionPolicy);
        if (safetyCard) {
            if (auto *safetyLayout = qobject_cast<QVBoxLayout *>(safetyCard->layout())) {
                // After the interlock/device indicators, before fault/connection details.
                safetyLayout->insertWidget(qMin(3, safetyLayout->count()), attention);
            }
        } else {
            ui->verticalLayout_flowBox->insertWidget(
                ui->verticalLayout_flowBox->indexOf(ui->labelCurrentTreatmentAction), attention);
        }
    }
    QStringList issues = m_currentDeviceIssues;
    const bool stopped = m_treatmentState == TreatmentState::ErrorStopped;
    if (stopped) issues.prepend(m_workflowErrorMessage.isEmpty() ? tr("后端未提供异常原因") : m_workflowErrorMessage);
    const auto interlock = TelemetryValue::binaryFlag(m_latestTelemetryPayload.value(QStringLiteral("interlock_ok")));
    if (interlock.isBool() && !interlock.toBool()) issues.append(tr("DDS联锁触发"));
    const bool beamFault = m_latestTelemetryPayload.value(QStringLiteral("beam_delivery_result")).toInt(-1) == 7;
    if (beamFault) issues.append(tr("DDS出束故障：%1")
        .arg(dispatchMonitorText(m_latestTelemetryPayload, 2).section(QLatin1Char('\n'), 0, 0)));
    issues.removeAll(QString());
    issues.removeDuplicates();
    attention->setProperty("statusTone", stopped || beamFault ? 4 : 3);
    setTreatmentRole(attention, QStringLiteral("workflowAttention"));
    attention->setMessage(tr("需要处理：%1").arg(issues.join(QStringLiteral("；"))));
    attention->setVisible(!issues.isEmpty());
    attention->setToolTip(attention->text() + (beamFault
        ? QLatin1Char('\n') + dispatchMonitorText(m_latestTelemetryPayload, 2) : QString()));
    ui->labelCurrentTreatmentAction->setTextFormat(Qt::PlainText);
    // Keep the state/action information available in the hover tooltip, not on screen.
    QString tooltip = tr("治疗状态：%1\n当前动作：%2")
        .arg(m_workflowSyncRequired ? tr("现场状态待同步") : treatmentStateText(m_treatmentState),
             workflowAction);
    if (!displayText.isEmpty()) tooltip += QLatin1Char('\n') + displayText;
    if (!issues.isEmpty()) tooltip += QLatin1Char('\n') + attention->text();
    if (!m_grpcCallHistory.isEmpty()) {
        tooltip += QStringLiteral("\n\n") + tr("最近 gRPC 调用顺序（本机接收时间，最多 20 条，连续重复合并）：")
            + QLatin1Char('\n') + m_grpcCallHistory.join(QLatin1Char('\n'))
            + QLatin1Char('\n') + tr("调用通知不代表执行成功或设备动作完成。");
    }
    ui->labelCurrentTreatmentAction->setToolTip(tooltip);
    static_cast<WorkflowTextLabel *>(ui->labelCurrentTreatmentAction)->setMessage(displayText);
    // Avoid leaving an empty strip between the step bar and the workflow diagram.
    ui->labelCurrentTreatmentAction->setVisible(!displayText.isEmpty());
    if (m_clinicalBody) {
        queueClinicalFaults(issues);
        updateClinicalWorkspace();
    }
}

void TreatmentWindow::updateAuditContext() const
{
    const QString role = m_userInfo.value("role").toString("unknown");
    QString operatorSub = m_userInfo.value("sub").toString();
    if (operatorSub.isEmpty()) {
        operatorSub = m_userInfo.value("id").toString();
    }
    if (operatorSub.isEmpty()) {
        operatorSub = m_userInfo.value("user_id").toString();
    }

    AuditLogger::instance().setContext(QStringLiteral("treatment"),
                                       m_currentUsername,
                                       role,
                                       m_roomId,
                                       operatorSub);
}

QJsonObject TreatmentWindow::currentAuditObjectPayload() const
{
    QJsonObject payload;

    QString patientId = m_currentPlanPayload.value("patientId").toString();
    if (patientId.isEmpty()) {
        patientId = m_currentPlanPayload.value("patient_id").toString();
    }

    QString courseId = m_currentPlanPayload.value("course_id").toString();
    if (courseId.isEmpty()) {
        courseId = m_currentPlanPayload.value("courseId").toString();
    }

    QString planVersionId =
        m_currentPlanPayload.value("plan_version_id").toString();
    if (planVersionId.isEmpty()) {
        planVersionId = m_currentPlanPayload.value("plan_id").toString();
    }

    QString sessionId = m_currentPlanPayload.value("session_id").toString();
    if (sessionId.isEmpty()) {
        sessionId = m_currentPlanPayload.value("treatment_id").toString();
    }
    if (sessionId.isEmpty()) {
        sessionId = m_currentPlanPayload.value("treatmentId").toString();
    }

    if (!patientId.isEmpty()) {
        payload["pt.medical_id"] = patientId;
    }
    if (!courseId.isEmpty()) {
        payload["pt.course_id"] = courseId;
    }
    if (!planVersionId.isEmpty()) {
        payload["pt.plan_version_id"] = planVersionId;
    }

    const QString fractionId =
        m_currentPlanPayload.value("fraction_id").toString();
    if (!fractionId.isEmpty()) {
        payload["pt.fraction_id"] = fractionId;
    }
    if (!sessionId.isEmpty()) {
        payload["pt.session_id"] = sessionId;
    }
    if (!m_roomId.isEmpty()) {
        payload["pt.room_id"] = m_roomId;
    }

    payload["tcs.treatment_state"] =
        treatmentStateText(m_treatmentState);

    return payload;
}

void TreatmentWindow::auditTreatmentEvent(const QString &category,
                                          const QString &action,
                                          const QString &objectType,
                                          const QString &objectId,
                                          const QString &result,
                                          const QString &message,
                                          const QJsonObject &payload,
                                          const QJsonObject &beforeState,
                                          const QJsonObject &afterState) const
{
    updateAuditContext();

    QJsonObject mergedPayload = currentAuditObjectPayload();
    for (auto it = payload.constBegin(); it != payload.constEnd(); ++it) {
        mergedPayload.insert(it.key(), it.value());
    }

    QString resolvedObjectType = objectType;
    if (resolvedObjectType.isEmpty()) {
        resolvedObjectType =
            mergedPayload.value("pt.plan_version_id").toString().isEmpty()
                                 ? QStringLiteral("treatment")
                                 : QStringLiteral("plan");
    }

    QString resolvedObjectId = objectId;
    if (resolvedObjectId.isEmpty()) {
        resolvedObjectId =
            mergedPayload.value("pt.plan_version_id").toString();
    }
    if (resolvedObjectId.isEmpty()) {
        resolvedObjectId = mergedPayload.value("pt.medical_id").toString();
    }

    AuditLogger::instance().logEvent(category,
                                     action,
                                     resolvedObjectType,
                                     resolvedObjectId,
                                     result,
                                     message,
                                     mergedPayload,
                                     beforeState,
                                     afterState);
}

void TreatmentWindow::loadMiddlewareSettings()
{
    QSettings configSettings(appConfigPath(), QSettings::IniFormat);
    const QString savedMiddlewareUrl =
        configSettings.value(QStringLiteral("Middleware/Url")).toString().trimmed();
    if (!savedMiddlewareUrl.isEmpty()) {
        m_middlewareUrl = savedMiddlewareUrl;
        m_wsMiddlewareUrl = savedMiddlewareUrl;
        qDebug() << "Loaded middleware URL from settings:" << m_middlewareUrl;
    } else {
        m_middlewareUrl.clear();
        m_wsMiddlewareUrl.clear();
        qDebug() << "Using default middleware URL:" << m_middlewareUrl;
    }

    const bool autoConnect =
        configSettings.value(QStringLiteral("Middleware/AutoConnect"), true).toBool();
    const int reconnectInterval =
        configSettings.value(QStringLiteral("Middleware/ReconnectInterval"), 5000).toInt();
    const int timeout =
        configSettings.value(QStringLiteral("Middleware/Timeout"), 30000).toInt();

    qDebug() << "middleware settings loaded - AutoConnect:" << autoConnect
             << "ReconnectInterval:" << reconnectInterval << "ms"
             << "Timeout:" << timeout << "ms";
}

void TreatmentWindow::connectWebSocketSignals()
{
    if (!m_webSocketClient)
        return;

    connect(m_webSocketClient, &WebSocketClient::connected,
            this, &TreatmentWindow::onWebSocketConnected, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::disconnected,
            this, &TreatmentWindow::onWebSocketDisconnected, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::errorOccurred,
            this, &TreatmentWindow::onWebSocketError, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::textMessageReceived,
            this, [](const QString &message) {
                qDebug() << "收到文本消息; characters:" << message.size();
            });

    connect(m_webSocketClient, &WebSocketClient::dataReceived,
            this, &TreatmentWindow::onWebSocketDataReceived, Qt::UniqueConnection);

    connect(m_webSocketClient, &WebSocketClient::dataSent,
            this, &TreatmentWindow::onWebSocketDataSent, Qt::UniqueConnection);
}

void TreatmentWindow::connectToMiddlewareWebSocket()
{
    if (!m_webSocketClient) {
        qWarning() << "WebSocketClient 为空";
        return;
    }

    if (m_webSocketClient->isConnected()) {
        qDebug() << "WebSocket 已连接，跳过重复连接";
        return;
    }

    if (m_wsMiddlewareUrl.isEmpty()) {
        qWarning() << "WebSocket 地址为空";
        statusBar()->showMessage("连接参数不完整", 5000);
        return;
    }

    const QUrl url(m_wsMiddlewareUrl);
    if (!url.isValid() || (url.scheme() != "ws" && url.scheme() != "wss")) {
        qWarning() << "无效的 WebSocket 地址:" << m_wsMiddlewareUrl;
        statusBar()->showMessage("网关地址无效", 5000);
        return;
    }

    QJsonObject authParams;
    if (m_startupState.authenticated && !m_authToken.isEmpty()) {
        authParams["token"] = m_authToken;
        authParams["client_type"] = "desktop";
        if (!m_currentUsername.isEmpty()) authParams["username"] = m_currentUsername;
    }

    const int reconnectInterval =
        m_settings->value(QStringLiteral("Middleware/ReconnectInterval"), 5000).toInt();
    m_webSocketClient->setMiddlewareAuthParams(authParams);
    m_webSocketClient->setAutoReconnect(true, reconnectInterval);

    statusBar()->showMessage("正在连接网关...", 3000);
    qDebug() << "开始连接 WebSocket:" << m_wsMiddlewareUrl;

    m_webSocketClient->connectToServer(m_wsMiddlewareUrl);
}

void TreatmentWindow::setClientModeSwitchHandler(ClientModeSwitchHandler handler)
{
    m_clientModeSwitchHandler = std::move(handler);
}

void TreatmentWindow::showLoginDialog()
{
    if (m_closing) return;
    if (loginDialog) {
        activateDialog(loginDialog);
        return;
    }
    m_loginTransitionPending = true;
    rejectWorkflowDialogs(this);
    applyTreatmentStateToUi();

    loginDialog = new LoginDialog(m_middlewareUrl, LoginDialog::AuthMode::Login, this,
                                 static_cast<bool>(m_clientModeSwitchHandler));
    loginDialog->setQaModeAvailable(m_startupState.initialized);
    const QPointer<LoginDialog> dialog = loginDialog;
    dialog->setObjectName(QStringLiteral("treatmentReloginDialog"));
    dialog->setWindowModality(Qt::ApplicationModal);

    connect(dialog, &LoginDialog::loginSuccess, this,
            [this, dialog](const QString &username, const QString &token,
                           WebSocketClient *client, const QJsonObject &response) {
        const QPointer<WebSocketClient> authenticatedClient(client);
        // Finish closing the modal login before restoring tasks or changing windows.
        QTimer::singleShot(0, this, [this, dialog, authenticatedClient, username, token, response] {
            if (m_closing || !dialog || loginDialog != dialog || !m_loginTransitionPending
                || dialog->result() != QDialog::Accepted) return;
            if (!authenticatedClient) {
                QMessageBox::critical(dialog, tr("登录失败"), tr("登录通信客户端已失效，请重新启动登录。"));
                if (dialog) dialog->reject();
                return;
            }
            onLoginSuccess(username, token, authenticatedClient, response);
        });
    });

    connect(dialog, &LoginDialog::loginFailed,
            this, &TreatmentWindow::onLoginFailed, Qt::UniqueConnection);

    connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
        if (m_closing) return;
        if (result != QDialog::Rejected || !dialog || loginDialog != dialog) return;
        dialog->clearPassword();
        loginDialog = nullptr;
        m_loginTransitionPending = false;
        dialog->deleteLater();
        applyTreatmentStateToUi();
        statusBar()->showMessage(tr("未登录，控制操作已锁定"), 4000);
    });

    dialog->show();
    activateDialog(dialog);
}

void TreatmentWindow::onLoginSuccess(const QString &username, const QString &authToken, WebSocketClient *client, const QJsonObject &loginResponse)
{
    if (m_closing || !loginDialog || !m_loginTransitionPending
        || loginDialog->result() != QDialog::Accepted) return;
    if (!client) {
        QMessageBox::critical(this, tr("登录失败"), tr("登录成功响应中没有可用的通信客户端。"));
        return;
    }
    if (m_logoutState.busy())
        auditTreatmentEvent("auth", "logout_unresolved_reauth", "request", m_logoutState.requestId,
            "warning", tr("注销结果未核实；新的后台认证会话取代本机旧会话，不补记注销成功"));
    m_logoutState = LogoutState();
    m_logoutTimer->stop();
    m_closeAfterLogout = false;
    m_deferredPlanConfirmation = QJsonObject();

    if (loginDialog->selectedClientMode() == LoginDialog::ClientMode::Qa && !m_startupState.initialized) {
        if (auto *pendingClient = loginDialog->takeClient()) {
            pendingClient->setAutoReconnect(false);
            pendingClient->disconnectFromServer();
            pendingClient->deleteLater();
        }
        loginDialog->clearPassword();
        loginDialog->reject();
        statusBar()->showMessage(tr("后台初始化未完成，暂不能进入 QA 模式"), 5000);
        return;
    }

    if (loginDialog && m_clientModeSwitchHandler
        && loginDialog->selectedClientMode() != LoginDialog::ClientMode::Treatment) {
        const auto mode = loginDialog->selectedClientMode();
        ClientSession session;
        session.webSocketClient = loginDialog->takeClient();
        session.username = username;
        session.authToken = authToken;
        session.userInfo = loginDialog->getUserInfo();
        session.loginResponse = loginResponse;
        // QA does not require retaining the login password.
        session.webSocketClient->setParent(this);
        loginDialog->clearPassword();
        loginDialog->deleteLater();
        loginDialog = nullptr;
        m_workflowReauthPending = true;
        if (m_workflowTimer) m_workflowTimer->stop();
        if (m_webSocketClient) {
            disconnect(m_webSocketClient, nullptr, this, nullptr);
            m_webSocketClient->setAutoReconnect(false);
            m_webSocketClient->disconnectFromServer();
        }
        m_clientModeSwitchHandler(mode, session);
        return;
    }

    client = loginDialog->takeClient();
    if (!client) {
        QMessageBox::critical(loginDialog, tr("登录失败"), tr("登录通信客户端不可用，请重新启动登录。"));
        if (loginDialog) loginDialog->reject();
        return;
    }
    m_currentUsername = username;
    m_startupState.authenticated = true;
    m_loginPassword = loginDialog ? loginDialog->getPassword() : QString();
    m_authToken = authToken;
    m_workflowReauthPending = false;
    m_workflowSyncRequired = true;
    if (loginResponse.value("user").isObject()) {
        m_userInfo = loginResponse.value("user").toObject();
    }
    const QJsonObject payload = loginResponse.value("payload").toObject();
    const QString previousRoom = m_roomId;
    const QString loginRoomId = normalizeRoomId(payload.value(QStringLiteral("room_id")));
    m_roomId = loginRoomId.isEmpty()
        ? m_settings->value(QStringLiteral("Treatment/RoomId")).toString().trimmed()
        : loginRoomId;
    if (m_roomId != previousRoom) m_startupState.initialized = false;
    m_activeBeamRoomId = payload.value("active_beam_room_id").toString(
        payload.value("beam_room_id").toString());

    m_todayScheduleEntries = payload.value("entries").toArray();

    if (m_todayScheduleEntries.isEmpty()) {
        m_todayScheduleEntries = loginResponse.value("entries").toArray();
    }
    updateTodaySchedule(m_todayScheduleEntries);

    qDebug() << "登录成功携带今日排程数量:" << m_todayScheduleEntries.size();
    updateAuditContext();
    QJsonObject auditPayload;
    auditPayload["schedule_count"] = m_todayScheduleEntries.size();
    auditPayload["room_id"] = payload.value("room_id").toString(m_roomId);
    auditTreatmentEvent(QStringLiteral("auth"),
                        QStringLiteral("login_success"),
                        QStringLiteral("user"),
                        username,
                        QStringLiteral("success"),
                        tr("用户登录治疗客户端成功"),
                        auditPayload);

    if (m_webSocketClient) {
        m_webSocketClient->setAutoReconnect(false);
        disconnect(m_webSocketClient, nullptr, this, nullptr);
        m_webSocketClient->disconnectFromServer();
        m_webSocketClient->deleteLater();
        m_webSocketClient = nullptr;
    }

    m_webSocketClient = client;
    m_webSocketClient->setParent(this);

    connectWebSocketSignals();

    loginDialog->clearPassword();
    loginDialog->deleteLater();
    loginDialog = nullptr;
    m_loginTransitionPending = false;
    updateUserInfoUi();
    updateBeamRoomOverview(m_activeBeamRoomId);
    updateSessionUi();
    show();

    if (m_webSocketClient->isConnected()) {
        onWebSocketConnected();
    } else {
        connectToMiddlewareWebSocket();
    }
    statusBar()->showMessage(tr("登录成功：%1").arg(username), 5000);

}

void TreatmentWindow::onLoginFailed(const QString &error)
{
    qDebug() << "登录失败:" << error;
    auditTreatmentEvent(QStringLiteral("auth"),
                        QStringLiteral("login_failed"),
                        QStringLiteral("user"),
                        m_currentUsername,
                        QStringLiteral("failed"),
                        error);

    // LoginDialog owns failure prompts; do not open a competing parent-window modal.
}

void TreatmentWindow::onWebSocketConnected()
{
    statusBar()->showMessage("已连接到网关", 3000);
    auditTreatmentEvent(QStringLiteral("network"),
                        QStringLiteral("websocket_connected"),
                        QStringLiteral("gateway"),
                        m_wsMiddlewareUrl,
                        QStringLiteral("success"),
                        tr("WebSocket 已连接"));
    m_workflowSyncRequired = true;
    applyTreatmentStateToUi();
    restorePendingWorkflowMessages();
    QTimer::singleShot(0, this, [this] {
        if (m_workflowSyncRequired) queryWorkflowState();
    });
}

void TreatmentWindow::onWebSocketDisconnected()
{
    m_deferredPlanConfirmation = QJsonObject();
    m_logoutState.invalidateBeam();
    if (m_logoutState.busy()) m_logoutState.phase = LogoutState::Phase::Uncertain;
    if (m_logoutTimer) m_logoutTimer->stop();
    m_closeAfterLogout = false;
    m_startupState.authenticated = false;
    m_startupState.initialized = false;
    m_startupState.failure.clear();
    if (m_webSocketClient) m_webSocketClient->setMiddlewareAuthParams(QJsonObject());
    m_authToken.clear();
    m_loginPassword.fill(QChar(u'\0'));
    m_loginPassword.clear();
    ++m_workflowRevision;
    m_workflowConfirmationPending = false;
    m_planConfirmationPending = false;
    m_lastWorkflowSnapshot = QJsonObject();
    clearGrpcCallDisplay();
    m_workflowSyncRequired = true;
    updateBeamMonitorSummary();
    if (m_workflowTimer) m_workflowTimer->stop();
    m_resetPending = false;
    rejectWorkflowDialogs(this, loginDialog);
    if (m_todayScheduleDialog) m_todayScheduleDialog->close();
    applyTreatmentStateToUi();
    qDebug() << "WebSocket 已断开";
    statusBar()->showMessage("WebSocket 已断开", 3000);
    auditTreatmentEvent(QStringLiteral("network"),
                        QStringLiteral("websocket_disconnected"),
                        QStringLiteral("gateway"),
                        m_wsMiddlewareUrl,
                        QStringLiteral("warning"),
                        tr("WebSocket 已断开"));
    m_currentUsername.clear();
    m_userInfo = QJsonObject();
    updateUserInfoUi();
}

void TreatmentWindow::onWebSocketError(const QString &error)
{
    qDebug() << "WebSocket 错误:" << error;
    statusBar()->showMessage("连接错误: " + error, 5000);
    auditTreatmentEvent(QStringLiteral("network"),
                        QStringLiteral("websocket_error"),
                        QStringLiteral("gateway"),
                        m_wsMiddlewareUrl,
                        QStringLiteral("failed"),
                        error);

    if (m_startupState.authenticated && (error.contains("401") ||
        error.contains("auth", Qt::CaseInsensitive) ||
        error.contains("token", Qt::CaseInsensitive))) {
        QMessageBox::warning(this, "认证错误", "网关认证失败，请重新登录");
    }
}

void TreatmentWindow::onWebSocketDataSent(const QString &message)
{
    qDebug() << "WebSocket data sent; characters:" << message.size();
}

void TreatmentWindow::onWebSocketDataReceived(const QJsonObject &data)
{
    if (handleLogoutReply(data)) return;
    handleInitializationNotification(data);
    const QString type = data.value("type").toString();

    if (type == "RSP") {
        handleRspMessage(data);
    } else if (type == "TELEMETRY") {
        handleTelemetryMessage(data);
    } else if (type == "TASK") {
        handleTaskMessage(data);
    } else if (type == "TASK_ERROR_STOPPED") {
        handleTaskErrorStopped(data);
    } else {
        qDebug() << "未处理的消息类型:" << type << maskSensitivePayload(data);
    }
}

void TreatmentWindow::handleRspMessage(const QJsonObject &data)
{
    // RSP.result is a network admission ACK only, not execution success.
    if (data.value(QStringLiteral("result")).isBool()
        && !data.value(QStringLiteral("result")).toBool()) {
        const QString cmd = data.value(QStringLiteral("cmd")).toString();
        statusBar()->showMessage(tr("后端拒绝接收指令：%1；操作未被确认执行").arg(cmd), 8000);
        auditTreatmentEvent(QStringLiteral("backend"),
            QStringLiteral("command_rejected_by_rsp"),
            QStringLiteral("command"), cmd, QStringLiteral("failed"),
            tr("收到 result=false 的网络应答，未据此改变治疗状态"));
    }
}

void TreatmentWindow::handleTelemetryMessage(const QJsonObject &data)
{
    const QString cmd = data.value("cmd").toString();
    const bool supportedTelemetryCommand =
        cmd == "DEVICE_TELEMETRY_STREAM"
        || cmd == "DISPATCH_TELEMETRY_STREAM"
        || cmd == "DEVICE_STATUS_STREAM"
        || cmd == "DISPATCH_DEVICE_STATUS_STREAM"
        || cmd == "DISPATCH_STATUS_STREAM"
        || cmd == "STATUS_UPDATE";
    if (!supportedTelemetryCommand) {
        return;
    }

    QJsonObject payload = data.value("payload").toObject();
    if (payload.isEmpty()) {
        return;
    }
    // Filter cross-room telemetry BEFORE updating any device indicators,
    // beam safety observations, plan progress, position readbacks or spot map.
    if (!telemetryRoomMatches(data, payload, m_roomId)) {
        qWarning() << "Ignoring telemetry for a different or invalid room:" << cmd;
        return;
    }
    const QJsonObject nestedStatus = firstObjectValue(
        payload, {QStringLiteral("status_update"), QStringLiteral("statusUpdate"),
                  QStringLiteral("status"), QStringLiteral("device_status"),
                  QStringLiteral("deviceStatus")});
    if (!nestedStatus.isEmpty() && !telemetryRoomMatches(data, nestedStatus, m_roomId)) {
        qWarning() << "Ignoring telemetry with a mismatched nested status room:" << cmd;
        return;
    }

    // Normalize the backend enum before display and logout-state consumers.
    const QJsonValue rawDeliveryResult = payload.value(QStringLiteral("beam_delivery_result"));
    if (rawDeliveryResult.isString()) {
        static const QMap<QString, int> deliveryCodes = {
            {QStringLiteral("BEAM_WAITING"), 0},
            {QStringLiteral("BEAM_TREATING"), 1},
            {QStringLiteral("BEAM_COMPLETED"), 2},
            {QStringLiteral("BEAM_FAULT"), 7}
        };
        payload.insert(QStringLiteral("beam_delivery_result"),
                       deliveryCodes.value(rawDeliveryResult.toString(), -1));
    }

    // The existing telemetry decoder defines which binary representations
    // are valid; never coerce an unknown value to false.
    for (const QString &flag : {QStringLiteral("beam_on"),
                                QStringLiteral("is_delivering"),
                                QStringLiteral("interlock_ok"),
                                QStringLiteral("dds_ilk_en_rb"),
                                QStringLiteral("accel_ready"),
                                QStringLiteral("g01_homed"),
                                QStringLiteral("g02_homed")}) {
        if (!payload.contains(flag)) continue;
        const QJsonValue normalized = TelemetryValue::binaryFlag(payload.value(flag));
        if (normalized.isBool()) payload.insert(flag, normalized);
    }

    updateDeviceStatusIndicators(payload);
    // DISPATCH_TELEMETRY_STREAM: sum_dose is cumulative MU, while
    // TASK_SYNC_WORKFLOW_STATE.cumulative_dose is cGy -- no conversion.
    if (cmd == QStringLiteral("DISPATCH_TELEMETRY_STREAM")) {
        if (auto *admission = findChild<QLabel *>(QStringLiteral("clinicalAdmissionTelemetryLabel"))) {
            const int ticket = readIntValue(payload, {"ticket"});
            const int position = readIntValue(payload, {"position"});
            const int eta = readIntValue(payload, {"eta_seconds"});
            const int state = readIntValue(payload, {"state"});
            static const QStringList states = {
                QObject::tr("等待"), QObject::tr("准入批准"), QObject::tr("已过期"),
                QObject::tr("已取消"), QObject::tr("机架准备中"), QObject::tr("调度失败")
            };
            if (ticket >= 0 || position >= 0 || eta >= 0 || state >= 0) {
                admission->setText(tr("号牌：%1 | 队列位置：%2 | ETA：%3 s | %4")
                    .arg(ticket >= 0 ? QString::number(ticket) : QStringLiteral("-"))
                    .arg(position >= 0 ? QString::number(position) : QStringLiteral("-"))
                    .arg(eta >= 0 ? QString::number(eta) : QStringLiteral("-"))
                    .arg(state >= 0 && state < states.size()
                        ? states.at(state) : tr("状态未知")));
                admission->setToolTip(tr("准入排队信息只供显示，不作为出束许可"));
            }
        }
    }
    if (!payload.contains(QStringLiteral("current_delivered_mu"))
        && payload.value(QStringLiteral("sum_dose")).isDouble()) {
        payload.insert(QStringLiteral("current_delivered_mu"),
                       payload.value(QStringLiteral("sum_dose")));
    }
    if (!payload.contains(QStringLiteral("therapy_fault_info"))
        && payload.contains(QStringLiteral("therapy_fault_ifo"))) {
        // The interface document spells this field *ifo*; legacy code used info.
        payload.insert(QStringLiteral("therapy_fault_info"),
                       payload.value(QStringLiteral("therapy_fault_ifo")));
    }
    if (!payload.contains("beam_on") && payload.contains("is_delivering")) {
        payload.insert("beam_on", payload.value("is_delivering"));
    }
    if (payload.contains(QStringLiteral("beam_delivery_result"))) {
        const bool matchingRoom = telemetryRoomMatches(data, payload, m_roomId);
        if (matchingRoom && !payload.value(QStringLiteral("beam_delivery_result_is_snapshot")).toBool())
            m_logoutState.observeDeliveryResult(payload.value(QStringLiteral("beam_delivery_result")));
        else m_logoutState.invalidateBeam();
    }
    if (payload.contains(QStringLiteral("beam_on"))) {
        const bool matchingRoom = telemetryRoomMatches(data, payload, m_roomId);
        if (matchingRoom && !payload.value(QStringLiteral("beam_on_is_snapshot")).toBool())
            m_logoutState.observeBeam(payload.value(QStringLiteral("beam_on")));
        else m_logoutState.invalidateBeam();
    }
    ui->spotMapWidget->setTelemetry(payload);

    QJsonObject mergedDoseTelemetry = m_latestTelemetryPayload;
    const int previousLayer = readIntValue(mergedDoseTelemetry, {"current_layer"});
    const int incomingLayer = readIntValue(payload, {"current_layer"});
    if (incomingLayer >= 0 && incomingLayer != previousLayer) {
        mergedDoseTelemetry.remove(QStringLiteral("layer_sum_dose"));
        mergedDoseTelemetry.remove(QStringLiteral("actual_mu"));
        mergedDoseTelemetry.remove(QStringLiteral("delivered_mu"));
    }

    const QStringList doseTelemetryKeys = {
        QStringLiteral("current_delivered_mu"), QStringLiteral("target_mu"), QStringLiteral("energy"),
        QStringLiteral("sum_dose"), QStringLiteral("beam_on"),
        QStringLiteral("beam_delivery_result"), QStringLiteral("current_dose"),
        QStringLiteral("layer_sum_dose"), QStringLiteral("dose_rate"),
        QStringLiteral("current_point"), QStringLiteral("next_layer"),
        QStringLiteral("therapy_id"), QStringLiteral("interlock_ok"),
        QStringLiteral("dds_ilk_en_rb"), QStringLiteral("accel_ready"),
        QStringLiteral("g01_homed"), QStringLiteral("g02_homed"),
        QStringLiteral("therapy_fault_code"), QStringLiteral("therapy_fault_info"),
        QStringLiteral("therapy_fault_ifo"),
        QStringLiteral("point_fault_code"), QStringLiteral("layer_fault_code"),
        QStringLiteral("current_layer"),
        QStringLiteral("current_beam"),
        QStringLiteral("current_beam_number"),
        QStringLiteral("beam_number"),
        QStringLiteral("cumulative_dose"),
        QStringLiteral("actual_mu"),
        QStringLiteral("delivered_mu"),
        QStringLiteral("dose_deviation"),
        QStringLiteral("dose_deviation_percent"),
        QStringLiteral("interruption_status"),
        QStringLiteral("is_delivering"),
        QStringLiteral("interruption_compensation_status"),
        QStringLiteral("compensation_status")
    };
    for (const QString &key : doseTelemetryKeys) {
        if (payload.contains(key)) {
            mergedDoseTelemetry.insert(key, payload.value(key));
        }
    }
    if (payload.value(QStringLiteral("current_delivered_mu")).isDouble())
        mergedDoseTelemetry.insert(QStringLiteral("workflow_mu_is_snapshot"), false);
    if (payload.contains(QStringLiteral("dose_rate")))
        mergedDoseTelemetry.insert(QStringLiteral("dose_rate_is_snapshot"), false);
    if (payload.contains(QStringLiteral("cumulative_dose")))
        mergedDoseTelemetry.insert(QStringLiteral("cumulative_dose_is_snapshot"), false);
    if (payload.contains(QStringLiteral("beam_on")))
        mergedDoseTelemetry.insert(QStringLiteral("beam_on_is_snapshot"), false);

    const QString doseSnapshot = doseTelemetrySnapshot(mergedDoseTelemetry);
    const bool shouldUpdateDosePanel = !doseSnapshot.isEmpty()
        && doseSnapshot != m_lastDoseTelemetrySnapshot;

    m_latestTelemetryPayload = mergedDoseTelemetry;
    applyTreatmentStateToUi();
    auto *monitor = findChild<QLabel *>(QStringLiteral("labelDispatchMonitor"));
    if (!monitor) {
        monitor = new QLabel(ui->groupBoxBedPosition);
        monitor->setObjectName(QStringLiteral("labelDispatchMonitor"));
        monitor->setWordWrap(true);
        monitor->setTextFormat(Qt::PlainText);
        setTreatmentRole(monitor, QStringLiteral("dispatchMonitor"));
        ui->verticalLayout_bedBox->addWidget(monitor);
    }
    updateBeamMonitorSummary();
    updateTreatmentActionDisplay();
    updateDdsInterlock(ui->groupBoxSystemStatus, mergedDoseTelemetry);
    updateGantryIndicators(this, mergedDoseTelemetry);
    if (shouldUpdateDosePanel) {
        m_lastDoseTelemetrySnapshot = doseSnapshot;
        updateDoseInfoPanel(m_currentPlanPayload.isEmpty() ? payload : m_currentPlanPayload);
    }

    const QString activeBeamRoomId = payload.value("active_beam_room_id").toString(
        payload.value("beam_room_id").toString());
    if (!activeBeamRoomId.isEmpty() && activeBeamRoomId.trimmed() != m_activeBeamRoomId) {
        updateBeamRoomOverview(activeBeamRoomId.trimmed());
    }

    QJsonObject couch = payload.value("couch").toObject();
    if (couch.isEmpty()) {
        couch = firstObjectValue(
            nestedStatus, {QStringLiteral("couch"), QStringLiteral("couch_status")});
    }
    if (couch.isEmpty()) {
        return;
    }

    const auto updateCouchAxis = [&couch](const char *fieldName,
                                          QLabel *valueLabel,
                                          const char *controllerKey) {
        const QJsonValue value = couch.value(QLatin1String(fieldName));
        if (!value.isDouble()) {
            return;
        }


        const double axisValue = value.toDouble();
        setLabelTextIfChanged(valueLabel, QString::number(axisValue, 'f', 2));
        DataProvider::instance().updateValue(QLatin1String(controllerKey), axisValue);
    };

    updateCouchAxis("a", ui->label_posa, "bed.a1");
    updateCouchAxis("b", ui->label_posb, "bed.a2");
    updateCouchAxis("c", ui->label_posc, "bed.a3");
    updateCouchAxis("x", ui->label_posx, "bed.a4");
    updateCouchAxis("y", ui->label_posy, "bed.a5");
    updateCouchAxis("z", ui->label_posz, "bed.a6");
    if (m_bedView) {
        m_bedView->show();
        bool hasAllAxisValues = true;
        for (QLabel *label : {ui->label_posa, ui->label_posb, ui->label_posc,
                              ui->label_posx, ui->label_posy, ui->label_posz})
            hasAllAxisValues = hasAllAxisValues && label->text() != QStringLiteral("-");
        m_bedView->setToolTip(hasAllAxisValues
            ? tr("三维模型仅供示意；位置请结合数值反馈和实机状态核对。")
            : tr("等待完整位置反馈；三维模型仅供示意，不代表已确认的实机位置。"));
    }
}

void TreatmentWindow::handleTaskMessage(const QJsonObject &data)
{
    const QString cmd = data.value("cmd").toString();
    const QJsonObject payload = data.value("payload").toObject();

    if (m_closing || m_workflowReauthPending) return;
    // Login-success may establish a new room; all other room-tagged TASKs
    // must not mutate another treatment room's workflow state.
    if (cmd != QStringLiteral("TASK_LOGIN_SUCCESS")
        && !telemetryRoomMatches(data, payload, m_roomId)) {
        qWarning() << "Ignoring task for a different or invalid room:" << cmd;
        return;
    }
    // An error-stop is a TASK command according to the wire protocol.
    if (cmd == QStringLiteral("TASK_ERROR_STOPPED")) {
        handleTaskErrorStopped(data);
        return;
    }
    if (cmd == QStringLiteral("TASK_BEAM_ON")) {
        m_logoutState.observeBeam(true);
        applyTreatmentStateToUi();
    }
    if (cmd == QStringLiteral("TASK_WORKFLOW_RESET") && !sessionReady()) {
        if (!telemetryRoomMatches(QJsonObject(), payload, m_roomId)) return;
        handleWorkflowReset();
        return;
    }
    // The observer path never acknowledges tasks or opens clinical confirmations.
    if (!sessionReady()) {
        if (cmd == QStringLiteral("TASK_SYNC_WORKFLOW_STATE")) handleWorkflowStateSync(payload);
        else if (cmd == QStringLiteral("TASK_GRPC_CALL")) handleGrpcCall(data);
        else if (cmd == QStringLiteral("TASK_PLAN_CONFIRM") && m_startupState.authenticated
                 && !m_startupState.initialized && !payload.isEmpty()) {
            if (telemetryRoomMatches(data, payload, m_roomId)) {
                m_deferredPlanConfirmation = data;
                statusBar()->showMessage(tr("已收到待确认计划；等待后台初始化完成后显示确认窗口"), 5000);
            }
        }
        return;
    }
    if (cmd == QStringLiteral("TASK_WORKFLOW_RESET")) {
        if (!telemetryRoomMatches(QJsonObject(), payload, m_roomId)) return;
        handleWorkflowReset();
        return;
    }
    if (cmd == QStringLiteral("TASK_SYNC_WORKFLOW_STATE")) {
        handleWorkflowStateSync(payload);
        return;
    }

    if (cmd == "TASK_ERROR") {
        const QString message = data.value("message").toString(
            payload.value("message").toString(tr("后端返回 TASK_ERROR")));
        auditTreatmentEvent(QStringLiteral("backend"),
                            QStringLiteral("task_error_received"),
                            QStringLiteral("task"),
                            cmd,
                            QStringLiteral("failed"),
                            message,
                            payload);
        statusBar()->showMessage(tr("网关返回错误：%1").arg(message), 5000);
        return;
    }

    if (cmd == "TASK_GRPC_CALL") {
        handleGrpcCall(data);
        return;
    }

    if (cmd == QStringLiteral("TASK_DDS_FAULT_ALERT")) {
        const QString faultDesc = payload.value(QStringLiteral("fault_desc")).toString(
            payload.value(QStringLiteral("dds_fault_desc")).toString(
                tr("DDS 报告故障，未提供详细说明")));
        statusBar()->showMessage(tr("DDS故障报警：%1").arg(faultDesc));
        auditTreatmentEvent(QStringLiteral("fault"),
            QStringLiteral("dds_fault_alert_received"),
            QStringLiteral("room"), m_roomId, QStringLiteral("warning"), faultDesc, payload);
        // This is an alert, not a confirmed all-clear or permission to reset.
        return;
    }
    if (cmd == QStringLiteral("TASK_DDS_FAULT_RESET_RESULT")) {
        const QJsonValue success = payload.value(QStringLiteral("success"));
        const QString message = payload.value(QStringLiteral("message")).toString(
            payload.value(QStringLiteral("error")).toString());
        statusBar()->showMessage(success.isBool() && success.toBool()
            ? tr("DDS复位响应：%1；待设备重新上报联锁回读").arg(message)
            : tr("DDS复位未获确认：%1").arg(message));
        auditTreatmentEvent(QStringLiteral("fault"),
            QStringLiteral("dds_fault_reset_result_received"),
            QStringLiteral("room"), m_roomId,
            success.isBool() && success.toBool() ? QStringLiteral("info") : QStringLiteral("warning"),
            message, payload);
        return;
    }
    if (cmd == QStringLiteral("TASK_REGISTRATION_RESULT")) {
        const QString decision = payload.value(QStringLiteral("decision")).toString(
            payload.value(QStringLiteral("position_result_decision")).toString());
        const QJsonObject shift = payload.value(QStringLiteral("shift")).toObject();
        const QString text = decision == QStringLiteral("PASSED")
            ? tr("图像配准报告：PASSED；仅供显示，仍需等待后端移床与许可状态")
            : decision == QStringLiteral("REJECTED")
                ? tr("图像配准未通过：REJECTED；请复核影像和摆位")
                : tr("收到图像配准结果，状态未识别：%1").arg(decision);
        statusBar()->showMessage(text);
        auditTreatmentEvent(QStringLiteral("imaging"),
            QStringLiteral("registration_result_received"),
            QStringLiteral("treatment"),
            payload.value(QStringLiteral("treatment_id")).toString(),
            decision == QStringLiteral("PASSED") ? QStringLiteral("info")
                : QStringLiteral("warning"), text, payload);
        Q_UNUSED(shift)
        // Do not drive the couch or unlock any treatment action from this view.
        return;
    }

    if (m_resetPending && cmd != QStringLiteral("TASK_LOGIN_SUCCESS")) return;
    // A fresh live plan is authoritative for the normal call workflow too.
    // Do not make legacy task delivery depend on a separate snapshot response.
    if (m_workflowSyncRequired && cmd != QStringLiteral("TASK_LOGIN_SUCCESS")
        && cmd != QStringLiteral("TASK_WAITING_OIS_PATIENT_CALL")
        && cmd != QStringLiteral("TASK_PLAN_CONFIRM")) return;

    if (cmd == "TASK_WAITING_OIS_PATIENT_CALL") {
        if (!telemetryRoomMatches(QJsonObject(), payload, m_roomId)) return;
        if (m_treatmentState != TreatmentState::Idle
            && m_treatmentState != TreatmentState::Completed) return;
        if (!setTreatmentState(TreatmentState::Idle, cmd)) return;
        m_workflowSyncRequired = false;
        m_workflowTimer->stop();
        applyTreatmentStateToUi();
        ui->spotMapWidget->clearPlan();
        if (!m_waitingOisPatientCallLogged) {
            auditTreatmentEvent(QStringLiteral("workflow"),
                                QStringLiteral("waiting_ois_patient_call"),
                                QStringLiteral("task"),
                                cmd,
                                QStringLiteral("info"),
                                tr("等待 OIS 患者叫号"),
                                payload);
            m_waitingOisPatientCallLogged = true;
        }
        statusBar()->showMessage(tr("等待 OIS 患者叫号..."), 3000);
        return;
    }

    if (cmd == "TASK_LOGIN_SUCCESS") {
        const QString roomId = normalizeRoomId(payload.value("room_id"));
        const QString activeBeamRoomId = payload.value("active_beam_room_id").toString(
            payload.value("beam_room_id").toString(m_roomId));

        if (!roomId.isEmpty()) m_roomId = roomId;
        m_activeBeamRoomId = activeBeamRoomId;
        const QJsonArray schedule = payload.value("entries").toArray(
            payload.value("data").toArray());
        if (!schedule.isEmpty()) updateTodaySchedule(schedule);
        statusBar()->showMessage(QString("登录成功，治疗室: %1").arg(roomId), 3000);
        updateUserInfoUi();
        updateBeamRoomOverview(m_activeBeamRoomId);
        updateDoseInfoPanel(m_currentPlanPayload);

        auditTreatmentEvent(QStringLiteral("backend"),
                            QStringLiteral("task_login_success_received"),
                            QStringLiteral("room"),
                            roomId,
                            QStringLiteral("success"),
                            tr("后端返回治疗室登录成功"),
                            payload);
        // Login does not imply an idle workflow. The snapshot is authoritative.
        applyTreatmentStateToUi();
        QTimer::singleShot(0, this, [this] {
            if (m_workflowSyncRequired) queryWorkflowState();
        });
        return;
    }

    if (cmd == "TASK_PLAN_CONFIRM") {
        if (payload.isEmpty() || !telemetryRoomMatches(QJsonObject(), payload, m_roomId)) {
            auditTreatmentEvent(QStringLiteral("workflow"), QStringLiteral("live_plan_rejected"),
                QStringLiteral("room"), m_roomId, QStringLiteral("failed"), tr("计划确认报文为空或治疗室不匹配"));
            return;
        }
        const bool samePlan = !payload.value("plan_id").toString().isEmpty()
            && payload.value("plan_id") == m_currentPlanPayload.value("plan_id")
            && payload.value("fraction_id") == m_currentPlanPayload.value("fraction_id")
            && payload.value("patient_id") == m_currentPlanPayload.value("patient_id");
        if (m_planConfirmationPending && samePlan) return;
        qDebug().noquote()
            << QStringLiteral("[Plan Raw Payload]\n%1")
                   .arg(QString::fromUtf8(
                       QJsonDocument(payload).toJson(QJsonDocument::Indented)));

        m_waitingOisPatientCallLogged = false;
        if (!setTreatmentState(TreatmentState::PlanConfirm, cmd)) {
            qWarning() << "拒绝在当前治疗状态下切换到新计划:" << data;
            return;
        }

        if (!samePlan) {
            ++m_workflowRevision;
            clearGrpcCallDisplay();
        }
        m_workflowSyncRequired = false;
        m_workflowTimer->stop();
        m_backendWorkflowState = QStringLiteral("WAITING_FRONTEND_PLAN_CONFIRM");
        applyTreatmentStateToUi();

        m_latestTelemetryPayload = QJsonObject();
        m_lastDoseTelemetrySnapshot.clear();
        m_currentPlanPayload = payload;
        ui->spotMapWidget->setPlanPayload(payload);
        // The plan-confirm message is the authoritative initial target MU.
        // It does not assert how much MU has actually been delivered.
        updateDoseInfoPanel(payload);
        auditTreatmentEvent(QStringLiteral("treatment_plan"),
                            QStringLiteral("plan_confirm_request_received"),
                            QStringLiteral("plan"),
                            payload.value("plan_id").toString(),
                            QStringLiteral("success"),
                            tr("收到治疗计划确认请求"),
                            payload);

        const PlanDisplayInfo plan = parsePlanDisplayInfo(payload);

        ui->patient_id->setText(tr("ID：%1").arg(plan.patientId));
        ui->patient_id->setToolTip(tr("病历号：%1").arg(
            payload.value(QStringLiteral("patient_mrn")).toString(QStringLiteral("-"))));
        ui->patient_name->setText(tr("姓名：%1").arg(plan.patientName));

        ui->labelPlanIdValue->setText(plan.planId);
        ui->labelPlanFractionIdValue->setText(plan.fractionId);
        ui->labelPlanSiteTargetValue->setText(plan.targetArea);
        ui->labelPlanIsocenterValue->setText(plan.isocenterText);
        ui->labelPlanQaReleaseValue->setText(tr("%1 / %2").arg(plan.qaStatus, plan.releaseStatus));

        scheduleWorkflowConfirmation();

        return;
    }

    if (cmd == "TASK_CAN_APPLY_BEAM") {
        if (!telemetryRoomMatches(QJsonObject(), payload, m_roomId)) return;
        auditTreatmentEvent(QStringLiteral("beam"),
                            QStringLiteral("can_apply_beam_received"),
                            QStringLiteral("beam"),
                            m_roomId,
                            QStringLiteral("success"),
                            tr("收到可申请束流通知"),
                            payload);
        if (setTreatmentState(TreatmentState::ReadyToApplyBeam, cmd)) scheduleWorkflowConfirmation();

        return;
    }

    if (cmd == "TASK_BEAM_READY") {
        auditTreatmentEvent(QStringLiteral("beam"),
                            QStringLiteral("beam_ready_received"),
                            QStringLiteral("beam"),
                            m_roomId,
                            QStringLiteral("success"),
                            tr("收到束流就绪通知"),
                            payload);
        setTreatmentState(TreatmentState::BeamReady, cmd);
        return;
    }

    if (cmd == "TASK_BEAM_PAUSED"
        || cmd == "TASK_BEAM_RESUMED"
        || cmd == "TASK_TREATMENT_STOPPED") {
        const bool paused = cmd == "TASK_BEAM_PAUSED";
        const bool treatmentStopped = cmd == "TASK_TREATMENT_STOPPED";
        const QString title = treatmentStopped
            ? tr("治疗停止通知")
            : (paused ? tr("束流暂停通知") : tr("束流恢复通知"));
        const QString summary = treatmentStopped
            ? tr("后端通知：治疗已停止。")
            : (paused ? tr("后端通知：束流已暂停。")
                      : tr("后端通知：束流已恢复。"));
        const QString eventCategory = treatmentStopped
            ? QStringLiteral("treatment")
            : QStringLiteral("beam");
        const QString eventAction = treatmentStopped
            ? QStringLiteral("treatment_stopped_received")
            : (paused ? QStringLiteral("beam_paused_received")
                      : QStringLiteral("beam_resumed_received"));
        const QString objectType = treatmentStopped
            ? QStringLiteral("treatment")
            : QStringLiteral("beam");
        auditTreatmentEvent(eventCategory,
                            eventAction,
                            objectType,
                            m_roomId,
                            QStringLiteral("warning"),
                            summary,
                            payload);

        if (treatmentStopped) {
            m_workflowErrorMessage = summary;
            setTreatmentState(TreatmentState::ErrorStopped, cmd);
        } else {
            setTreatmentState(paused ? TreatmentState::Paused : TreatmentState::Treating, cmd);
        }

        QDialog dialog(this);
        dialog.setWindowTitle(title);
        dialog.resize(640, 430);

        auto *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(18, 16, 18, 16);
        layout->setSpacing(10);

        auto *titleLabel = new QLabel(title, &dialog);
        setTreatmentRole(titleLabel, QStringLiteral("treatmentStyle22"));
        layout->addWidget(titleLabel);

        auto *summaryLabel = new QLabel(summary, &dialog);
        summaryLabel->setWordWrap(true);
        setTreatmentRole(summaryLabel, QStringLiteral("treatmentStyle23"));
        layout->addWidget(summaryLabel);

        auto *payloadTitle = new QLabel(tr("Payload 内容"), &dialog);
        setTreatmentRole(payloadTitle, QStringLiteral("treatmentStyle24"));
        layout->addWidget(payloadTitle);

        auto *payloadEdit = new QPlainTextEdit(&dialog);
        payloadEdit->setReadOnly(true);
        payloadEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        payloadEdit->setPlainText(QString::fromUtf8(
            QJsonDocument(payload).toJson(QJsonDocument::Indented)));
        setTreatmentRole(payloadEdit, QStringLiteral("treatmentStyle25"));
        layout->addWidget(payloadEdit, 1);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        layout->addWidget(buttons);

        statusBar()->showMessage(summary, 5000);
        const quint64 notificationRevision = m_workflowRevision;
        const int dialogResult = dialog.exec();
        if (treatmentStopped && dialogResult == QDialog::Accepted
            && notificationRevision == m_workflowRevision && !m_workflowReauthPending) {
            resetTreatmentSessionAfterStop();
        }
        return;
    }

    if (cmd == "TASK_BEAM_ON") {
        auditTreatmentEvent(QStringLiteral("beam"),
                            QStringLiteral("beam_on_received"),
                            QStringLiteral("beam"),
                            m_roomId,
                            QStringLiteral("success"),
                            tr("收到束流开始通知"),
                            payload);
        setTreatmentState(TreatmentState::Treating, cmd);
        return;
    }

    if (cmd == "TASK_BEAM_FINISHED") {
        auditTreatmentEvent(QStringLiteral("beam"),
                            QStringLiteral("beam_finished_received"),
                            QStringLiteral("beam"),
                            m_roomId,
                            QStringLiteral("success"),
                            tr("收到束流结束通知"),
                            payload);
        setTreatmentState(TreatmentState::BeamFinished, cmd);

        // Receipt of a workflow notification is not physical beam-off proof.
        // This modeless dialog allows telemetry to continue updating before
        // the operator explicitly confirms a documented manual command.
        if (findChild<QDialog *>(QStringLiteral("clinicalBeamEndReviewDialog"))) return;
        const quint64 reviewRevision = m_workflowRevision;
        auto *review = new QDialog(this);
        review->setObjectName(QStringLiteral("clinicalBeamEndReviewDialog"));
        review->setAttribute(Qt::WA_DeleteOnClose);
        review->setWindowTitle(tr("人工复核：出束结束"));
        review->resize(520, 230);
        auto *reviewLayout = new QVBoxLayout(review);
        auto *message = new QLabel(tr("收到 TASK_BEAM_FINISHED。\n"
            "请核对实际治疗记录、剂量及物理停束状态。\n"
            "仅在收到新鲜的 beam_on=false 回读后才能确认。"), review);
        message->setWordWrap(true);
        reviewLayout->addWidget(message);
        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, review);
        buttons->button(QDialogButtonBox::Ok)->setText(tr("核对并确认结束"));
        buttons->button(QDialogButtonBox::Cancel)->setText(tr("暂不确认"));
        reviewLayout->addWidget(buttons);
        const auto confirmedStop = [this]() {
            const qint64 maxAgeMs = m_settings->value(
                QStringLiteral("Logout/BeamStatusMaxAgeMs"), 5000).toLongLong();
            return sessionReady() && !m_workflowSyncRequired && !m_resetPending
                && m_webSocketClient && m_webSocketClient->isConnected()
                && m_logoutState.liveBeam.isBool() && !m_logoutState.liveBeam.toBool()
                && m_logoutState.beamAge.isValid()
                && m_logoutState.beamAge.elapsed() < maxAgeMs;
        };
        connect(buttons, &QDialogButtonBox::accepted, review,
            [this, review, reviewRevision, confirmedStop] {
                if (reviewRevision != m_workflowRevision
                    || m_treatmentState != TreatmentState::BeamFinished
                    || !confirmedStop()) {
                    QMessageBox::warning(review, tr("不能确认出束结束"),
                        tr("当前没有新鲜、明确的物理停束回读，或治疗会话已发生变化。\n"
                           "请核对现场及后端，再尝试确认。"));
                    return;
                }
                QJsonObject reply;
                reply.insert(QStringLiteral("cmd"), QStringLiteral("REPLY_BEAM_FINISHED"));
                m_webSocketClient->sendData(reply);
                auditTreatmentEvent(QStringLiteral("beam"),
                    QStringLiteral("reply_beam_finished_sent"),
                    QStringLiteral("beam"), m_roomId, QStringLiteral("attempt"),
                    tr("操作员确认停束，已发送结束确认；等待后端业务结果"), reply);
                review->accept();
            });
        connect(buttons, &QDialogButtonBox::rejected, review, &QDialog::reject);
        review->show();
        review->raise();
        review->activateWindow();
        return;
    }

    if (cmd == "TASK_CHART_GENERATED") {
        auditTreatmentEvent(QStringLiteral("report"),
                            QStringLiteral("chart_generated_received"),
                            QStringLiteral("report"),
                            m_currentPlanPayload.value("plan_id").toString(),
                            QStringLiteral("success"),
                            tr("收到治疗报告生成通知"),
                            payload);
        setTreatmentState(TreatmentState::ReportConfirm, cmd);

        QString reportText;
        if (payload.value("report").isString()) {
            reportText = payload.value("report").toString();
        } else if (payload.value("report").isObject()) {
            reportText = QString::fromUtf8(
                QJsonDocument(payload.value("report").toObject())
                    .toJson(QJsonDocument::Indented));
        } else {
            reportText = tr("治疗报告已生成，患者流程闭环。");
        }

        const quint64 reportRevision = m_workflowRevision;
        if (!showReportConfirmDialog(reportText) || reportRevision != m_workflowRevision
            || !sessionReady() || m_workflowSyncRequired || m_resetPending || m_workflowReauthPending) {
            auditTreatmentEvent(QStringLiteral("report"),
                                QStringLiteral("report_confirm_cancelled"),
                                QStringLiteral("report"),
                                m_currentPlanPayload.value("plan_id").toString(),
                                QStringLiteral("cancelled"),
                                tr("治疗报告确认被取消"));
            statusBar()->showMessage("报告确认已取消", 3000);
            return;
        }
        auditTreatmentEvent(QStringLiteral("report"),
                            QStringLiteral("report_confirmed"),
                            QStringLiteral("report"),
                            m_currentPlanPayload.value("plan_id").toString(),
                            QStringLiteral("success"),
                            tr("治疗报告已确认"));

        if (!ensureConnected()) return;
        QJsonObject flagMessage;
        flagMessage["type"] = "TASK";
        flagMessage["cmd"] = "START_NEXT_TREATMENT";
        m_webSocketClient->sendData(flagMessage);
        auditTreatmentEvent(QStringLiteral("treatment_flow"),
                            QStringLiteral("start_next_treatment_sent"),
                            QStringLiteral("task"),
                            QStringLiteral("START_NEXT_TREATMENT"),
                            QStringLiteral("success"),
                            tr("已请求进入下一位患者流程"),
                            flagMessage);

        setTreatmentState(TreatmentState::Completed, "report confirmed");
        setTreatmentState(TreatmentState::Idle, "start next treatment");

        return;
    }

    auditTreatmentEvent(QStringLiteral("backend"),
                        QStringLiteral("unhandled_task_received"),
                        QStringLiteral("task"),
                        cmd,
                        QStringLiteral("warning"),
                        tr("收到未处理的 TASK 指令"),
                        data);
    qDebug() << "未处理的 TASK 指令:" << cmd << maskSensitivePayload(data);
}

void TreatmentWindow::handleTaskErrorStopped(const QJsonObject &data)
{
    const QJsonObject payload = data.value("payload").toObject();
    if (!telemetryRoomMatches(data, payload, m_roomId)) {
        qWarning() << "Ignoring TASK_ERROR_STOPPED for a different or invalid room";
        return;
    }
    const QString errorMsg = payload.value("message").toString("Unknown error");
    auditTreatmentEvent(QStringLiteral("exception"),
                        QStringLiteral("task_error_stopped"),
                        QStringLiteral("task"),
                        data.value("cmd").toString(QStringLiteral("TASK_ERROR_STOPPED")),
                        QStringLiteral("failed"),
                        errorMsg,
                        payload);
    m_workflowErrorMessage = errorMsg;
    setTreatmentState(TreatmentState::ErrorStopped, errorMsg);

    QMessageBox::critical(this,
                          tr("流程阻断"),
                          tr("流程发生致命异常而阻断:\n%1").arg(errorMsg));
    statusBar()->showMessage("流程发生致命异常", 5000);
}

int TreatmentWindow::treatmentStateToStep(TreatmentState state) const
{
    switch (state) {
    case TreatmentState::Idle:
    case TreatmentState::PlanConfirm:
        return 0;   // 预览

    case TreatmentState::Preparing:
        return 1;   // 准备

    case TreatmentState::ReadyToApplyBeam:
    case TreatmentState::BeamRequested:
    case TreatmentState::BeamReady:
        return 2;   // 就绪

    case TreatmentState::Treating:
    case TreatmentState::Paused:
        return 3;   // 治疗中

    case TreatmentState::BeamFinished:
    case TreatmentState::ReportConfirm:
    case TreatmentState::Completed:
        return 4;   // 记录

    case TreatmentState::ErrorStopped:
        return m_previousTreatmentState == TreatmentState::ErrorStopped
            ? 0 : treatmentStateToStep(m_previousTreatmentState);
    }

    return 0;
}

QString TreatmentWindow::treatmentStateText(TreatmentState state) const
{
    switch (state) {
    case TreatmentState::Idle:
        return tr("等待任务");
    case TreatmentState::PlanConfirm:
        return tr("计划确认");
    case TreatmentState::Preparing:
        return tr("准备中");
    case TreatmentState::ReadyToApplyBeam:
        return tr("可申请束流");
    case TreatmentState::BeamRequested:
        return tr("束流申请中");
    case TreatmentState::BeamReady:
        return tr("束流已就绪");
    case TreatmentState::Treating:
        return tr("治疗中");
    case TreatmentState::Paused:
        return tr("治疗暂停");
    case TreatmentState::BeamFinished:
        return tr("治疗结束");
    case TreatmentState::ReportConfirm:
        return tr("报告确认");
    case TreatmentState::Completed:
        return tr("流程完成");
    case TreatmentState::ErrorStopped:
        return tr("异常中止");
    }

    return tr("未知状态");
}

bool TreatmentWindow::canTransitTo(TreatmentState from, TreatmentState to) const
{
    if (from == to) {
        return true;
    }

    // 异常中止可以从任何状态进入
    if (to == TreatmentState::ErrorStopped) {
        return true;
    }

    // 异常状态下，只允许回到空闲或重新进入计划确认
    if (from == TreatmentState::ErrorStopped) {
        return to == TreatmentState::Idle || to == TreatmentState::PlanConfirm;
    }

    switch (from) {
    case TreatmentState::Idle:
        return to == TreatmentState::PlanConfirm;

    case TreatmentState::PlanConfirm:
        return to == TreatmentState::Preparing;

    case TreatmentState::Preparing:
        return to == TreatmentState::ReadyToApplyBeam ||
               to == TreatmentState::PlanConfirm;

    case TreatmentState::ReadyToApplyBeam:
        return to == TreatmentState::BeamRequested ||
               to == TreatmentState::BeamReady ||
               to == TreatmentState::Treating ||
               to == TreatmentState::PlanConfirm;

    case TreatmentState::BeamRequested:
        return to == TreatmentState::ReadyToApplyBeam ||
               to == TreatmentState::BeamReady ||
               to == TreatmentState::Treating ||
               to == TreatmentState::PlanConfirm;

    case TreatmentState::BeamReady:
        return to == TreatmentState::Treating;

    case TreatmentState::Treating:
        return to == TreatmentState::BeamFinished || to == TreatmentState::Paused;
    case TreatmentState::Paused:
        return to == TreatmentState::Treating || to == TreatmentState::BeamFinished;

    case TreatmentState::BeamFinished:
        return to == TreatmentState::ReportConfirm;

    case TreatmentState::ReportConfirm:
        return to == TreatmentState::Completed ||
               to == TreatmentState::Idle;

    case TreatmentState::Completed:
        return to == TreatmentState::Idle ||
               to == TreatmentState::PlanConfirm;

    case TreatmentState::ErrorStopped:
        return to == TreatmentState::Idle ||
               to == TreatmentState::PlanConfirm;
    }

    return false;
}

bool TreatmentWindow::setTreatmentState(TreatmentState newState, const QString &reason)
{
    if (m_treatmentState == newState) {
        if (newState != TreatmentState::ErrorStopped) m_workflowErrorMessage.clear();
        applyTreatmentStateToUi();
        return true;
    }

    if (!canTransitTo(m_treatmentState, newState)) {
        QJsonObject beforeState;
        beforeState["state"] = treatmentStateText(m_treatmentState);
        QJsonObject afterState;
        afterState["state"] = treatmentStateText(newState);
        auditTreatmentEvent(QStringLiteral("treatment_flow"),
                            QStringLiteral("state_transition_rejected"),
                            QStringLiteral("state"),
                            treatmentStateText(m_treatmentState),
                            QStringLiteral("failed"),
                            reason,
                            QJsonObject(),
                            beforeState,
                            afterState);

        qWarning() << "非法状态跳转:"
                   << treatmentStateText(m_treatmentState)
                   << "->"
                   << treatmentStateText(newState)
                   << "reason =" << reason;

        statusBar()->showMessage(
            tr("非法流程跳转：%1 -> %2")
                .arg(treatmentStateText(m_treatmentState))
                .arg(treatmentStateText(newState)),
            5000);

        return false;
    }

    m_previousTreatmentState = m_treatmentState;
    m_treatmentState = newState;
    if (newState != TreatmentState::ErrorStopped) m_workflowErrorMessage.clear();
    m_backendWorkflowState.clear();
    QJsonObject beforeState;
    beforeState["state"] = treatmentStateText(m_previousTreatmentState);
    QJsonObject afterState;
    afterState["state"] = treatmentStateText(m_treatmentState);
    auditTreatmentEvent(QStringLiteral("treatment_flow"),
                        QStringLiteral("state_transition"),
                        QStringLiteral("state"),
                        treatmentStateText(m_treatmentState),
                        QStringLiteral("success"),
                        reason,
                        QJsonObject(),
                        beforeState,
                        afterState);

    qDebug() << "治疗状态切换:"
             << treatmentStateText(m_previousTreatmentState)
             << "->"
             << treatmentStateText(m_treatmentState)
             << "reason =" << reason;

    applyTreatmentStateToUi();
    return true;
}

void TreatmentWindow::applyTreatmentStateToUi()
{
    updateSessionUi();
    updateStepBar();
    updateTreatmentActionDisplay();

    const bool connected =
        sessionReady() && m_webSocketClient && m_webSocketClient->isConnected()
        && !m_workflowSyncRequired && !m_resetPending && !m_workflowReauthPending;
    if (m_queryWorkflowButton)
        m_queryWorkflowButton->setEnabled(sessionReady() && m_webSocketClient && m_webSocketClient->isConnected()
            && !m_resetPending && !m_workflowReauthPending);
    if (m_resetWorkflowButton)
        m_resetWorkflowButton->setEnabled(sessionReady() && m_webSocketClient && m_webSocketClient->isConnected()
            && !m_resetPending && !m_workflowReauthPending);

    // 计划确认按钮
    if (ui->confirmPlan_pushButton) {
        ui->confirmPlan_pushButton->setEnabled(
            connected && m_treatmentState == TreatmentState::PlanConfirm);
    }

    if (ui->confirmPlan_cancel_pushButton) {
        ui->confirmPlan_cancel_pushButton->hide();
        ui->confirmPlan_cancel_pushButton->setEnabled(false);
    }

    // 申请束流按钮
    if (ui->beamApply_pushButton) {
        ui->beamApply_pushButton->setText(
            m_treatmentState == TreatmentState::BeamRequested ? tr("申请中") : tr("申请束流"));
        ui->beamApply_pushButton->setEnabled(
            connected &&
            m_treatmentState == TreatmentState::ReadyToApplyBeam);
    }

    if (ui->beamApply_cancel_pushButton) {
        ui->beamApply_cancel_pushButton->hide();
        ui->beamApply_cancel_pushButton->setEnabled(false);
    }


    // 异常状态下锁定高风险按钮
    if (m_treatmentState == TreatmentState::ErrorStopped) {
        if (ui->confirmPlan_pushButton) ui->confirmPlan_pushButton->setEnabled(false);
        if (ui->confirmPlan_cancel_pushButton) ui->confirmPlan_cancel_pushButton->setEnabled(false);
        if (ui->beamApply_pushButton) ui->beamApply_pushButton->setEnabled(false);
        if (ui->beamApply_cancel_pushButton) ui->beamApply_cancel_pushButton->setEnabled(false);
    }

    statusBar()->showMessage(
        tr("当前治疗状态：%1").arg(treatmentStateText(m_treatmentState)),
        3000);
}

void TreatmentWindow::resetTreatmentSessionAfterStop()
{
    auditTreatmentEvent(QStringLiteral("treatment"),
                        QStringLiteral("treatment_stopped_acknowledged"),
                        QStringLiteral("treatment"),
                        m_currentPlanPayload.value(QStringLiteral("plan_id")).toString(),
                        QStringLiteral("success"),
                        tr("用户确认治疗停止通知，清空本次治疗数据并恢复预览状态"));

    m_todayScheduleEntries = QJsonArray();
    m_currentPlanPayload = QJsonObject();
    m_latestTelemetryPayload = QJsonObject();
    m_lastDoseTelemetrySnapshot.clear();
    m_waitingOisPatientCallLogged = false;
    AuditLogger::instance().clearTreatmentContext();

    updateTodaySchedule(QJsonArray());

    ui->patient_id->setText(tr("ID：-"));
    ui->patient_name->setText(tr("姓名：-"));
    ui->labelPlanIdValue->setText(QStringLiteral("-"));
    ui->labelPlanFractionIdValue->setText(QStringLiteral("-"));
    ui->labelPlanSiteTargetValue->setText(QStringLiteral("-"));
    ui->labelPlanIsocenterValue->setText(QStringLiteral("-"));
    ui->labelPlanQaReleaseValue->setText(QStringLiteral("-"));

    ui->spotMapWidget->clearPlan();
    ui->spotMapWidget->setTelemetry(QJsonObject());

    updateDoseInfoPanel();
    setLabelTextIfChanged(ui->labelTopPrescriptionDose, QStringLiteral("0.00 Gy"));
    setLabelTextIfChanged(ui->labelTopPrescriptionDetail,
                          tr("本次：0.00 Gy / fraction"));
    setLabelTextIfChanged(ui->labelPrescriptionDose, QStringLiteral("-"));
    setLabelTextIfChanged(ui->labelFractionDose, QStringLiteral("- / -"));
    setLabelTextIfChanged(ui->labelCurrentFractionDose, tr("0 个"));
    setLabelTextIfChanged(ui->labelCompletedDose, tr("能层：0，Spot：0"));
    setLabelTextIfChanged(ui->labelBeamMuDose, tr("等待当前能层"));
    setLabelTextIfChanged(ui->labelDoseDeviation,
                          tr("状态：待治疗反馈\nGy 剂量回读：暂无累计剂量回读"));
    setLabelTextIfChanged(ui->labelBeamDoseInfo, QStringLiteral("-"));
    setProgressBarIfChanged(ui->progressBarBeamMeterset, 0, 1000, 0, QStringLiteral("0%"));
    setProgressBarIfChanged(ui->progressBarLayerProgress, 0, 100, 0, QStringLiteral("0%"));
    setLabelTextIfChanged(ui->labelLayerProgressValue, QStringLiteral("0 / 0"));
    ui->progressBarDoseDelivery->setProperty("clinicalHasMuProgress", false);
    setProgressBarIfChanged(ui->progressBarDoseDelivery, 0, 1000, 0, QStringLiteral("0%"));
    ui->progressBarDoseDelivery->update();
    setLabelTextIfChanged(ui->labelDoseDeliveryProgressValue, QStringLiteral("0.00 / 0.00 Gy"));

    for (QLabel *label : {ui->label_posa, ui->label_posb, ui->label_posc,
                          ui->label_posx, ui->label_posy, ui->label_posz}) {
        setLabelTextIfChanged(label, QStringLiteral("0.00"));
    }
    for (QLineEdit *edit : {ui->lineEdit_a, ui->lineEdit_b, ui->lineEdit_c,
                            ui->lineEdit_x, ui->lineEdit_y, ui->lineEdit_z}) {
        edit->setText(QStringLiteral("0.00"));
    }
    for (const char *key : {"bed.a1", "bed.a2", "bed.a3", "bed.a4", "bed.a5", "bed.a6"}) {
        DataProvider::instance().updateValue(QLatin1String(key), 0.0);
    }

    m_activeBeamRoomId.clear();
    updateBeamRoomOverview();

    setTreatmentState(TreatmentState::Idle,
                      QStringLiteral("TASK_TREATMENT_STOPPED acknowledged"));
    statusBar()->showMessage(tr("治疗已停止，界面数据已清零并恢复到预览"), 5000);
}

bool TreatmentWindow::ensureConnected() const
{
    if (!sessionReady()) {
        QMessageBox::warning(const_cast<TreatmentWindow *>(this), tr("操作已锁定"),
            !m_startupState.authenticated ? tr("请先通过右上角登录按钮登录。") : tr("请等待后台初始化完成。"));
        return false;
    }
    if (m_workflowSyncRequired || m_resetPending || m_workflowReauthPending) {
        QMessageBox::warning(const_cast<TreatmentWindow *>(this), tr("操作暂不可用"),
            tr("现场待同步或正在重置，请等待后端确认。"));
        return false;
    }
    if (!m_webSocketClient || !m_webSocketClient->isConnected()) {
        auditTreatmentEvent(QStringLiteral("network"),
                            QStringLiteral("operation_blocked_not_connected"),
                            QStringLiteral("gateway"),
                            m_wsMiddlewareUrl,
                            QStringLiteral("failed"),
                            tr("未连接到服务器，无法发送指令"));
        QMessageBox::warning(
            const_cast<TreatmentWindow *>(this),
            tr("连接错误"),
            tr("未连接到服务器，无法发送指令。"));
        return false;
    }

    return true;
}

bool TreatmentWindow::ensureStateAllowed(const QList<TreatmentState> &allowedStates,
                                 const QString &operationName) const
{
    if (sessionReady() && !m_workflowSyncRequired && !m_resetPending && !m_workflowReauthPending
        && allowedStates.contains(m_treatmentState)) {
        return true;
    }

    QJsonObject payload;
    payload["operation"] = operationName;
    payload["current_state"] = treatmentStateText(m_treatmentState);
    auditTreatmentEvent(QStringLiteral("permission"),
                        QStringLiteral("operation_blocked_by_state"),
                        QStringLiteral("operation"),
                        operationName,
                        QStringLiteral("failed"),
                        tr("当前治疗状态不允许执行该操作"),
                        payload);

    QMessageBox::warning(
        const_cast<TreatmentWindow *>(this),
        tr("操作禁止"),
        tr("当前状态为【%1】，不允许执行【%2】。")
            .arg(treatmentStateText(m_treatmentState))
            .arg(operationName));

    return false;
}

void TreatmentWindow::sendTaskCommand(const QString &cmd)
{
    if (!ensureConnected()) {
        return;
    }

    QJsonObject msg;
    msg["type"] = "TASK";
    msg["cmd"] = cmd;
    msg["username"] = m_currentUsername;
    msg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    m_webSocketClient->sendData(msg);
    auditTreatmentEvent(QStringLiteral("command"),
                        QStringLiteral("task_command_sent"),
                        QStringLiteral("task"),
                        cmd,
                        QStringLiteral("success"),
                        tr("已发送 TASK 指令"),
                        msg);

    qDebug() << "已发送 TASK 指令:" << msg;
    statusBar()->showMessage(tr("已发送指令：%1").arg(cmd), 2000);
}

bool TreatmentWindow::validateCredentialConfirmation(const QString &confirmedUsername,
                                                     const QString &confirmedPassword)
{
    if (m_loginPassword.isEmpty()) {
        auditTreatmentEvent(QStringLiteral("credential"),
                            QStringLiteral("credential_confirm_failed"),
                            QStringLiteral("user"),
                            m_currentUsername,
                            QStringLiteral("failed"),
                            tr("当前登录密码不可用，二次确认失败"));
        QMessageBox::warning(this,
                             tr("确认失败"),
                             tr("当前登录密码不可用，请重新登录后再操作。"));
        return false;
    }

    if (confirmedUsername.trimmed() != m_currentUsername.trimmed()
        || confirmedPassword != m_loginPassword) {
        auditTreatmentEvent(QStringLiteral("credential"),
                            QStringLiteral("credential_confirm_failed"),
                            QStringLiteral("user"),
                            m_currentUsername,
                            QStringLiteral("failed"),
                            tr("二次确认账号或密码与当前登录用户不一致"));
        QMessageBox::warning(this,
                             tr("确认失败"),
                             tr("用户名或密码与当前登录用户不一致。"));
        return false;
    }

    return true;
}

bool TreatmentWindow::showCredentialConfirmDialog(const QString &windowTitle,
                                                  const QString &prompt,
                                                  const QString &okText,
                                                  const QVector<QPair<QString, QString>> &summaryRows,
                                                  QString *confirmedUsername,
                                                  QString *confirmedPassword)
{
    if (!confirmedUsername || !confirmedPassword) {
        return false;
    }

    if (loginDialog || m_loginTransitionPending || m_workflowReauthPending) return false;
    if (m_credentialDialog) {
        activateDialog(m_credentialDialog);
        return false;
    }
    const quint64 confirmationRevision = m_workflowRevision;
    QDialog dialog(this);
    m_credentialDialog = &dialog;
    const auto confirmationCurrent = [this, confirmationRevision] {
        return confirmationRevision == m_workflowRevision && !m_workflowSyncRequired
            && !m_resetPending && !m_workflowReauthPending && !m_loginTransitionPending;
    };
    dialog.setWindowTitle(windowTitle);
    dialog.resize(640, summaryRows.size() > 4 ? 640 : 480);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *titleLabel = new QLabel(prompt, &dialog);
    titleLabel->setWordWrap(true);
    setTreatmentRole(titleLabel, QStringLiteral("treatmentStyle26"));
    layout->addWidget(titleLabel);

    if (!summaryRows.isEmpty()) {
        auto *summaryBox = new QGroupBox(tr("确认摘要"), &dialog);
        auto *summaryGrid = new QGridLayout(summaryBox);
        summaryGrid->setContentsMargins(12, 14, 12, 12);
        summaryGrid->setHorizontalSpacing(12);
        summaryGrid->setVerticalSpacing(8);
        summaryGrid->setAlignment(Qt::AlignTop);
        summaryGrid->setColumnStretch(1, 1);

        for (int row = 0; row < summaryRows.size(); ++row) {
            auto *nameLabel = new QLabel(summaryRows.at(row).first, summaryBox);
            auto *valueLabel = new ClinicalReadbackView(summaryBox);
            valueLabel->setReadback(summaryRows.at(row).second);
            setTreatmentRole(nameLabel, QStringLiteral("treatmentStyle27"));
            nameLabel->setWordWrap(true);
            nameLabel->setMinimumWidth(90);
            nameLabel->setMaximumWidth(160);
            setTreatmentRole(valueLabel, QStringLiteral("treatmentStyle28"));
            summaryGrid->addWidget(nameLabel, row, 0, Qt::AlignTop);
            summaryGrid->addWidget(valueLabel, row, 1, Qt::AlignTop);
        }

        auto *summaryScroll = new QScrollArea(&dialog);
        summaryScroll->setWidgetResizable(true);
        summaryScroll->setFrameShape(QFrame::NoFrame);
        summaryScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        summaryScroll->setMinimumHeight(120);
        summaryScroll->setMaximumHeight(280);
        summaryScroll->setWidget(summaryBox);
        layout->addWidget(summaryScroll, 1);
    }

    auto *authBox = new QGroupBox(tr("二次确认"), &dialog);
    auto *authGrid = new QGridLayout(authBox);
    authGrid->setContentsMargins(12, 14, 12, 12);
    authGrid->setHorizontalSpacing(10);
    authGrid->setVerticalSpacing(10);

    auto *usernameEdit = new QLineEdit(&dialog);
    usernameEdit->setText(m_currentUsername);
    usernameEdit->setPlaceholderText(tr("请输入用户名"));

    auto *passwordEdit = new QLineEdit(&dialog);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText(tr("请输入密码"));

    authGrid->addWidget(new QLabel(tr("用户名"), &dialog), 0, 0);
    authGrid->addWidget(usernameEdit, 0, 1);
    authGrid->addWidget(new QLabel(tr("密码"), &dialog), 1, 0);
    authGrid->addWidget(passwordEdit, 1, 1);
    authGrid->setColumnStretch(1, 1);
    layout->addWidget(authBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QPushButton *okButton = buttons->button(QDialogButtonBox::Ok);
    QPushButton *cancelButton = buttons->button(QDialogButtonBox::Cancel);
    okButton->setText(okText);
    okButton->setMinimumHeight(42);
    okButton->setMinimumWidth(96);
    setTreatmentRole(okButton, QStringLiteral("primaryActionButtonStyle"));
    cancelButton->setText(tr("取消"));
    cancelButton->setMinimumHeight(42);
    cancelButton->setMinimumWidth(72);
    setTreatmentRole(cancelButton, QStringLiteral("secondaryActionButtonStyle"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    while (confirmationCurrent() && dialog.exec() == QDialog::Accepted) {
        if (!confirmationCurrent()) return false;
        const QString username = usernameEdit->text().trimmed();
        const QString password = passwordEdit->text();
        if (username.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(&dialog, tr("确认失败"), tr("用户名和密码不能为空。"));
            continue;
        }

        if (!validateCredentialConfirmation(username, password)) {
            passwordEdit->clear();
            passwordEdit->setFocus();
            continue;
        }

        if (!confirmationCurrent()) return false;
        *confirmedUsername = username;
        *confirmedPassword = password;
        return true;
    }

    return false;
}

bool TreatmentWindow::sendCredentialTaskCommand(const QString &cmd,
                                                const QString &confirmedUsername,
                                                const QString &confirmedPassword,
                                                const QString &successMessage)
{
    if (!ensureConnected()) {
        return false;
    }

    QJsonObject msg;
    msg["type"] = "TASK";
    msg["cmd"] = cmd;
    msg["username"] = confirmedUsername;
    msg["password"] = confirmedPassword;
    msg["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    m_webSocketClient->sendData(msg);

    const QJsonObject auditPayload = maskSensitivePayload(msg);
    auditTreatmentEvent(QStringLiteral("command"),
                        QStringLiteral("task_command_sent"),
                        QStringLiteral("task"),
                        cmd,
                        QStringLiteral("attempt"),
                        successMessage,
                        auditPayload);

    qDebug() << successMessage << auditPayload;
    statusBar()->showMessage(successMessage, 2000);
    return true;
}

bool TreatmentWindow::showPlanConfirmationDialog(QString *confirmedUsername,
                                                 QString *confirmedPassword)
{
    if (!confirmedUsername || !confirmedPassword) {
        return false;
    }

    if (m_currentPlanPayload.isEmpty()) {
        QMessageBox::information(this, tr("计划确认"), tr("当前还没有可确认的治疗计划。"));
        return false;
    }

    const PlanDisplayInfo plan = parsePlanDisplayInfo(m_currentPlanPayload);

    QVector<QPair<QString, QString>> summaryRows;
    const auto appendAvailableRow = [&summaryRows](const QString &name, const QString &value) {
        if (!value.trimmed().isEmpty() && value != QStringLiteral("-")) {
            summaryRows.append({name, value});
        }
    };
    appendAvailableRow(tr("患者"), tr("%1 / %2").arg(plan.patientName, plan.patientId));
    appendAvailableRow(tr("计划ID"), plan.planId);
    appendAvailableRow(tr("分次ID"), plan.fractionId);
    appendAvailableRow(tr("靶区"), plan.targetArea);
    appendAvailableRow(tr("等中心点坐标"), plan.isocenterText);
    appendAvailableRow(tr("处方剂量"), plan.prescriptionText);
    appendAvailableRow(tr("本次剂量"),
                       plan.fractionDose >= 0.0
                           ? tr("%1 Gy / fraction").arg(plan.fractionDose, 0, 'f', 2)
                           : QStringLiteral("-"));
    if (plan.qaStatus != QStringLiteral("-") || plan.releaseStatus != QStringLiteral("-")) {
        appendAvailableRow(tr("QA / 放行"), tr("%1 / %2").arg(plan.qaStatus, plan.releaseStatus));
    }

    return showCredentialConfirmDialog(tr("治疗计划确认"),
                                       tr("请核对治疗计划关键信息"),
                                       tr("确认并提交"),
                                       summaryRows,
                                       confirmedUsername,
                                       confirmedPassword);
}

bool TreatmentWindow::sendPlanConfirmedCommand(const QString &confirmedUsername,
                                               const QString &confirmedPassword)
{
    return sendCredentialTaskCommand(QStringLiteral("REPLY_PLAN_CONFIRMED"),
                                     confirmedUsername,
                                     confirmedPassword,
                                     tr("已发送治疗计划确认"));
}

bool TreatmentWindow::showBeamApplyConfirmationDialog(QString *confirmedUsername,
                                                      QString *confirmedPassword)
{
    if (!confirmedUsername || !confirmedPassword) {
        return false;
    }

    const PlanDisplayInfo plan = parsePlanDisplayInfo(m_currentPlanPayload);
    const QString beamRoomText = m_activeBeamRoomId.isEmpty() ? tr("未分配") : m_activeBeamRoomId;
    const QVector<QPair<QString, QString>> summaryRows = {
        {tr("患者"), tr("%1 / %2").arg(plan.patientName, plan.patientId)},
        {tr("治疗室"), m_roomId},
        {tr("当前束流"), beamRoomText},
        {tr("流程状态"), treatmentStateText(m_treatmentState)}
    };

    return showCredentialConfirmDialog(tr("申请束流确认"),
                                       tr("请确认是否申请束流"),
                                       tr("确认申请"),
                                       summaryRows,
                                       confirmedUsername,
                                       confirmedPassword);
}

bool TreatmentWindow::sendBeamApplyCommand(const QString &confirmedUsername,
                                           const QString &confirmedPassword)
{
    return sendCredentialTaskCommand(QStringLiteral("APPLY_BEAM"),
                                     confirmedUsername,
                                     confirmedPassword,
                                     tr("已发送申请束流"));
}

bool TreatmentWindow::showReportConfirmDialog(const QString &reportText)
{
    QDialog reportDialog(this);
    reportDialog.setWindowTitle(tr("治疗报告确认"));
    reportDialog.resize(700, 500);

    auto *layout = new QVBoxLayout(&reportDialog);

    auto *label = new QLabel(tr("请查看治疗报告内容，确认后继续："), &reportDialog);

    auto *textEdit = new QTextEdit(&reportDialog);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(reportText);

    auto *buttonLayout = new QHBoxLayout;
    auto *confirmButton = new QPushButton(tr("确认"), &reportDialog);
    auto *cancelButton = new QPushButton(tr("取消"), &reportDialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(confirmButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addWidget(textEdit);
    layout->addLayout(buttonLayout);

    connect(confirmButton, &QPushButton::clicked, &reportDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &reportDialog, &QDialog::reject);

    return reportDialog.exec() == QDialog::Accepted;
}

void TreatmentWindow::showCurrentPlanDetail()
{
    if (m_currentPlanPayload.isEmpty()) {
        auditTreatmentEvent(QStringLiteral("treatment_plan"),
                            QStringLiteral("view_plan_detail_failed"),
                            QStringLiteral("plan"),
                            QString(),
                            QStringLiteral("failed"),
                            tr("当前没有可查看的治疗计划"));
        QMessageBox::information(this, tr("计划详情"), tr("当前还没有可查看的治疗计划。"));
        return;
    }

    const QJsonObject payload = m_currentPlanPayload;
    const PlanDisplayInfo plan = parsePlanDisplayInfo(payload);
    const QJsonObject summary = plan.summary;

    const QString patientId = plan.patientId;
    const QString patientName = plan.patientName;
    const QString planId = plan.planId;
    const QString fractionId = plan.fractionId;
    const QString targetArea = plan.targetArea;
    const QString isocenterText = plan.isocenterText;
    const QString planVersion = payload.value(QStringLiteral("version_no")).toString(QStringLiteral("-"));
    const QString doctorApproval = plan.approvalStatus;
    const QString machine = summary.value(QStringLiteral("machine")).toString(QStringLiteral("-"));
    const QString scanMode = summary.value(QStringLiteral("scan_mode")).toString(QStringLiteral("-"));
    const QString prescriptionText = plan.prescriptionText;
    const double fractionDose = plan.fractionDose;
    const double completedDose = plan.completedDose;
    const QString qaStatus = plan.qaStatus;
    const QString releaseStatus = plan.releaseStatus;
    const QString releaseReason = plan.releaseReason;
    const QJsonArray beams = plan.beams;
    const double plannedMu = plan.plannedMu;
    const int beamCount = summary.value(QStringLiteral("beam_count")).toInt(-1);
    int totalLayers = 0;
    int totalSpots = 0;
    QStringList beamLines;
    for (const QJsonValue &beamValue : beams) {
        const QJsonObject beam = beamValue.toObject();
        const int beamNumber = readIntValue(beam, {"beam_number"}, beamLines.size() + 1);
        const double meterset = readDoubleValue(beam, {"beam_meterset"});
        const int layers = readIntValue(beam, {"energy_layers"});
        const int spots = readIntValue(beam, {"total_scan_spots"});
        QStringList values;
        if (meterset >= 0.0) {
            values << tr("meterset %1").arg(meterset, 0, 'f', 1);
        }
        if (layers >= 0) {
            totalLayers += layers;
            values << tr("能层 %1").arg(layers);
        }
        if (spots >= 0) {
            totalSpots += spots;
            values << tr("Spot %1").arg(spots);
        }
        if (!values.isEmpty()) {
            beamLines << tr("Beam %1：%2").arg(beamNumber).arg(values.join(QStringLiteral("，")));
        }
    }
    if (beamLines.size() > 4) {
        beamLines = beamLines.mid(0, 4);
        beamLines << tr("...");
    }

    auditTreatmentEvent(QStringLiteral("treatment_plan"),
                        QStringLiteral("view_plan_detail"),
                        QStringLiteral("plan"),
                        planId,
                        QStringLiteral("success"),
                        tr("查看治疗计划摘要"),
                        payload);

    QDialog dialog(this);
    dialog.setWindowTitle(tr("治疗计划详情"));
    dialog.resize(760, 620);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(tr("治疗计划摘要"), &dialog);
    setTreatmentRole(titleLabel, QStringLiteral("treatmentStyle29"));
    layout->addWidget(titleLabel);

    auto *scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *contentWidget = new QWidget(scrollArea);
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(10);

    auto addSection = [&dialog, contentLayout](const QString &title,
                                               const QVector<QPair<QString, QString>> &rows) {
        auto *box = new QGroupBox(title, &dialog);
        setTreatmentRole(box, QStringLiteral("treatmentStyle30"));
        auto *grid = new QGridLayout(box);
        grid->setHorizontalSpacing(14);
        grid->setVerticalSpacing(8);
        grid->setColumnStretch(1, 1);
        grid->setColumnStretch(3, 1);

        for (int index = 0; index < rows.size(); ++index) {
            const int row = index / 2;
            const int col = (index % 2) * 2;
            auto *nameLabel = new QLabel(rows.at(index).first, box);
            auto *valueLabel = new QLabel(rows.at(index).second, box);
            setTreatmentRole(nameLabel, QStringLiteral("treatmentStyle31"));
            valueLabel->setWordWrap(true);
            setTreatmentRole(valueLabel, QStringLiteral("treatmentStyle32"));
            grid->addWidget(nameLabel, row, col);
            grid->addWidget(valueLabel, row, col + 1);
        }

        contentLayout->addWidget(box);
    };

    addSection(tr("患者信息"), {
        {tr("姓名"), patientName},
        {tr("患者ID"), patientId}
    });
    addSection(tr("计划信息"), {
        {tr("计划ID"), planId},
        {tr("分次ID"), fractionId},
        {tr("靶区"), targetArea},
        {tr("等中心点坐标"), isocenterText},
        {tr("计划版本"), planVersion},
        {tr("计划审批状态"), doctorApproval}
    });
    addSection(tr("剂量信息"), {
        {tr("处方剂量"), prescriptionText},
        {tr("总分次数"), plan.totalFractions > 0 ? tr("%1 次").arg(plan.totalFractions) : QStringLiteral("-")},
        {tr("本次分次剂量"), fractionDose >= 0.0 ? tr("%1 Gy / fraction").arg(fractionDose, 0, 'f', 2) : QStringLiteral("-")},
        {tr("分次剂量来源"), plan.fractionDoseSource},
        {tr("已完成累计剂量"), completedDose >= 0.0 ? tr("%1 Gy").arg(completedDose, 0, 'f', 1) : QStringLiteral("-")}
    });
    addSection(tr("QA / 放行"), {
        {tr("QA状态"), qaStatus},
        {tr("治疗放行"), releaseStatus},
        {tr("放行原因"), releaseReason}
    });
    addSection(tr("束流信息"), {
        {tr("治疗室 / 设备"), machine},
        {tr("扫描模式"), scanMode},
        {tr("Beam数量"), beamCount >= 0 ? QString::number(beamCount) : QStringLiteral("-")},
        {tr("总 MU / meterset"), plannedMu >= 0.0 ? QString::number(plannedMu, 'f', 1) : QStringLiteral("-")},
        {tr("能层数"), totalLayers > 0 ? QString::number(totalLayers) : QStringLiteral("-")},
        {tr("Spot数"), totalSpots > 0 ? QString::number(totalSpots) : QStringLiteral("-")},
        {tr("Beam摘要"), beamLines.isEmpty() ? QStringLiteral("-") : beamLines.join(QLatin1Char('\n'))}
    });

    contentLayout->addStretch();
    scrollArea->setWidget(contentWidget);
    layout->addWidget(scrollArea, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText(tr("关闭"));
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *buttonLayout = new QHBoxLayout;
    auto *rawButton = new QPushButton(tr("查看原始 Plan"), &dialog);
    rawButton->setMinimumSize(128, 34);
    setTreatmentRole(rawButton, QStringLiteral("treatmentStyle33"));
    connect(rawButton, &QPushButton::clicked, this, [this, payload, planId]() {
        auditTreatmentEvent(QStringLiteral("treatment_plan"),
                            QStringLiteral("view_raw_plan"),
                            QStringLiteral("plan"),
                            planId,
                            QStringLiteral("success"),
                            tr("查看原始 Plan JSON"),
                            payload);

        QDialog rawDialog(this);
        rawDialog.setWindowTitle(tr("原始 Plan 数据"));
        rawDialog.resize(900, 680);
        auto *rawLayout = new QVBoxLayout(&rawDialog);
        rawLayout->setContentsMargins(14, 14, 14, 14);
        rawLayout->setSpacing(10);

        auto *detailEdit = new QPlainTextEdit(&rawDialog);
        detailEdit->setReadOnly(true);
        detailEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        detailEdit->setPlainText(QString::fromUtf8(
            QJsonDocument(payload).toJson(QJsonDocument::Indented)));
        setTreatmentRole(detailEdit, QStringLiteral("treatmentStyle34"));
        rawLayout->addWidget(detailEdit, 1);

        auto *rawButtons = new QDialogButtonBox(QDialogButtonBox::Close, &rawDialog);
        rawButtons->button(QDialogButtonBox::Close)->setText(tr("关闭"));
        connect(rawButtons, &QDialogButtonBox::rejected, &rawDialog, &QDialog::reject);
        rawLayout->addWidget(rawButtons);
        rawDialog.exec();
    });

    buttonLayout->addWidget(rawButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(buttons);
    layout->addLayout(buttonLayout);

    dialog.exec();
}

void TreatmentWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    updateStepBar();
}

void TreatmentWindow::on_bed_pushButton_clicked()
{
    if (!ensureConnected()) return;
    QMessageBox::StandardButton reply =
        QMessageBox::question(this,
                              tr("确认操作"),
                              tr("确定要发送床数据吗？此操作需要再次确认。"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        auditTreatmentEvent(QStringLiteral("couch"),
                            QStringLiteral("send_couch_position_cancelled"),
                            QStringLiteral("couch"),
                            m_roomId,
                            QStringLiteral("cancelled"),
                            tr("治疗师取消发送床位参数"));
        return;
    }

    if (!ensureConnected()) return;

    if (!m_webSocketClient || !m_webSocketClient->isConnected()) {
        auditTreatmentEvent(QStringLiteral("couch"),
                            QStringLiteral("send_couch_position_failed"),
                            QStringLiteral("couch"),
                            m_roomId,
                            QStringLiteral("failed"),
                            tr("未连接到服务器，无法发送床位参数"));
        QMessageBox::warning(this, tr("错误"),
                             tr("未连接到服务器，无法发送数据。"));
        return;
    }

    bool ok1, ok2, ok3, ok4, ok5, ok6;
    const double a = ui->lineEdit_a->text().toDouble(&ok1);
    const double b = ui->lineEdit_b->text().toDouble(&ok2);
    const double c = ui->lineEdit_c->text().toDouble(&ok3);
    const double x = ui->lineEdit_x->text().toDouble(&ok4);
    const double y = ui->lineEdit_y->text().toDouble(&ok5);
    const double z = ui->lineEdit_z->text().toDouble(&ok6);


    if (!(ok1 && ok2 && ok3 && ok4 && ok5 && ok6)) {
        auditTreatmentEvent(QStringLiteral("couch"),
                            QStringLiteral("send_couch_position_failed"),
                            QStringLiteral("couch"),
                            m_roomId,
                            QStringLiteral("failed"),
                            tr("床位参数输入非法"));
        QMessageBox::warning(this, tr("输入错误"), tr("请输入合法的床位参数。"));
        return;
    }

    QJsonObject data;
    data["type"] = "bed_data";
    data["username"] = m_currentUsername;
    data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    data["value1"] = a;
    data["value2"] = b;
    data["value3"] = c;
    data["value4"] = x;
    data["value5"] = y;
    data["value6"] = z;

    m_webSocketClient->sendData(data);
    auditTreatmentEvent(QStringLiteral("couch"),
                        QStringLiteral("send_couch_position"),
                        QStringLiteral("couch"),
                        m_roomId,
                        QStringLiteral("success"),
                        tr("治疗床位参数已发送"),
                        data);

    statusBar()->showMessage(tr("床数据已发送"), 2000);
    qDebug() << "床数据已发送:" << data;
}

void TreatmentWindow::on_todays_pushButton_clicked()
{
    if (!sessionReady()) return;
    QJsonObject payload;
    payload["schedule_count"] = m_todayScheduleEntries.size();
    auditTreatmentEvent(QStringLiteral("schedule"),
                        QStringLiteral("open_today_schedule"),
                        QStringLiteral("schedule"),
                        m_roomId,
                        QStringLiteral("success"),
                        tr("打开今日排程"),
                        payload);

    if (!m_todayScheduleDialog) {
        m_todayScheduleDialog = new TodaySchedule(this);
        m_todayScheduleDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_todayScheduleDialog, &QObject::destroyed, this, [this]() {
            m_todayScheduleDialog = nullptr;
        });
    }

    qDebug() << "打开今日排程，缓存数量:" << m_todayScheduleEntries.size();
    m_todayScheduleDialog->setHeaderInfo(m_currentUsername, m_roomId);

    m_todayScheduleDialog->updateScheduleEntries(m_todayScheduleEntries);

    m_todayScheduleDialog->show();
    m_todayScheduleDialog->raise();
    m_todayScheduleDialog->activateWindow();
}

void TreatmentWindow::on_theme_pushButton_clicked()
{
    if (!sessionReady()) return;
    const Theme current = ThemeManager::instance().currentTheme();
    const Theme next = (current == Theme::Light) ? Theme::Dark : Theme::Light;
    ThemeManager::instance().setTheme(next);
}

void TreatmentWindow::on_logout_pushButton_clicked()
{
    if (m_closing) return;
    if (!m_startupState.authenticated) {
        showLoginDialog();
        return;
    }
    if (m_loginTransitionPending || loginDialog) {
        activateDialog(loginDialog);
        return;
    }
    if (m_logoutConfirming) return;
    QString reason = logoutBlockReason();
    if (!reason.isEmpty()) {
        m_closeAfterLogout = false;
        QMessageBox::warning(this, tr("暂不能注销"), reason);
        return;
    }
    const quint64 revision = m_workflowRevision;
    m_logoutConfirming = true;
    applyTreatmentStateToUi();
    const auto answer = QMessageBox::question(this, tr("确认注销"),
        tr("向中间层请求注销？收到中间层完成注销并确认 OIS 已记录时间后，才清理本机会话。"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    m_logoutConfirming = false;
    applyTreatmentStateToUi();
    reason = logoutBlockReason();
    if (answer != QMessageBox::Yes || revision != m_workflowRevision
        || !m_startupState.authenticated || !reason.isEmpty()) {
        m_closeAfterLogout = false;
        if (answer == QMessageBox::Yes && !reason.isEmpty())
            QMessageBox::warning(this, tr("注销已取消"), reason);
        return;
    }
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QJsonObject request = m_logoutState.begin(requestId, m_currentUsername, m_roomId);
    const int timeout = qBound(1000, m_settings->value(QStringLiteral("Logout/TimeoutMs"), 15000).toInt(), 120000);
    m_logoutTimer->start(timeout);
    applyTreatmentStateToUi();
    statusBar()->showMessage(tr("注销中，等待中间层及 OIS 确认；本机会话尚未清理"));
    auditTreatmentEvent("auth", "logout_requested", "request", requestId, "attempt");
    m_webSocketClient->sendData(request);
}

void TreatmentWindow::completeLogout(bool backendConfirmed)
{
    if (m_closing || (!backendConfirmed && !m_workflowReauthPending)) return;
    const bool closeAfterLogout = backendConfirmed && m_closeAfterLogout;
    m_closeAfterLogout = false;
    m_logoutTimer->stop();
    m_loginTransitionPending = true;
    ++m_workflowRevision;
    m_workflowConfirmationPending = false;
    m_planConfirmationPending = false;
    m_lastWorkflowSnapshot = QJsonObject();
    m_workflowSyncRequired = true;
    m_workflowReauthPending = false;
    rejectWorkflowDialogs(this);
    m_resetPending = false;
    m_backendWorkflowState.clear();
    if (m_workflowTimer) m_workflowTimer->stop();
    m_treatmentState = TreatmentState::Idle;
    m_previousTreatmentState = TreatmentState::Idle;
    m_currentDeviceActionText = tr("等待重新登录并同步现场");
    clearGrpcCallDisplay();
    m_currentDeviceIssues.clear();
    m_activeBeamRoomId.clear();
    if (m_bedView) {
        m_bedView->show();
        m_bedView->setToolTip(tr("等待位置反馈；三维模型仅供示意，不代表已确认的实机位置。"));
    }
    for (QLabel *label : {ui->label_posa, ui->label_posb, ui->label_posc,
                          ui->label_posx, ui->label_posy, ui->label_posz})
        label->setText(QStringLiteral("-"));
    for (QLineEdit *edit : {ui->lineEdit_a, ui->lineEdit_b, ui->lineEdit_c,
                            ui->lineEdit_x, ui->lineEdit_y, ui->lineEdit_z}) edit->clear();
    if (m_todayScheduleDialog) m_todayScheduleDialog->close();
    ui->logout_pushButton->setEnabled(false);

    QJsonObject logoutDetails;
    if (backendConfirmed) {
        logoutDetails.insert(QStringLiteral("request_id"), m_logoutState.requestId);
        logoutDetails.insert(QStringLiteral("ois_recorded"), true);
    }
    auditTreatmentEvent(QStringLiteral("auth"),
                        backendConfirmed ? QStringLiteral("logout") : QStringLiteral("session_revoked_by_backend"),
                        QStringLiteral("user"),
                        m_currentUsername,
                        QStringLiteral("success"),
                        backendConfirmed ? tr("中间层确认注销且 OIS 已记录注销时间")
                                         : tr("后台重置已撤销本机会话；不代表 OIS 注销记录已完成"), logoutDetails);
    m_logoutState = LogoutState();

    if (m_webSocketClient) {
        m_webSocketClient->setAutoReconnect(false);
        disconnect(m_webSocketClient, nullptr, this, nullptr);
        m_webSocketClient->disconnectFromServer();
        m_webSocketClient->deleteLater();
        m_webSocketClient = nullptr;
    }

    m_authToken.clear();
    m_loginPassword.fill(QChar(u'\0'));
    m_loginPassword.clear();
    m_startupState.authenticated = false;
    m_startupState.initialized = false;
    m_startupState.failure.clear();
    m_currentUsername.clear();
    m_userInfo = QJsonObject();
    m_todayScheduleEntries = QJsonArray();
    m_currentPlanPayload = QJsonObject();
    m_latestTelemetryPayload = QJsonObject();
    m_lastDoseTelemetrySnapshot.clear();
    m_waitingOisPatientCallLogged = false;
    AuditLogger::instance().clearTreatmentContext();

    if (m_settings) {
        m_settings->remove(QStringLiteral("auth_token"));
        m_settings->remove(QStringLiteral("token_expire"));
        m_settings->sync();
    }

    updateTodaySchedule(QJsonArray());
    updateDoseInfoPanel();
    ui->spotMapWidget->setPlanPayload(QJsonObject());
    ui->patient_id->setText(tr("ID：-"));
    ui->patient_name->setText(tr("姓名：-"));
    ui->labelPlanIdValue->setText(QStringLiteral("-"));
    ui->labelPlanFractionIdValue->setText(QStringLiteral("-"));
    ui->labelPlanSiteTargetValue->setText(QStringLiteral("-"));
    ui->labelPlanIsocenterValue->setText(QStringLiteral("-"));
    ui->labelPlanQaReleaseValue->setText(QStringLiteral("-"));
    setTreatmentState(TreatmentState::Idle, QStringLiteral("logout"));

    updateBeamRoomOverview();
    initStatusIndicators();
    m_loginTransitionPending = false;
    if (closeAfterLogout) { close(); return; }
    m_webSocketClient = new WebSocketClient(this);
    connectWebSocketSignals();
    updateUserInfoUi();
    applyTreatmentStateToUi();
    initConnectionState();
}

void TreatmentWindow::on_beamApply_pushButton_clicked()
{
    auditTreatmentEvent(QStringLiteral("beam"),
                        QStringLiteral("apply_beam_clicked"),
                        QStringLiteral("beam"),
                        m_roomId,
                        QStringLiteral("attempt"),
                        tr("治疗师点击申请束流"));

    if (!ensureStateAllowed(
            {TreatmentState::ReadyToApplyBeam},
            tr("申请束流"))) {
        return;
    }

    QString confirmedUsername;
    QString confirmedPassword;
    if (!showBeamApplyConfirmationDialog(&confirmedUsername, &confirmedPassword)) {
        auditTreatmentEvent(QStringLiteral("beam"),
                            QStringLiteral("apply_beam_cancelled"),
                            QStringLiteral("beam"),
                            m_roomId,
                            QStringLiteral("cancelled"),
                            tr("申请束流二次确认被取消"));
        return;
    }

    if (!sendBeamApplyCommand(confirmedUsername, confirmedPassword)) {
        return;
    }

    setTreatmentState(TreatmentState::BeamRequested, "operator applied beam");
}

void TreatmentWindow::on_beamApply_cancel_pushButton_clicked()
{
    auditTreatmentEvent(QStringLiteral("beam"),
                        QStringLiteral("cancel_beam_clicked"),
                        QStringLiteral("beam"),
                        m_roomId,
                        QStringLiteral("attempt"),
                        tr("治疗师点击取消束流"));
}

void TreatmentWindow::on_confirmPlan_pushButton_clicked()
{
    auditTreatmentEvent(QStringLiteral("treatment_plan"),
                        QStringLiteral("confirm_plan_clicked"),
                        QStringLiteral("plan"),
                        m_currentPlanPayload.value("plan_id").toString(),
                        QStringLiteral("attempt"),
                        tr("治疗师点击确认治疗计划"));

    if (!ensureStateAllowed({TreatmentState::PlanConfirm}, tr("计划确认"))) {
        return;
    }

    QString confirmedUsername;
    QString confirmedPassword;
    if (!showPlanConfirmationDialog(&confirmedUsername, &confirmedPassword)) {
        auditTreatmentEvent(QStringLiteral("treatment_plan"),
                            QStringLiteral("plan_confirm_cancelled"),
                            QStringLiteral("plan"),
                            m_currentPlanPayload.value("plan_id").toString(),
                            QStringLiteral("cancelled"),
                            tr("治疗计划确认被取消"));
        return;
    }

    if (!sendPlanConfirmedCommand(confirmedUsername, confirmedPassword)) {
        return;
    }

    auditTreatmentEvent(QStringLiteral("treatment_plan"),
                        QStringLiteral("plan_confirmed"),
                        QStringLiteral("plan"),
                        m_currentPlanPayload.value("plan_id").toString(),
                        QStringLiteral("success"),
                        tr("治疗师已确认治疗计划"));

    setTreatmentState(TreatmentState::Preparing, "operator confirmed plan");
}

void TreatmentWindow::on_confirmPlan_cancel_pushButton_clicked()
{
    auditTreatmentEvent(QStringLiteral("treatment_plan"),
                        QStringLiteral("cancel_plan_confirm_clicked"),
                        QStringLiteral("plan"),
                        m_currentPlanPayload.value("plan_id").toString(),
                        QStringLiteral("attempt"),
                        tr("治疗师点击取消计划确认"));
}
