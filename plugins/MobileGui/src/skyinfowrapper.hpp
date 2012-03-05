#ifndef SKYINFOWRAPPER_HPP
#define SKYINFOWRAPPER_HPP

#include <QObject>
#include "../../../src/core/StelObject.hpp"

//! @class Allows QML access to information about the world
class SkyInfoWrapper : public QObject
{
    Q_OBJECT
public:
    explicit SkyInfoWrapper(QObject *parent = 0);

	//! Get a pointer on the info panel used to display selected object info
	virtual void setInfoTextFilters(const StelObject::InfoStringGroup& aflags);
	virtual const StelObject::InfoStringGroup& getInfoTextFilters() const;

	/* Q_INVOKABLE methods (accessible from QML) */
	Q_INVOKABLE QString getInfoText();

signals:

public slots:

private:
	StelObject::InfoStringGroup infoTextFilters;

};

#endif // SKYINFOWRAPPER_HPP
