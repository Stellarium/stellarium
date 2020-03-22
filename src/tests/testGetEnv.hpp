#ifndef TESTGETENV_HPP
#define TESTGETENV_HPP

#include <QObject>
#include <QtTest>

class TestGetEnv : public QObject
{
    Q_OBJECT
private slots:
    void testGetExistingEnv();
    void testGetMissingEnv();
};

#endif // TESTGETENV_HPP
