/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef SCENERY3DDIALOG_P_HPP
#define SCENERY3DDIALOG_P_HPP

#include "S3DEnum.hpp"
#include "Scenery3d.hpp"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include <QAbstractListModel>

class CubemapModeListModel : public QAbstractListModel
{
	Q_OBJECT

private:
	Scenery3d* mgr;

public:
	CubemapModeListModel(QObject* parent = Q_NULLPTR) : QAbstractListModel(parent)
	{
		mgr = GETSTELMODULE(Scenery3d);
		Q_ASSERT(mgr);
	}

	virtual int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE
	{
		Q_UNUSED(parent);
		if(mgr->getIsANGLE())
		{
			return 1;
		}
		if(!mgr->getIsGeometryShaderSupported())
		{
			return 2;
		}
		return 3;
	}

	virtual QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE
	{
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			switch (index.row())
			{
				case S3DEnum::CM_TEXTURES:
					return QVariant(QString(q_("6 Textures")));
				case S3DEnum::CM_CUBEMAP:
					return QVariant(QString(q_("Cubemap")));
				case S3DEnum::CM_CUBEMAP_GSACCEL:
					return QVariant(QString(q_("Geometry shader")));
			}
		}
		return QVariant();
	}
};


#endif
