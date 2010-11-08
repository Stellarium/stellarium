#ifndef PROPERTYBASEDTABLEMODEL_H
#define PROPERTYBASEDTABLEMODEL_H

#include <QAbstractTableModel>

class PropertyBasedTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	PropertyBasedTableModel(QObject *parent = 0);
	virtual ~PropertyBasedTableModel();

	void init(QList<QObject *>* content, QObject *model, QMap<int, QString> mappings);

	//Over-rides from QAbstractTableModel
	virtual QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
	virtual bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());

private:
	QList<QObject *>* content;
	QMap<int, QString> mappings;
	QObject* modelObject;
};

#endif // PROPERTYBASEDTABLEMODEL_H
