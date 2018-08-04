/*
 * Stellarium Remote Control plugin
 * Copyright (C) 2015 Florian Schaukowitsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef OBJECTSERVICE_HPP
#define OBJECTSERVICE_HPP

#include "AbstractAPIService.hpp"
#include "StelObjectType.hpp"

#include <QStringList>

class StelCore;
class StelObjectMgr;

//! @ingroup remoteControl
//! Provides operations to look up objects in the Stellarium catalogs
//!
//! @see \ref rcObjectService
class ObjectService : public AbstractAPIService
{
	Q_OBJECT
public:
	ObjectService(QObject* parent = Q_NULLPTR);

	virtual QLatin1String getPath() const Q_DECL_OVERRIDE { return QLatin1String("objects"); }
	//! @brief Implements the HTTP GET method
	//! @see \ref rcObjectServiceGET
	virtual void get(const QByteArray& operation,const APIParameters& parameters, APIServiceResponse& response) Q_DECL_OVERRIDE;

private slots:
	//! Executed in Stellarium main thread to avoid multiple QMetaObject::invoke calls
	QStringList performSearch(const QString& text);
	//! Wrapper around SearchDialog::substituteGreek
	QString substituteGreek(const QString& text);

	//! Wrapper around StelObjectMgr::find...
	StelObjectP findObject(const QString& name);

	//! Wrapper around obj->getInfoString
	QString getInfoString(const StelObjectP obj);
private:
	StelCore* core;
	StelObjectMgr* objMgr;
	bool useStartOfWords;
};



#endif
