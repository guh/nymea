#ifndef USERAUTORIZER_H
#define USERAUTORIZER_H

#include <QObject>

class UserAutorizer : public QObject
{
    Q_OBJECT
public:
    explicit UserAutorizer(QObject *parent = nullptr);

signals:

};

#endif // USERAUTORIZER_H
