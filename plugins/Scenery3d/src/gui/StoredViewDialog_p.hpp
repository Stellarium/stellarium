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

#ifndef STOREDVIEWDIALOG_P_HPP
#define STOREDVIEWDIALOG_P_HPP

#include "SceneInfo.hpp"

#include <QDebug>
#include <QTextEdit>
#include <QAbstractListModel>
#include <QIcon>

//! A custom QTextEdit subclass that has an editingFinished() signal like QLineEdit.
class CustomTextEdit : public QTextEdit
{
	Q_OBJECT
public:
	CustomTextEdit(QWidget* parent = Q_NULLPTR) : QTextEdit(parent), textChanged(false), trackChange(false)
	{
		connect(this,&QTextEdit::textChanged,this,&CustomTextEdit::handleTextChange);
	}
protected:
	virtual void focusInEvent(QFocusEvent* e) Q_DECL_OVERRIDE
	{
		trackChange = true;
		textChanged = false;
		QTextEdit::focusInEvent(e);
	}

	virtual void focusOutEvent(QFocusEvent *e) Q_DECL_OVERRIDE
	{
		QTextEdit::focusOutEvent(e);
		trackChange = false;

		if(textChanged)
		{
			textChanged = false;
			emit editingFinished();
		}
	}
signals:
	//! Emitted when focus lost and text was changed
	void editingFinished();
private slots:
	void handleTextChange()
	{
		if(trackChange)
			textChanged = true;
	}

private:
	bool textChanged, trackChange;
};

class StoredViewModel : public QAbstractListModel
{
	Q_OBJECT
public:
	StoredViewModel(QObject* parent = Q_NULLPTR) : QAbstractListModel(parent)
	{ }

	virtual int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE
	{
		if(parent.isValid())
			return 0;
		return global.size() + user.size();
	}

	virtual QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE
	{
		if(role == Qt::DisplayRole || role == Qt::EditRole)
		{
			return getViewAtIdx(index.row()).label;
		}
		if(role == Qt::DecorationRole)
		{
			if(getViewAtIdx(index.row()).isGlobal)
			{
				//TODO use a proper lock icon or something here
				return QIcon(":/graphicGui/folder.png");
			}
		}
		return QVariant();
	}

	const StoredView& getViewAtIdx(int idx) const
	{
		if(idx >= global.size())
		{
			return user[idx-global.size()];
		}
		else
		{
			return global[idx];
		}
	}

	StoredView& getViewAtIdx(int idx)
	{
		if(idx >= global.size())
		{
			return user[idx-global.size()];
		}
		else
		{
			return global[idx];
		}
	}

	QModelIndex addUserView(const StoredView& v)
	{
		int idx = global.size() + user.size();
		beginInsertRows(QModelIndex(),idx,idx);
		user.append(v);
		endInsertRows();
		persistUserViews();
		return index(idx);
	}

	void deleteView(int idx)
	{
		const StoredView& v = getViewAtIdx(idx);
		if(!v.isGlobal)
		{
			int useridx = idx - global.size();

			beginRemoveRows(QModelIndex(),idx,idx);
			user.removeAt(useridx);
			endRemoveRows();

			persistUserViews();
		}
		else
		{
			qWarning()<<"[StoredViewDialog] Cannot delete global view";
		}
	}

	void persistUserViews()
	{
		qDebug()<<"[StoredViewDialog] Persisting user views...";
		StoredView::saveUserViews(currentScene,user);
	}

	void updatedAtIdx(int idx)
	{
		QModelIndex mIdx = index(idx);
		persistUserViews();
		emit dataChanged(mIdx,mIdx);
	}

	SceneInfo getScene() { return currentScene; }

public slots:
	void setScene(const SceneInfo& scene)
	{
		qDebug()<<"[StoredViewDialog] Loading stored views for"<<scene.name;
		this->currentScene = scene;
		resetData(StoredView::getGlobalViewsForScene(currentScene),StoredView::getUserViewsForScene(currentScene));
		qDebug()<<"[StoredViewDialog]"<<rowCount(QModelIndex())<<"entries loaded";
	}

private:
	void resetData(const StoredViewList& global, const StoredViewList& user)
	{
		this->beginResetModel();
		this->global = global;
		this->user = user;
		this->endResetModel();
	}

	SceneInfo currentScene;
	StoredViewList global, user;
};


#endif
