/*
 * Copyright (C) 2012 Stellarium Team
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

#pragma once

#include <QAbstractTableModel>
#include <QtGlobal>

//! This class provides a table model for just about any QObject.  It it nice, as a table model implementation per
//! class is not required.  It does this by using the Qt meta object system.
//!
//! To use this class, your domain objects basically just need to use properties (for any properties you want to make
//! available to the model), and have a Q_INVOKABLE copy constructor.  Then, when you instantiate an
//! instance, you must call the init methd.  The init method takes the data to model, as well as an instance of your
//! model class (to use as a model for creating new instances), and a map to know the ordering  of the properties to
//! their position (as you want them displayed).
//!
//! @author Timothy Reaves <treaves@silverfieldstech.com>
//! @ref http://doc.qt.nokia.com/latest/properties.html
//! @ingroup oculars
class PropertyBasedTableModel : public QAbstractTableModel
{
   Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
   Q_DISABLE_COPY_MOVE(PropertyBasedTableModel)
#endif
public:
   explicit PropertyBasedTableModel(QObject * parent = Q_NULLPTR);
   ~PropertyBasedTableModel() override;

   //! Initializes this instance for use.  If you do not call this method, and use this class, your app will crash.
   //! @param modelData the domain objects you want to model.  They should all be the same type.  This isnstance does
   //! not
   //!			take ownership of modelData, or the elements in it.
   //! @param model an instance of the same type as in modelData, this instance is used to create new instances of your
   //!			domain objects by calling the model objects copy constructor.  This instance takes ownership of model.
   //! @param properties mas an integer positional index to the property.
   void init(QList<QObject *> * modelData, QObject * model, const QMap<int, QString> & properties);

   // Over-rides from QAbstractTableModel
   auto data(const QModelIndex & item, int role = Qt::DisplayRole) const -> QVariant override;

   auto rowCount(const QModelIndex & parent = QModelIndex()) const -> int override;
   auto columnCount(const QModelIndex & parent = QModelIndex()) const -> int override;

   auto flags(const QModelIndex & index) const -> Qt::ItemFlags override;
   auto insertRows(int position, int rows, const QModelIndex & index = QModelIndex()) -> bool override;
   auto setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) -> bool override;
   auto removeRows(int position, int rows, const QModelIndex & index = QModelIndex()) -> bool override;

   void moveRowUp(int position);
   void moveRowDown(int position);

private:
   QList<QObject *> * content;
   QMap<int, QString> mappings;
   QObject *          modelObject;
};
