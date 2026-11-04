/****************************************************************************
** Meta object code from reading C++ file 'treatmentwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../apps/treatment/treatmentwindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'treatmentwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN15TreatmentWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto TreatmentWindow::qt_create_metaobjectdata<qt_meta_tag_ZN15TreatmentWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TreatmentWindow",
        "onThemeChanged",
        "",
        "Theme",
        "newTheme",
        "onLoginSuccess",
        "username",
        "authToken",
        "WebSocketClient*",
        "client",
        "QJsonObject",
        "loginResponse",
        "onLoginFailed",
        "error",
        "onWebSocketConnected",
        "onWebSocketDisconnected",
        "onWebSocketError",
        "onWebSocketDataReceived",
        "data",
        "onWebSocketDataSent",
        "messageId",
        "initTheme",
        "initWebSocket",
        "initBedView",
        "initStepView",
        "initConnectionState",
        "handleRspMessage",
        "handleTelemetryMessage",
        "handleTaskMessage",
        "handleGrpcCall",
        "handleTaskErrorStopped",
        "on_bed_pushButton_clicked",
        "on_todays_pushButton_clicked",
        "on_theme_pushButton_clicked",
        "on_logout_pushButton_clicked",
        "on_beamApply_pushButton_clicked",
        "on_beamApply_cancel_pushButton_clicked",
        "on_confirmPlan_pushButton_clicked",
        "on_confirmPlan_cancel_pushButton_clicked",
        "showCurrentPlanDetail"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onThemeChanged'
        QtMocHelpers::SlotData<void(Theme)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onLoginSuccess'
        QtMocHelpers::SlotData<void(const QString &, const QString &, WebSocketClient *, const QJsonObject &)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 6 }, { QMetaType::QString, 7 }, { 0x80000000 | 8, 9 }, { 0x80000000 | 10, 11 },
        }}),
        // Slot 'onLoginFailed'
        QtMocHelpers::SlotData<void(const QString &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Slot 'onWebSocketConnected'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onWebSocketDisconnected'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onWebSocketError'
        QtMocHelpers::SlotData<void(const QString &)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Slot 'onWebSocketDataReceived'
        QtMocHelpers::SlotData<void(const QJsonObject &)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 18 },
        }}),
        // Slot 'onWebSocketDataSent'
        QtMocHelpers::SlotData<void(const QString &)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Slot 'initTheme'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'initWebSocket'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'initBedView'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'initStepView'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'initConnectionState'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleRspMessage'
        QtMocHelpers::SlotData<void(const QJsonObject &)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 18 },
        }}),
        // Slot 'handleTelemetryMessage'
        QtMocHelpers::SlotData<void(const QJsonObject &)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 18 },
        }}),
        // Slot 'handleTaskMessage'
        QtMocHelpers::SlotData<void(const QJsonObject &)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 18 },
        }}),
        // Slot 'handleGrpcCall'
        QtMocHelpers::SlotData<void(const QJsonObject &)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 18 },
        }}),
        // Slot 'handleTaskErrorStopped'
        QtMocHelpers::SlotData<void(const QJsonObject &)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 18 },
        }}),
        // Slot 'on_bed_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_todays_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_theme_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_logout_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_beamApply_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_beamApply_cancel_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_confirmPlan_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_confirmPlan_cancel_pushButton_clicked'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showCurrentPlanDetail'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TreatmentWindow, qt_meta_tag_ZN15TreatmentWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TreatmentWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15TreatmentWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15TreatmentWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15TreatmentWindowE_t>.metaTypes,
    nullptr
} };

void TreatmentWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TreatmentWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onThemeChanged((*reinterpret_cast<std::add_pointer_t<Theme>>(_a[1]))); break;
        case 1: _t->onLoginSuccess((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<WebSocketClient*>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[4]))); break;
        case 2: _t->onLoginFailed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onWebSocketConnected(); break;
        case 4: _t->onWebSocketDisconnected(); break;
        case 5: _t->onWebSocketError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->onWebSocketDataReceived((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 7: _t->onWebSocketDataSent((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->initTheme(); break;
        case 9: _t->initWebSocket(); break;
        case 10: _t->initBedView(); break;
        case 11: _t->initStepView(); break;
        case 12: _t->initConnectionState(); break;
        case 13: _t->handleRspMessage((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 14: _t->handleTelemetryMessage((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 15: _t->handleTaskMessage((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 16: _t->handleGrpcCall((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 17: _t->handleTaskErrorStopped((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 18: _t->on_bed_pushButton_clicked(); break;
        case 19: _t->on_todays_pushButton_clicked(); break;
        case 20: _t->on_theme_pushButton_clicked(); break;
        case 21: _t->on_logout_pushButton_clicked(); break;
        case 22: _t->on_beamApply_pushButton_clicked(); break;
        case 23: _t->on_beamApply_cancel_pushButton_clicked(); break;
        case 24: _t->on_confirmPlan_pushButton_clicked(); break;
        case 25: _t->on_confirmPlan_cancel_pushButton_clicked(); break;
        case 26: _t->showCurrentPlanDetail(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 2:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< WebSocketClient* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *TreatmentWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TreatmentWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15TreatmentWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int TreatmentWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    }
    return _id;
}
QT_WARNING_POP
