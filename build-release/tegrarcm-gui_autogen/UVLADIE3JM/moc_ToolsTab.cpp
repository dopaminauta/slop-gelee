/****************************************************************************
** Meta object code from reading C++ file 'ToolsTab.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ToolsTab.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ToolsTab.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8ToolsTabE_t {};
} // unnamed namespace

template <> constexpr inline auto ToolsTab::qt_create_metaobjectdata<qt_meta_tag_ZN8ToolsTabE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ToolsTab",
        "onLogMessage",
        "",
        "message",
        "onPayloadInjected",
        "path",
        "bytesSent",
        "onInjectionFailed",
        "errorMessage"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onLogMessage'
        QtMocHelpers::SlotData<void(const QString &)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'onPayloadInjected'
        QtMocHelpers::SlotData<void(const QString &, int)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 5 }, { QMetaType::Int, 6 },
        }}),
        // Slot 'onInjectionFailed'
        QtMocHelpers::SlotData<void(const QString &)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 8 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ToolsTab, qt_meta_tag_ZN8ToolsTabE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ToolsTab::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8ToolsTabE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8ToolsTabE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8ToolsTabE_t>.metaTypes,
    nullptr
} };

void ToolsTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ToolsTab *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onLogMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->onPayloadInjected((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->onInjectionFailed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *ToolsTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ToolsTab::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8ToolsTabE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ToolsTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}
QT_WARNING_POP
