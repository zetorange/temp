#include "device.h"

#include "script.h"

#include <memory>
#include <QDebug>
#include <QJsonDocument>

Device::Device(CateyesDevice *handle, QObject *parent) :
    QObject(parent),
    m_handle(handle),
    m_id(cateyes_device_get_id(handle)),
    m_name(cateyes_device_get_name(handle)),
    m_icon(IconProvider::instance()->add(cateyes_device_get_icon(handle))),
    m_type(static_cast<Device::Type>(cateyes_device_get_dtype(handle))),
    m_gcTimer(nullptr),
    m_mainContext(cateyes_get_main_context())
{
    g_object_ref(m_handle);
    g_object_set_data(G_OBJECT(m_handle), "qdevice", this);
}

void Device::dispose()
{
    if (m_gcTimer != nullptr) {
        g_source_destroy(m_gcTimer);
        m_gcTimer = nullptr;
    }

    auto it = m_sessions.constBegin();
    while (it != m_sessions.constEnd()) {
        delete it.value();
        ++it;
    }

    g_object_set_data(G_OBJECT(m_handle), "qdevice", nullptr);
    g_object_unref(m_handle);
}

Device::~Device()
{
    IconProvider::instance()->remove(m_icon);

    m_mainContext.perform([this] () { dispose(); });
}

void Device::inject(Script *script, unsigned int pid)
{
    ScriptInstance *scriptInstance = script != nullptr ? script->bind(this, pid) : nullptr;
    if (scriptInstance != nullptr) {
        auto onStatusChanged = std::make_shared<QMetaObject::Connection>();
        auto onStopRequest = std::make_shared<QMetaObject::Connection>();
        auto onSend = std::make_shared<QMetaObject::Connection>();
        auto onEnableDebugger = std::make_shared<QMetaObject::Connection>();
        auto onDisableDebugger = std::make_shared<QMetaObject::Connection>();
        auto onEnableJit = std::make_shared<QMetaObject::Connection>();
        *onStatusChanged = connect(script, &Script::statusChanged, [=] (Script::Status newStatus) {
            if (newStatus == Script::Loaded) {
                auto name = script->name();
                auto source = script->source();
                m_mainContext.schedule([=] () { performLoad(scriptInstance, name, source); });
            }
        });
        *onStopRequest = connect(scriptInstance, &ScriptInstance::stopRequest, [=] () {
            QObject::disconnect(*onStatusChanged);
            QObject::disconnect(*onStopRequest);
            QObject::disconnect(*onSend);
            QObject::disconnect(*onEnableDebugger);
            QObject::disconnect(*onDisableDebugger);
            QObject::disconnect(*onEnableJit);

            script->unbind(scriptInstance);

            m_mainContext.schedule([=] () { performStop(scriptInstance); });
        });
        *onSend = connect(scriptInstance, &ScriptInstance::send, [=] (QJsonObject object) {
            m_mainContext.schedule([=] () { performPost(scriptInstance, object); });
        });
        *onEnableDebugger = connect(scriptInstance, &ScriptInstance::enableDebuggerRequest, [=] (quint16 port) {
            m_mainContext.schedule([=] () { performEnableDebugger(scriptInstance, port); });
        });
        *onDisableDebugger = connect(scriptInstance, &ScriptInstance::disableDebuggerRequest, [=] () {
            m_mainContext.schedule([=] () { performDisableDebugger(scriptInstance); });
        });
        *onEnableJit = connect(scriptInstance, &ScriptInstance::enableJitRequest, [=] () {
            m_mainContext.schedule([=] () { performEnableJit(scriptInstance); });
        });

        m_mainContext.schedule([=] () { performInject(pid, scriptInstance); });

        if (script->status() == Script::Loaded) {
            auto name = script->name();
            auto source = script->source();
            m_mainContext.schedule([=] () { performLoad(scriptInstance, name, source); });
        }
    }
}

