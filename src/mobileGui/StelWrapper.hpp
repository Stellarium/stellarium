#ifndef STELWRAPPER_HPP
#define STELWRAPPER_HPP

#include <QObject>
#include "StelObject.hpp"

//! @class Allows QML access to information about the world
class StelWrapper : public QObject
{
    Q_OBJECT
public:
	explicit StelWrapper(QObject *parent = 0);

	//! Get a pointer on the info panel used to display selected object info
	virtual void setInfoTextFilters(const StelObject::InfoStringGroup& aflags);
	virtual const StelObject::InfoStringGroup& getInfoTextFilters() const;

	/* Q_INVOKABLE methods (accessible from QML) */
	Q_INVOKABLE QString getInfoText();
	Q_INVOKABLE void toggleAllInfo();
	Q_INVOKABLE double getCurrentFov();

signals:

public slots:
	void setFov(qreal f);

private:
	StelObject::InfoStringGroup infoTextFilters;

};

#endif // INFOWRAPPER_HPP
