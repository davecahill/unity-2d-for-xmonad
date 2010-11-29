#ifndef WINDOWINFO_H
#define WINDOWINFO_H

#include <QObject>
#include <QPoint>
#include <QSize>
#include <QVariant>
#include <QX11Info>
#include <QtDeclarative/qdeclarative.h>

#include <X11/Xlib.h>

class BamfWindow;
class BamfApplication;

class WindowInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant xid READ xid WRITE setXid NOTIFY xidChanged)
    Q_PROPERTY(QPoint location READ location NOTIFY locationChanged)
    Q_PROPERTY(QSize size READ size NOTIFY sizeChanged)
    Q_PROPERTY(int z READ z NOTIFY zChanged)
    Q_PROPERTY(QString appName READ appName NOTIFY appNameChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged)

public:
    explicit WindowInfo(Window xid = 0, QObject *parent = 0);

    QVariant xid() const;
    void setXid(QVariant varXid);

    QPoint location() const;
    QSize size() const;
    int z() const;
    QString appName() const;
    QString title() const;
    QString icon() const;

signals:
    void windowChanged(BamfWindow *window);
    void xidChanged(QVariant xid);
    void sizeChanged(QSize size);
    void locationChanged(QPoint location);
    void zChanged(int z);
    void appNameChanged(QString appName);
    void titleChanged(QString title);
    void iconChanged(QString icon);

private:
    void fromXid(Window xid);
    bool geometry(Window xid, QSize *size, QPoint *position, int *z) const;

private:
    BamfWindow *m_bamfWindow;
    BamfApplication *m_bamfApplication;
    QPixmap *shot;
};

QML_DECLARE_TYPE(WindowInfo)

#endif // WINDOWINFO_H
