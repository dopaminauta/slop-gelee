/****************************************************************************
** Meta object code from reading C++ file 'UdevRules.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/UdevRules.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UdevRules.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9UdevRulesE_t {};
} // unnamed namespace

template <> constexpr inline auto UdevRules::qt_create_metaobjectdata<qt_meta_tag_ZN9UdevRulesE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "UdevRules",
        "logMessage",
        "",
        "message",
        "finished",
        "success",
        "onProcessFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus",
        "onProcessError",
        "QProcess::ProcessError",
        "error"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'logMessage'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'finished'
        QtMocHelpers::SignalData<void(bool)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 5 },
        }}),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 7 }, { 0x80000000 | 8, 9 },
        }}),
        // Slot 'onProcessError'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<UdevRules, qt_meta_tag_ZN9UdevRulesE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject UdevRules::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9UdevRulesE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9UdevRulesE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9UdevRulesE_t>.metaTypes,
    nullptr
} };

void UdevRules::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<UdevRules *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->logMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->finished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->onProcessFinished((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 3: _t->onProcessError((*reinterpret_cast<std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (UdevRules::*)(const QString & )>(_a, &UdevRules::logMessage, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (UdevRules::*)(bool )>(_a, &UdevRules::finished, 1))
            return;
    }
}

const QMetaObject *UdevRules::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UdevRules::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9UdevRulesE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UdevRules::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void UdevRules::logMessage(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void UdevRules::finished(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
