#include "nymeascript.h"

NymeaScript::NymeaScript(QObject *parent) : QObject(parent)
{

}

void NymeaScript::classBegin()
{

}

void NymeaScript::componentComplete()
{
    emit init();
}

QQmlListProperty<QObject> NymeaScript::children()
{
    return QQmlListProperty<QObject>(this, m_children);
}

int NymeaScript::childrenCount() const
{
    return m_children.count();
}

QObject *NymeaScript::child(int index) const
{
    return m_children.at(index);
}
