/****************************************************************************
** Meta object code from reading C++ file 'Injector.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/Injector.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Injector.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
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
struct qt_meta_tag_ZN8InjectorE_t {};
} // unnamed namespace

template <> constexpr inline auto Injector::qt_create_metaobjectdata<qt_meta_tag_ZN8InjectorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Injector",
        "deviceConnected",
        "",
        "deviceDisconnected",
        "payloadInjected",
        "payloadPath",
        "bytesSent",
        "injectionFailed",
        "errorMessage",
        "logMessage",
        "message",
        "onProcessFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus",
        "onProcessError",
        "QProcess::ProcessError",
        "error"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'deviceConnected'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'deviceDisconnected'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'payloadInjected'
        QtMocHelpers::SignalData<void(const QString &, int)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 }, { QMetaType::Int, 6 },
        }}),
        // Signal 'injectionFailed'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
        // Signal 'logMessage'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 12 }, { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onProcessError'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Injector, qt_meta_tag_ZN8InjectorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Injector::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8InjectorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8InjectorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8InjectorE_t>.metaTypes,
    nullptr
} };

void Injector::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Injector *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->deviceConnected(); break;
        case 1: _t->deviceDisconnected(); break;
        case 2: _t->payloadInjected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->injectionFailed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->logMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onProcessFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 6: _t->onProcessError((*reinterpret_cast<std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Injector::*)()>(_a, &Injector::deviceConnected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Injector::*)()>(_a, &Injector::deviceDisconnected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Injector::*)(const QString & , int )>(_a, &Injector::payloadInjected, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Injector::*)(const QString & )>(_a, &Injector::injectionFailed, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Injector::*)(const QString & )>(_a, &Injector::logMessage, 4))
            return;
    }
}

const QMetaObject *Injector::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Injector::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8InjectorE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Injector::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void Injector::deviceConnected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Injector::deviceDisconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Injector::payloadInjected(const QString & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void Injector::injectionFailed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void Injector::logMessage(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
