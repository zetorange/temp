#include <cateyes-core.h>

#include "plugin.h"

#include "device.h"
#include "devicelistmodel.h"
#include "cateyes.h"
#include "iconprovider.h"
#include "process.h"
#include "processlistmodel.h"
#include "script.h"

#include <qqml.h>

static QObject *createCateyesSingleton(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);

    return Cateyes::instance();
}

void Cateyes_QmlPlugin::registerTypes(const char *uri)
{
    qRegisterMetaType<Device *>("Device *");
    qRegisterMetaType<QList<Process *>>("QList<Process *>");
    qRegisterMetaType<QSet<unsigned int>>("QSet<unsigned int>");
    qRegisterMetaType<ScriptInstance::Status>("ScriptInstance::Status");

    // @uri Cateyes
    qmlRegisterSingletonType<Cateyes>(uri, 1, 0, "Cateyes", createCateyesSingleton);
    qmlRegisterType<DeviceListModel>(uri, 1, 0, "DeviceListModel");
    qmlRegisterType<ProcessListModel>(uri, 1, 0, "ProcessListModel");
    qmlRegisterType<Script>(uri, 1, 0, "Script");

    qmlRegisterUncreatableType<Device>(uri, 1, 0, "Device", "Device objects cannot be instantiated from Qml");
    qmlRegisterUncreatableType<Process>(uri, 1, 0, "Process", "Process objects cannot be instantiated from Qml");
}

void Cateyes_QmlPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(uri);

    // Ensure Cateyes is initialized
    Cateyes::instance();

    engine->addImageProvider("cateyes", IconProvider::instance());
}
