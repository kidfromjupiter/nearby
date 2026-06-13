/****************************************************************************
** Meta object code from reading C++ file 'file_share_tray_controller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../file_share_tray_controller.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'file_share_tray_controller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
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
struct qt_meta_tag_ZN23FileShareTrayControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto FileShareTrayController::qt_create_metaobjectdata<qt_meta_tag_ZN23FileShareTrayControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "FileShareTrayController",
        "modeChanged",
        "",
        "deviceNameChanged",
        "statusMessageChanged",
        "runningChanged",
        "pendingSendFileNameChanged",
        "pendingSendFilePathChanged",
        "discoveredTargetsChanged",
        "transfersChanged",
        "autoAcceptIncomingChanged",
        "qrCodeUrlChanged",
        "qrCodeChanged",
        "logPathChanged",
        "requestTrayMessage",
        "title",
        "body",
        "requestCopyLinkTrayMessage",
        "link",
        "start",
        "stop",
        "switchToReceiveMode",
        "switchToSendModeWithFile",
        "file_path",
        "sendPendingFileToTarget",
        "share_target_id",
        "copyTextToClipboard",
        "text",
        "openFileLocation",
        "clearTransfers",
        "hideToTray",
        "mode",
        "deviceName",
        "statusMessage",
        "running",
        "pendingSendFileName",
        "pendingSendFilePath",
        "discoveredTargets",
        "QVariantList",
        "transfers",
        "autoAcceptIncoming",
        "qrCodeUrl",
        "qrCodeRows",
        "qrCodeSize",
        "logPath"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modeChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'deviceNameChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'statusMessageChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'runningChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pendingSendFileNameChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pendingSendFilePathChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'discoveredTargetsChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'transfersChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'autoAcceptIncomingChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'qrCodeUrlChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'qrCodeChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'logPathChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'requestTrayMessage'
        QtMocHelpers::SignalData<void(const QString &, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 },
        }}),
        // Signal 'requestCopyLinkTrayMessage'
        QtMocHelpers::SignalData<void(const QString &, const QString &, const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 }, { QMetaType::QString, 18 },
        }}),
        // Method 'start'
        QtMocHelpers::MethodData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'stop'
        QtMocHelpers::MethodData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'switchToReceiveMode'
        QtMocHelpers::MethodData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'switchToSendModeWithFile'
        QtMocHelpers::MethodData<void(const QString &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 23 },
        }}),
        // Method 'sendPendingFileToTarget'
        QtMocHelpers::MethodData<void(qlonglong)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::LongLong, 25 },
        }}),
        // Method 'copyTextToClipboard'
        QtMocHelpers::MethodData<void(const QString &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 27 },
        }}),
        // Method 'openFileLocation'
        QtMocHelpers::MethodData<void(const QString &)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 23 },
        }}),
        // Method 'clearTransfers'
        QtMocHelpers::MethodData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'hideToTray'
        QtMocHelpers::MethodData<void()>(30, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'mode'
        QtMocHelpers::PropertyData<QString>(31, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'deviceName'
        QtMocHelpers::PropertyData<QString>(32, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'statusMessage'
        QtMocHelpers::PropertyData<QString>(33, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'running'
        QtMocHelpers::PropertyData<bool>(34, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
        // property 'pendingSendFileName'
        QtMocHelpers::PropertyData<QString>(35, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'pendingSendFilePath'
        QtMocHelpers::PropertyData<QString>(36, QMetaType::QString, QMC::DefaultPropertyFlags, 5),
        // property 'discoveredTargets'
        QtMocHelpers::PropertyData<QVariantList>(37, 0x80000000 | 38, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 6),
        // property 'transfers'
        QtMocHelpers::PropertyData<QVariantList>(39, 0x80000000 | 38, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 7),
        // property 'autoAcceptIncoming'
        QtMocHelpers::PropertyData<bool>(40, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 8),
        // property 'qrCodeUrl'
        QtMocHelpers::PropertyData<QString>(41, QMetaType::QString, QMC::DefaultPropertyFlags, 9),
        // property 'qrCodeRows'
        QtMocHelpers::PropertyData<QStringList>(42, QMetaType::QStringList, QMC::DefaultPropertyFlags, 10),
        // property 'qrCodeSize'
        QtMocHelpers::PropertyData<int>(43, QMetaType::Int, QMC::DefaultPropertyFlags, 10),
        // property 'logPath'
        QtMocHelpers::PropertyData<QString>(44, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 11),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<FileShareTrayController, qt_meta_tag_ZN23FileShareTrayControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject FileShareTrayController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23FileShareTrayControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23FileShareTrayControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN23FileShareTrayControllerE_t>.metaTypes,
    nullptr
} };

void FileShareTrayController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FileShareTrayController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modeChanged(); break;
        case 1: _t->deviceNameChanged(); break;
        case 2: _t->statusMessageChanged(); break;
        case 3: _t->runningChanged(); break;
        case 4: _t->pendingSendFileNameChanged(); break;
        case 5: _t->pendingSendFilePathChanged(); break;
        case 6: _t->discoveredTargetsChanged(); break;
        case 7: _t->transfersChanged(); break;
        case 8: _t->autoAcceptIncomingChanged(); break;
        case 9: _t->qrCodeUrlChanged(); break;
        case 10: _t->qrCodeChanged(); break;
        case 11: _t->logPathChanged(); break;
        case 12: _t->requestTrayMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->requestCopyLinkTrayMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 14: _t->start(); break;
        case 15: _t->stop(); break;
        case 16: _t->switchToReceiveMode(); break;
        case 17: _t->switchToSendModeWithFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->sendPendingFileToTarget((*reinterpret_cast<std::add_pointer_t<qlonglong>>(_a[1]))); break;
        case 19: _t->copyTextToClipboard((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->openFileLocation((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: _t->clearTransfers(); break;
        case 22: _t->hideToTray(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::modeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::deviceNameChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::statusMessageChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::runningChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::pendingSendFileNameChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::pendingSendFilePathChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::discoveredTargetsChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::transfersChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::autoAcceptIncomingChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::qrCodeUrlChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::qrCodeChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)()>(_a, &FileShareTrayController::logPathChanged, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)(const QString & , const QString & )>(_a, &FileShareTrayController::requestTrayMessage, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (FileShareTrayController::*)(const QString & , const QString & , const QString & )>(_a, &FileShareTrayController::requestCopyLinkTrayMessage, 13))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->mode(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->deviceName(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->statusMessage(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->running(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->pendingSendFileName(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->pendingSendFilePath(); break;
        case 6: *reinterpret_cast<QVariantList*>(_v) = _t->discoveredTargets(); break;
        case 7: *reinterpret_cast<QVariantList*>(_v) = _t->transfers(); break;
        case 8: *reinterpret_cast<bool*>(_v) = _t->autoAcceptIncoming(); break;
        case 9: *reinterpret_cast<QString*>(_v) = _t->qrCodeUrl(); break;
        case 10: *reinterpret_cast<QStringList*>(_v) = _t->qrCodeRows(); break;
        case 11: *reinterpret_cast<int*>(_v) = _t->qrCodeSize(); break;
        case 12: *reinterpret_cast<QString*>(_v) = _t->logPath(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 1: _t->setDeviceName(*reinterpret_cast<QString*>(_v)); break;
        case 8: _t->setAutoAcceptIncoming(*reinterpret_cast<bool*>(_v)); break;
        case 12: _t->setLogPath(*reinterpret_cast<QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *FileShareTrayController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FileShareTrayController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN23FileShareTrayControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int FileShareTrayController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 23;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void FileShareTrayController::modeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void FileShareTrayController::deviceNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void FileShareTrayController::statusMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void FileShareTrayController::runningChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void FileShareTrayController::pendingSendFileNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void FileShareTrayController::pendingSendFilePathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void FileShareTrayController::discoveredTargetsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void FileShareTrayController::transfersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void FileShareTrayController::autoAcceptIncomingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void FileShareTrayController::qrCodeUrlChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void FileShareTrayController::qrCodeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void FileShareTrayController::logPathChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void FileShareTrayController::requestTrayMessage(const QString & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1, _t2);
}

// SIGNAL 13
void FileShareTrayController::requestCopyLinkTrayMessage(const QString & _t1, const QString & _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
