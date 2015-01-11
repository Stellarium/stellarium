#ifndef _SCENERY3DDIALOG_P_HPP_
#define _SCENERY3DDIALOG_P_HPP_

#include "S3DEnum.hpp"
#include "StelTranslator.hpp"
#include <QAbstractListModel>

class CubemapModeListModel : public QAbstractListModel
{
	Q_OBJECT

private:
	bool gsSupported;
public:


	CubemapModeListModel(QObject* parent = NULL) : QAbstractListModel(parent),gsSupported(false)
	{}

	void setGSSupported(bool supported)
	{
		gsSupported = supported;
		QModelIndex idx = index(S3DEnum::CUBEMAP_GSACCEL);
		emit dataChanged(idx,idx);
	}

	int rowCount(const QModelIndex &parent) const
	{
		return 3;
	}

	QVariant data(const QModelIndex &index, int role) const
	{
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			switch (index.row())
			{
				case S3DEnum::TEXTURES:
					return QVariant(QString(N_("6 Textures")));
				case S3DEnum::CUBEMAP:
					return QVariant(QString(N_("Cubemap")));
				case S3DEnum::CUBEMAP_GSACCEL:
					return QVariant(QString(N_("Geometry shader")));
			}
		}
		return QVariant();
	}

	Qt::ItemFlags flags(const QModelIndex &index) const
	{
		//disable the third item depending on geometry shader support flag
		Qt::ItemFlags parFlags = QAbstractListModel::flags(index);

		if(index.row() == S3DEnum::CUBEMAP_GSACCEL)
		{
			if(gsSupported)
			{
				//set flag, may not be necessary
				parFlags|= Qt::ItemIsEnabled;
			}
			else
			{
				//remove flag
				parFlags&= ~Qt::ItemIsEnabled;
			}
		}
		return parFlags;
	}
};


#endif