void Device::performInject(unsigned int pid, ScriptInstance *wrapper)
{
    auto session = m_sessions[pid];
    if (session == nullptr) {
        session = new SessionEntry(this, pid);
        m_sessions[pid] = session;
        connect(session, &SessionEntry::detached, [=] () {
            foreach (ScriptEntry *script, session->scripts())
                m_scripts.remove(script->wrapper());
            m_sessions.remove(pid);
            m_mainContext.schedule([=] () {
                delete session;
            });
        });
    }

    auto script = session->add(wrapper);
    m_scripts[wrapper] = script;
    connect(script, &ScriptEntry::stopped, [=] () {
        m_mainContext.schedule([=] () { delete script; });
    });
}

void Device::performLoad(ScriptInstance *wrapper, QString name, QString source)
{
    auto script = m_scripts[wrapper];
    if (script == nullptr)
        return;
    script->load(name, source);
}

void Device::performStop(ScriptInstance *wrapper)
{
    auto script = m_scripts[wrapper];
    if (script == nullptr)
        return;
    m_scripts.remove(wrapper);

    script->session()->remove(script);

    scheduleGarbageCollect();
}

void Device::performPost(ScriptInstance *wrapper, QJsonObject object)
{
    auto script = m_scripts[wrapper];
    if (script == nullptr)
        return;
    script->post(object);
}

void Device::performEnableDebugger(ScriptInstance *wrapper, quint16 port)
{
    auto script = m_scripts[wrapper];
    if (script == nullptr)
        return;
    script->session()->enableDebugger(port);
}

void Device::performDisableDebugger(ScriptInstance *wrapper)
{
    auto script = m_scripts[wrapper];
    if (script == nullptr)
        return;
    script->session()->disableDebugger();
}

void Device::performEnableJit(ScriptInstance *wrapper)
{
    auto script = m_scripts[wrapper];
    if (script == nullptr)
        return;
    script->session()->enableJit();
}

void Device::scheduleGarbageCollect()
{
    if (m_gcTimer != nullptr) {
        g_source_destroy(m_gcTimer);
        m_gcTimer = nullptr;
    }

    auto timer = g_timeout_source_new_seconds(5);
    g_source_set_callback(timer, onGarbageCollectTimeoutWrapper, this, nullptr);
    g_source_attach(timer, m_mainContext.handle());
    g_source_unref(timer);
    m_gcTimer = timer;
}

gboolean Device::onGarbageCollectTimeoutWrapper(gpointer data)
{
    static_cast<Device *>(data)->onGarbageCollectTimeout();

    return FALSE;
}

void Device::onGarbageCollectTimeout()
{
    m_gcTimer = nullptr;

    auto newSessions = QHash<unsigned int, SessionEntry *>();
    auto it = m_sessions.constBegin();
    while (it != m_sessions.constEnd()) {
        auto pid = it.key();
        auto session = it.value();
        if (session->scripts().isEmpty()) {
            delete session;
        } else {
            newSessions[pid] = session;
        }
        ++it;
    }
    m_sessions = newSessions;
}

SessionEntry::SessionEntry(Device *device, unsigned int pid, QObject *parent) :
    QObject(parent),
    m_device(device),
    m_pid(pid),
    m_handle(nullptr)
{
    cateyes_device_attach(device->handle(), pid, onAttachReadyWrapper, this);
}

SessionEntry::~SessionEntry()
{
    if (m_handle != nullptr) {
        cateyes_session_detach(m_handle, nullptr, nullptr);

        g_signal_handlers_disconnect_by_func(m_handle, GSIZE_TO_POINTER(onDetachedWrapper), this);

        g_object_set_data(G_OBJECT(m_handle), "qsession", nullptr);
        g_object_unref(m_handle);
    }
}

ScriptEntry *SessionEntry::add(ScriptInstance *wrapper)
{
    auto script = new ScriptEntry(this, wrapper, this);
    m_scripts.append(script);
    script->updateSessionHandle(m_handle);
    return script;
}

void SessionEntry::remove(ScriptEntry *script)
{
    script->stop();
    m_scripts.removeOne(script);
}

void SessionEntry::enableDebugger(quint16 port)
{
  if (m_handle == nullptr)
    return;

  cateyes_session_enable_debugger (m_handle, port, NULL, NULL);
}

