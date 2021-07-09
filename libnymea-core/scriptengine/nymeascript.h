#ifndef NYMEASCRIPT_H
#define NYMEASCRIPT_H

#include <QObject>
#include <QQmlParserStatus>
#include <QQmlListProperty>

class NymeaScript : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> children READ children)
    Q_CLASSINFO("DefaultProperty", "children")

public:
    explicit NymeaScript(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    QQmlListProperty<QObject> children();
    int childrenCount() const;
    QObject *child(int index) const;

signals:
    void init();

private:
    QList<QObject*> m_children;

};

#endif // NYMEASCRIPT_H
