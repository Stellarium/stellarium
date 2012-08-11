#include "PropertyBasedTableModel.hpp"
#include <QDebug>

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark instance Methods
#endif
/* ********************************************************************* */
PropertyBasedTableModel::PropertyBasedTableModel(QObject *parent)
	: QAbstractTableModel(parent)
{
}


PropertyBasedTableModel::~PropertyBasedTableModel()
{
	delete modelObject;
	modelObject = NULL;
}

void PropertyBasedTableModel::init(QList<QObject *>* content, QObject *model, QMap<int,QString> mappings)
{
	beginResetModel();
	this->content = content;
	this->modelObject = model;
	this->mappings = mappings;
	endResetModel();
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Model Methods
#endif
/* ********************************************************************* */

int PropertyBasedTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return content->size();
}

int PropertyBasedTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return mappings.size();
}

QVariant PropertyBasedTableModel::data(const QModelIndex &index, int role) const
{
	QVariant data;
	if ((role == Qt::DisplayRole || role == Qt::EditRole)
		 && index.isValid()
		 && index.row() < content->size()
		 && index.row() >= 0
		 && index.column() < mappings.size()
		 && index.column() >= 0){
			QObject *object = content->at(index.row());
			data = object->property(mappings[index.column()].toStdString().c_str());
  }
	return data;
}

bool PropertyBasedTableModel::insertRows(int position, int rows, const QModelIndex &index)
{
	Q_UNUSED(index);
	beginInsertRows(QModelIndex(), position, position + rows - 1);

	for (int row=0; row < rows; row++) {
		QObject* newInstance = modelObject->metaObject()->newInstance(Q_ARG(QObject, *modelObject));
		Q_ASSERT(newInstance != 0);
		content->insert(position, newInstance);
	}

	endInsertRows();
	return true;
}

bool PropertyBasedTableModel::removeRows(int position, int rows, const QModelIndex &index)
{
	Q_UNUSED(index);
	beginRemoveRows(QModelIndex(), position, position + rows - 1);

	for (int row=0; row < rows; ++row) {
		content->removeAt(position);
	}

	endRemoveRows();
	return true;
}

bool PropertyBasedTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	bool changeMade = false;
	if (index.isValid() && role == Qt::EditRole && index.column() < mappings.size()) {
		QObject* object = content->at(index.row());
		object->setProperty(mappings[index.column()].toStdString().c_str(), value);
		emit(QAbstractItemModel::dataChanged(index, index));

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

void PropertyBasedTableModel::moveRowUp(int position)
{
	int count = content->count();
	if (count < 2 || position < 1 || position >= count)
		return;

	beginMoveRows(QModelIndex(), position, position, QModelIndex(), position-1);

	content->move(position, position - 1);

	endMoveRows();
}

void PropertyBasedTableModel::moveRowDown(int position)
{
	int count = content->count();
	if (count < 2 || position < 0 || position > (count - 2))
		return;

	beginMoveRows(QModelIndex(), position, position, QModelIndex(), position+2);

	content->move(position, position + 1);

	endMoveRows();
}