void SessionEntry::disableDebugger()
{
  if (m_handle == nullptr)
    return;

  cateyes_session_disable_debugger (m_handle, NULL, NULL);
}

void SessionEntry::enableJit()
{
  if (m_handle == nullptr)
    return;

  cateyes_session_enable_jit (m_handle, NULL, NULL);
}

void SessionEntry::onAttachReadyWrapper(GObject *obj, GAsyncResult *res, gpointer data)
{
    if (g_object_get_data(obj, "qdevice") != nullptr) {
        static_cast<SessionEntry *>(data)->onAttachReady(res);
    }
}

void SessionEntry::onAttachReady(GAsyncResult *res)
{
    GError *error = nullptr;
    m_handle = cateyes_device_attach_finish(m_device->handle(), res, &error);
    if (error == nullptr) {
        g_object_set_data(G_OBJECT(m_handle), "qsession", this);

        g_signal_connect_swapped(m_handle, "detached", G_CALLBACK(onDetachedWrapper), this);

        foreach (ScriptEntry *script, m_scripts) {
            script->updateSessionHandle(m_handle);
        }
    } else {
        foreach (ScriptEntry *script, m_scripts) {
            script->notifySessionError(error);
        }
        g_clear_error(&error);
    }
}

void SessionEntry::onDetachedWrapper(SessionEntry *self, CateyesSessionDetachReason reason)
{
    self->onDetached(static_cast<DetachReason>(reason));
}

void SessionEntry::onDetached(DetachReason reason)
{
    const char *message;
    switch (reason) {
    case ApplicationRequested:
        message = "Detached by application";
        break;
    case ProcessTerminated:
        message = "Process terminated";
        break;
    case ServerTerminated:
        message = "Server terminated";
        break;
    case DeviceGone:
        message = "Device gone";
        break;
    default:
        g_assert_not_reached();
    }

    foreach (ScriptEntry *script, m_scripts)
        script->notifySessionError(message);

    emit detached(reason);
}

ScriptEntry::ScriptEntry(SessionEntry *session, ScriptInstance *wrapper, QObject *parent) :
    QObject(parent),
    m_status(ScriptInstance::Loading),
    m_session(session),
    m_wrapper(wrapper),
    m_handle(nullptr),
    m_sessionHandle(nullptr)
{
}

ScriptEntry::~ScriptEntry()
{
    if (m_handle != nullptr) {
        cateyes_script_unload(m_handle, nullptr, nullptr);

        g_signal_handlers_disconnect_by_func(m_handle, GSIZE_TO_POINTER(onMessage), this);

        g_object_set_data(G_OBJECT(m_handle), "qscript", nullptr);
        g_object_unref(m_handle);
    }
}

void ScriptEntry::updateSessionHandle(CateyesSession *sessionHandle)
{
    m_sessionHandle = sessionHandle;
    start();
}

void ScriptEntry::notifySessionError(GError *error)
{
    updateError(error);
    updateStatus(ScriptInstance::Error);
}

void ScriptEntry::notifySessionError(QString message)
{
    updateError(message);
    updateStatus(ScriptInstance::Error);
}

void ScriptEntry::post(QJsonObject object)
{
    if (m_status == ScriptInstance::Started) {
        performPost(object);
    } else if (m_status < ScriptInstance::Started) {
        m_pending.enqueue(object);
    } else {
        // Drop silently
    }
}

void ScriptEntry::updateStatus(ScriptInstance::Status status)
{
    if (status == m_status)
        return;

    m_status = status;

    QMetaObject::invokeMethod(m_wrapper, "onStatus", Qt::QueuedConnection,
        Q_ARG(ScriptInstance::Status, status));

    if (status == ScriptInstance::Started) {
        while (!m_pending.isEmpty())
            performPost(m_pending.dequeue());
    } else if (status > ScriptInstance::Started) {
        m_pending.clear();
    }
}

void ScriptEntry::updateError(GError *error)
{
    updateError(QString::fromUtf8(error->message));
}

