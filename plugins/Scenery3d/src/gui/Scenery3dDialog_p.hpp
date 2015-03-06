#ifndef _SCENERY3DDIALOG_P_HPP_
#define _SCENERY3DDIALOG_P_HPP_

#include "S3DEnum.hpp"
#include "Scenery3dMgr.hpp"

#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "StelTranslator.hpp"

#include <QAbstractListModel>

class CubemapModeListModel : public QAbstractListModel
{
	Q_OBJECT
private:
	Scenery3dMgr* mgr;
public:


	CubemapModeListModel(QObject* parent = NULL) : QAbstractListModel(parent)
	{
		mgr = GETSTELMODULE(Scenery3dMgr);
		Q_ASSERT(mgr);
	}

	int rowCount(const QModelIndex &parent) const
	{
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

	QVariant data(const QModelIndex &index, int role) const
	{
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			switch (index.row())
			{
				case S3DEnum::CM_TEXTURES:
					return QVariant(QString(N_("6 Textures")));
				case S3DEnum::CM_CUBEMAP:
					return QVariant(QString(N_("Cubemap")));
				case S3DEnum::CM_CUBEMAP_GSACCEL:
					return QVariant(QString(N_("Geometry shader")));
			}
		}
		return QVariant();
	}
};


#endif
