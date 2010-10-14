#include "PropertyBasedTableModel.hpp"

template <class T>
PropertyBasedTableModel<T>::PropertyBasedTableModel(QObject *parent)
	: QAbstractTableModel(parent)
{
}

template <class T>
PropertyBasedTableModel<T>::PropertyBasedTableModel(QList<T> content,
																 T &modelObject,
																 QObject *parent)
	: QAbstractTableModel(parent)
{
	this->content = content;
	this->mappings = modelObject->propertyMap();
	this->modelObject = modelObject;
}
template <class T>
PropertyBasedTableModel<T>::~PropertyBasedTableModel()
{
	delete modelObject;
	modelObject = NULL;
}

template <class T>
int PropertyBasedTableModel<T>::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return content.size();
}

template <class T>
int PropertyBasedTableModel<T>::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return mappings.size();
}

template <class T>
QVariant PropertyBasedTableModel<T>::data(const QModelIndex &index, int role) const
{
	QVariant data;
	if (role == Qt::DisplayRole
		 && index.isValid()
		 && index.row() < content.size()
		 && index.row() >= 0
		 && index.column() < mappings.size()
		 && index.column() > 0){
			T *object = content.at(index.row());
			data = object->property(mappings[index.column()]);

	}
	return data;
}

template <class T>
bool PropertyBasedTableModel<T>::insertRows(int position, int rows, const QModelIndex &index)
{
	Q_UNUSED(index);
	beginInsertRows(QModelIndex(), position, position+rows-1);

	for (int row=0; row < rows; row++) {
		T* newInstance = modelObject->metaObject()->newInstance(modelObject);
		content.insert(position, newInstance);
	}

	endInsertRows();
	return true;
}

template <class T>
bool PropertyBasedTableModel<T>::removeRows(int position, int rows, const QModelIndex &index)
{
	Q_UNUSED(index);
	beginRemoveRows(QModelIndex(), position, position+rows-1);

	for (int row=0; row < rows; ++row) {
		content.removeAt(position);
	}

	endRemoveRows();
	return true;
}

template <class T>
bool PropertyBasedTableModel<T>::setData(const QModelIndex &index, const QVariant &value, int role)
{
	bool changeMade = false;
	if (index.isValid() && role == Qt::EditRole && index.column() < mappings.size()) {
		T* object = content.at(index.row());
		object->setProperty(mappings[index.column()], value);
		emit(dataChanged(index, index));

		changeMade = true;
	}

	return changeMade;
}

template <class T>
Qt::ItemFlags PropertyBasedTableModel<T>::flags(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return Qt::ItemIsEnabled;
	}

	return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

