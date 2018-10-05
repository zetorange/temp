#include "cateyes.h"

#include "device.h"
#include "devicelistmodel.h"

Cateyes *Cateyes::s_instance = nullptr;

Cateyes::Cateyes(QObject *parent) :
    QObject(parent),
    m_localSystem(nullptr),
    m_mainContext(nullptr)
{
    cateyes_init();

    g_mutex_init(&m_mutex);
    g_cond_init(&m_cond);

    m_mainContext = new MainContext(cateyes_get_main_context());
    m_mainContext->schedule([this] () { initialize(); });

    g_mutex_lock(&m_mutex);
    while (m_localSystem == nullptr)
        g_cond_wait(&m_cond, &m_mutex);
    g_mutex_unlock(&m_mutex);
}

void Cateyes::initialize()
{
    m_handle = cateyes_device_manager_new();
    g_signal_connect_swapped(m_handle, "added", G_CALLBACK(onDeviceAddedWrapper), this);
    g_signal_connect_swapped(m_handle, "removed", G_CALLBACK(onDeviceRemovedWrapper), this);
    cateyes_device_manager_enumerate_devices(m_handle, onEnumerateDevicesReadyWrapper, this);
}

void Cateyes::dispose()
{
    g_signal_handlers_disconnect_by_func(m_handle, GSIZE_TO_POINTER(onDeviceRemovedWrapper), this);
    g_signal_handlers_disconnect_by_func(m_handle, GSIZE_TO_POINTER(onDeviceAddedWrapper), this);
    g_object_unref(m_handle);
    m_handle = nullptr;
}

Cateyes::~Cateyes()
{
    m_localSystem = nullptr;
    foreach (Device *device, m_deviceItems)
        delete device;
    m_deviceItems.clear();

    cateyes_device_manager_close_sync(m_handle);
    m_mainContext->perform([this] () { dispose(); });
    delete m_mainContext;

    g_cond_clear(&m_cond);
    g_mutex_clear(&m_mutex);

    s_instance = nullptr;

    cateyes_deinit();
}

Cateyes *Cateyes::instance()
{
    if (s_instance == nullptr)
        s_instance = new Cateyes();
    return s_instance;
}

void Cateyes::onEnumerateDevicesReadyWrapper(GObject *obj, GAsyncResult *res, gpointer data)
{
    Q_UNUSED(obj);

    static_cast<Cateyes *>(data)->onEnumerateDevicesReady(res);
}

void Cateyes::onEnumerateDevicesReady(GAsyncResult *res)
{
    GError *error = nullptr;
    CateyesDeviceList *devices = cateyes_device_manager_enumerate_devices_finish(m_handle, res, &error);
    g_assert(error == nullptr);

    gint count = cateyes_device_list_size(devices);
    for (gint i = 0; i != count; i++) {
        CateyesDevice *device = cateyes_device_list_get(devices, i);
        onDeviceAdded(device);
        g_object_unref(device);
    }

    g_object_unref(devices);
}

void Cateyes::onDeviceAddedWrapper(Cateyes *self, CateyesDevice *deviceHandle)
{
    self->onDeviceAdded(deviceHandle);
}

void Cateyes::onDeviceRemovedWrapper(Cateyes *self, CateyesDevice *deviceHandle)
{
    self->onDeviceRemoved(deviceHandle);
}

void Cateyes::onDeviceAdded(CateyesDevice *deviceHandle)
{
    auto device = new Device(deviceHandle);
    device->moveToThread(this->thread());
    if (device->type() == Device::Local) {
        g_mutex_lock(&m_mutex);
        m_localSystem = device;
        g_cond_signal(&m_cond);
        g_mutex_unlock(&m_mutex);
    }
    QMetaObject::invokeMethod(this, "add", Qt::QueuedConnection, Q_ARG(Device *, device));
}

void Cateyes::onDeviceRemoved(CateyesDevice *deviceHandle)
{
    QMetaObject::invokeMethod(this, "removeById", Qt::QueuedConnection, Q_ARG(QString, cateyes_device_get_id(deviceHandle)));
}

void Cateyes::add(Device *device)
{
    device->setParent(this);
    m_deviceItems.append(device);
    emit deviceAdded(device);
}

void Cateyes::removeById(QString id)
{
    for (int i = 0; i != m_deviceItems.size(); i++) {
        auto device = m_deviceItems.at(i);
        if (device->id() == id) {
            m_deviceItems.removeAt(i);
            emit deviceRemoved(device);
            delete device;
            break;
        }
    }
}
