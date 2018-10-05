#ifndef CATEYESQML_ICONPROVIDER_H
#define CATEYESQML_ICONPROVIDER_H

#include <cateyes-core.h>

#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>
#include <QUrl>

class Icon
{
public:
    Icon() :
        m_id(-1),
        m_url(QUrl())
    {
    }

    Icon(int id, QUrl url) :
        m_id(id),
        m_url(url)
    {
    }

    bool isValid() const { return m_id != -1; }
    int id() const { return m_id; }
    QUrl url() const { return m_url; }

private:
    int m_id;
    QUrl m_url;
};

class IconProvider : public QQuickImageProvider
{
public:
    explicit IconProvider();
    virtual ~IconProvider();

    static IconProvider *instance();

    Icon add(CateyesIcon *iconHandle);
    void remove(Icon icon);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    static IconProvider *s_instance;
    int m_lastId;
    QHash<int, CateyesIcon *> m_icons;
    QMutex m_mutex;
};

#endif