void ScriptEntry::updateError(QString message)
{
    QMetaObject::invokeMethod(m_wrapper, "onError", Qt::QueuedConnection,
        Q_ARG(QString, message));
}

void ScriptEntry::load(QString name, QString source)
{
    if (m_status != ScriptInstance::Loading)
        return;

    m_name = name;
    m_source = source;
    updateStatus(ScriptInstance::Loaded);

    start();
}

void ScriptEntry::start()
{
    if (m_status == ScriptInstance::Loading)
        return;

    if (m_sessionHandle != nullptr) {
        updateStatus(ScriptInstance::Compiling);
        auto name = m_name.toUtf8();
        auto source = m_source.toUtf8();
        cateyes_session_create_script(m_sessionHandle, !m_name.isEmpty() ? name.data() : NULL, source.data(),
            onCreateReadyWrapper, this);
    } else {
        updateStatus(ScriptInstance::Establishing);
    }
}

void ScriptEntry::stop()
{
    bool canStopNow = m_status != ScriptInstance::Compiling && m_status != ScriptInstance::Starting;

    m_status = ScriptInstance::Destroyed;

    if (canStopNow)
        emit stopped();
}

void ScriptEntry::onCreateReadyWrapper(GObject *obj, GAsyncResult *res, gpointer data)
{
    if (g_object_get_data(obj, "qsession") != nullptr) {
        static_cast<ScriptEntry *>(data)->onCreateReady(res);
    }
}

void ScriptEntry::onCreateReady(GAsyncResult *res)
{
    if (m_status == ScriptInstance::Destroyed) {
        emit stopped();
        return;
    }

    GError *error = nullptr;
    m_handle = cateyes_session_create_script_finish(m_sessionHandle, res, &error);
    if (error == nullptr) {
        g_object_set_data(G_OBJECT(m_handle), "qscript", this);

        g_signal_connect_swapped(m_handle, "message", G_CALLBACK(onMessage), this);

        updateStatus(ScriptInstance::Starting);
        cateyes_script_load(m_handle, onLoadReadyWrapper, this);
    } else {
        updateError(error);
        updateStatus(ScriptInstance::Error);
        g_clear_error(&error);
    }
}

void ScriptEntry::onLoadReadyWrapper(GObject *obj, GAsyncResult *res, gpointer data)
{
    if (g_object_get_data(obj, "qscript") != nullptr) {
        static_cast<ScriptEntry *>(data)->onLoadReady(res);
    }
}

void ScriptEntry::onLoadReady(GAsyncResult *res)
{
    if (m_status == ScriptInstance::Destroyed) {
        emit stopped();
        return;
    }

    GError *error = nullptr;
    cateyes_script_load_finish(m_handle, res, &error);
    if (error == nullptr) {
        updateStatus(ScriptInstance::Started);
    } else {
        updateError(error);
        updateStatus(ScriptInstance::Error);
        g_clear_error(&error);
    }
}

void ScriptEntry::performPost(QJsonObject object)
{
    QJsonDocument document(object);
    auto json = document.toJson(QJsonDocument::Compact);
    cateyes_script_post(m_handle, json.data(), nullptr, nullptr, nullptr);
}

void ScriptEntry::onMessage(ScriptEntry *self, const gchar *message, GBytes *data)
{
    auto messageJson = QByteArray::fromRawData(message, static_cast<int>(strlen(message)));
    auto messageDocument = QJsonDocument::fromJson(messageJson);
    auto messageObject = messageDocument.object();

    if (messageObject["type"] == "log") {
        auto logMessage = messageObject["payload"].toString().toUtf8();
        qDebug("%s", logMessage.data());
    } else {
        QVariant dataValue;
        if (data != NULL) {
            gsize dataSize;
            auto dataBuffer = static_cast<const char *>(g_bytes_get_data(data, &dataSize));
            dataValue = QByteArray(dataBuffer, dataSize);
        }

        QMetaObject::invokeMethod(self->m_wrapper, "onMessage", Qt::QueuedConnection,
            Q_ARG(QJsonObject, messageDocument.object()),
            Q_ARG(QVariant, dataValue));
    }
}
