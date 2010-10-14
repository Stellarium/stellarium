#ifndef PROPERTYBASEDTABLEMODEL_H
#define PROPERTYBASEDTABLEMODEL_H

#include <QAbstractTableModel>

template <class T>
class PropertyBasedTableModel : public QAbstractTableModel
{
public:
	explicit PropertyBasedTableModel(QList<T> content,
												T &modelObject,
												QObject *parent = 0);
	explicit PropertyBasedTableModel(QObject *parent = 0);
	virtual ~PropertyBasedTableModel();

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
	bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex());
	bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());

private:
	QList<T> content;
	QMap<int, QString> mappings;
	T* modelObject;
};

#endif // PROPERTYBASEDTABLEMODEL_H
