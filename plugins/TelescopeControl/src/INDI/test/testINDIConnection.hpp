#ifndef TESTINDICONNECTION_HPP
#define TESTINDICONNECTION_HPP

#include <QTest>

class testINDIConnection : public QObject
{
    Q_OBJECT

private slots:
    void deafultCoordinates();
    void defaultPosition();
    void initialConnectionStatus();
    void setPositionNotConnected();
    void listDevices();
};

#endif // TESTINDICONNECTION_HPP
