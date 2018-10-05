#ifndef CATEYESQML_CATEYES_H
#define CATEYESQML_CATEYES_H

#include <cateyes-core.h>

#include "maincontext.h"

#include <QObject>

class Device;
class Scripts;

class Cateyes : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Cateyes)
    Q_PROPERTY(Device *localSystem READ localSystem CONSTANT)

public:
    explicit Cateyes(QObject *parent = nullptr);
private:
    void initialize();
    void dispose();
public:
    ~Cateyes();

    static Cateyes *instance();

    Device *localSystem() const { return m_localSystem; }

    QList<Device *> deviceItems() const { return m_deviceItems; }

signals:
    void localSystemChanged(Device *newLocalSystem);
    void deviceAdded(Device *device);
    void deviceRemoved(Device *device);

private:
    static void onEnumerateDevicesReadyWrapper(GObject *obj, GAsyncResult *res, gpointer data);
    void onEnumerateDevicesReady(GAsyncResult *res);
    static void onDeviceAddedWrapper(Cateyes *self, CateyesDevice *deviceHandle);
    static void onDeviceRemovedWrapper(Cateyes *self, CateyesDevice *deviceHandle);
    void onDeviceAdded(CateyesDevice *deviceHandle);
    void onDeviceRemoved(CateyesDevice *deviceHandle);

private slots:
    void add(Device *device);
    void removeById(QString id);

private:
    CateyesDeviceManager *m_handle;
    QList<Device *> m_deviceItems;
    Device *m_localSystem;
    MainContext *m_mainContext;
    GMutex m_mutex;
    GCond m_cond;

    static Cateyes *s_instance;
};

#endif
