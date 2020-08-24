#pragma once

#include "dringctrl.h"

#include <QtCore/QThread>

class Jamictl : public QObject
{
    Q_OBJECT
public:
    Jamictl(QObject* parent = nullptr);
    ~Jamictl();

private:
    int chooseAccount();
    void createAccount();

    QThread thread;
    Dringctrl dringctrl;

public slots:
    void run();
    void mainLoop();
    void exit();
signals:
    void finished();
};
