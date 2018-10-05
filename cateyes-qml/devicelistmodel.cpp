#include "devicelistmodel.h"

#include "device.h"
#include "cateyes.h"

static const int DeviceNameRole = Qt::UserRole + 0;
static const int DeviceIconRole = Qt::UserRole + 1;
static const int DeviceTypeRole = Qt::UserRole + 2;

DeviceListModel::DeviceListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    auto cateyes = Cateyes::instance();
    m_devices = cateyes->deviceItems();
    connect(cateyes, &Cateyes::deviceAdded, this, &DeviceListModel::onDeviceAdded);
    connect(cateyes, &Cateyes::deviceRemoved, this, &DeviceListModel::onDeviceRemoved);
}

Device *DeviceListModel::get(int index) const
{
    if (index < 0 || index >= m_devices.size())
        return nullptr;

    return m_devices[index];
}

QHash<int, QByteArray> DeviceListModel::roleNames() const
{
    QHash<int, QByteArray> r;
    r[Qt::DisplayRole] = "display";
    r[DeviceNameRole] = "name";
    r[DeviceIconRole] = "icon";
    r[DeviceTypeRole] = "type";
    return r;
}

int DeviceListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_devices.size();
}

QVariant DeviceListModel::data(const QModelIndex &index, int role) const
{
    auto device = m_devices[index.row()];
    switch (role) {
    case Qt::DisplayRole:
    case DeviceNameRole:
        return QVariant(device->name());
    case DeviceIconRole:
        return QVariant(device->icon());
    case DeviceTypeRole:
        return QVariant(device->type());
    default:
        return QVariant();
    }
}

void DeviceListModel::onDeviceAdded(Device *device)
{
    auto rowIndex = m_devices.size();
    beginInsertRows(QModelIndex(), rowIndex, rowIndex);
    m_devices.append(device);
    endInsertRows();
}

void DeviceListModel::onDeviceRemoved(Device *device)
{
    auto rowIndex = m_devices.indexOf(device);
    beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
    m_devices.removeAt(rowIndex);
    endRemoveRows();
}
