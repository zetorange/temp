#ifndef CATEYESQML_PROCESS_H
#define CATEYESQML_PROCESS_H

#include <cateyes-core.h>

#include "iconprovider.h"

#include <QObject>

class Process : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Process)
    Q_PROPERTY(unsigned int pid READ pid CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QUrl smallIcon READ smallIcon CONSTANT)
    Q_PROPERTY(QUrl largeIcon READ largeIcon CONSTANT)

public:
    explicit Process(CateyesProcess *handle, QObject *parent = nullptr);
    ~Process();

    unsigned int pid() const { return m_pid; }
    QString name() const { return m_name; }
    bool hasIcons() const { return m_smallIcon.isValid(); }
    QUrl smallIcon() const { return m_smallIcon.url(); }
    QUrl largeIcon() const { return m_largeIcon.url(); }

private:
    unsigned int m_pid;
    QString m_name;
    Icon m_smallIcon;
    Icon m_largeIcon;
};

#endif
