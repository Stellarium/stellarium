#ifndef TESTTELESCOPECONTROL_HPP
#define TESTTELESCOPECONTROL_HPP

#include <QtTest/QtTest>
#include "common/TelescopeControl.hpp"

class TestTelescopeControl : public QObject
{
    Q_OBJECT

private slots:
    void instantiate();

private:
    //TelescopeControl telescopeControl;
};

#endif // TESTTELESCOPECONTROL_HPP
