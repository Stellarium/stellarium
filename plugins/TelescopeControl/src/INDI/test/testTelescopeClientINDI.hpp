#ifndef TESTTELESCOPECLIENTINDI_HPP
#define TESTTELESCOPECLIENTINDI_HPP

#include <QTest>

class testTelescopeClientINDI : public QObject
{
    Q_OBJECT

private slots:
	void isConnectedInitialValue();
};

#endif // TESTTELESCOPECLIENTINDI_HPP
