#ifndef PROPERTYBASEDTABLEMODEL_H
#define PROPERTYBASEDTABLEMODEL_H

#include <QAbstractTableModel>

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
class PropertyBasedTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	PropertyBasedTableModel(QObject *parent = 0);
	virtual ~PropertyBasedTableModel();

	//! Initializes this instance for use.  If you do not call this method, and use this class, your app will crash.
	//! @param content the domain objects you want to model.  They should all be the same type.  This isnstance does not
	//!			take ownership of content, or the elements in it.
	//! @param model an instance of the same type as in content, this instance is used to create new instances of your
	//!			domain objects by calling the model objects copy constructor.  This instance takes ownership of model.
	//! @param mappings mas an integer positional index to the property.
	void init(QList<QObject *>* content, QObject *model, QMap<int, QString> mappings);

	//Over-rides from QAbstractTableModel
	virtual QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
	virtual bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());

	void moveRowUp(int position);
	void moveRowDown(int position);

private:
	QList<QObject *>* content;
	QMap<int, QString> mappings;
	QObject* modelObject;
};

#endif // PROPERTYBASEDTABLEMODEL_H
