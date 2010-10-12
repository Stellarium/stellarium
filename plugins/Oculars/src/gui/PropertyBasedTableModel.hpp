#ifndef PROPERTYBASEDTABLEMODEL_H
#define PROPERTYBASEDTABLEMODEL_H

#include <QAbstractTableModel>

class PropertyBasedTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
	 explicit PropertyBasedTableModel(const QList<QObject *> content,
												 const QMap<int, QString> mappings,
												 const QObject* modelObject,
												 QObject *parent = 0);
	 virtual ~PropertyBasedTableModel();

signals:

public slots:

private:
	QList<QObject *> content;
	QMap<int, QString> mappings;
	QObject* modelObject;
};

#endif // PROPERTYBASEDTABLEMODEL_H
