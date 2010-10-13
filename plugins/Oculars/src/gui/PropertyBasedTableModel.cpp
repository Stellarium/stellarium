#include "PropertyBasedTableModel.hpp"

PropertyBasedTableModel::PropertyBasedTableModel(QList<QObject *> content,
																 QMap<int, QString> mappings,
																 QObject* modelObject,
																 QObject *parent)
	: QAbstractTableModel(parent)
{
	this->content = content;
	this->mappings = mappings;
	this->modelObject = modelObject;
}

PropertyBasedTableModel::~PropertyBasedTableModel()
{
	delete modelObject;
	modelObject = NULL;
}

int PropertyBasedTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return content.size();
}

int PropertyBasedTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return mappings.size();
}

QVariant PropertyBasedTableModel::data(const QModelIndex &index, int role) const
{
	QVariant data;
	if (role == Qt::DisplayRole
		 && index.isValid()
		 && index.row() < content.size()
		 && index.row() >= 0
		 && index.column() < mappings.size()
		 && index.column() > 0){
			QObject *object = content.at(index.row());
			data = object->property(mappings[index.column()]);

	}
	return data;
}

bool PropertyBasedTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
	Q_UNUSED(index);
	beginInsertRows(QModelIndex(), position, position+rows-1);

	for (int row=0; row < rows; row++) {
		QObject* newInstance = modelObject->metaObject()->newInstance(modelObject);
		content.insert(position, newInstance);
	}

	endInsertRows();
	return true;
}

bool PropertyBasedTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
	Q_UNUSED(index);
	beginRemoveRows(QModelIndex(), position, position+rows-1);

	for (int row=0; row < rows; ++row) {
		content.removeAt(position);
	}

	endRemoveRows();
	return true;
}

bool PropertyBasedTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	bool changeMade = false;
	if (index.isValid() && role == Qt::EditRole && index.column() < mappings.size()) {
		QObject* object = content.at(index.row());
		object->setProperty(mappings[index.column()], value);
		emit(dataChanged(index, index));

		changeMade = true;
	}

	return changeMade;
}

Qt::ItemFlags PropertyBasedTableModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return Qt::ItemIsEnabled;
	}

	return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

QList< QPair<QString, QString> > PropertyBasedTableModel::getList()
{
	return listOfPairs;
}
