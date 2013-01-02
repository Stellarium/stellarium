#include "MenuListModel.hpp"
#include <QAction>

Q_DECLARE_METATYPE(QAction*)

MenuListModel::MenuListModel(QObject *parent) :
    QAbstractListModel(parent),
    headerText("")
{
    setRoleNames(getRoleNames());
}

MenuListModel::MenuListModel(QObject *parent, QString header) :
    QAbstractListModel(parent),
    headerText(header)
{
    setRoleNames(getRoleNames());
}

const QHash<int, QByteArray> MenuListModel::getRoleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ActionRole] = "action";
    roles[ImageSourceRole] = "imageSource";
    return roles;
}

int MenuListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return listData.count();
}

QVariant MenuListModel::data(const QModelIndex &index, int role) const
{
    switch(role)
    {
    case ActionRole:
        return QVariant::fromValue(listData[index.row()].action);
        break;

    case ImageSourceRole:
        return listData[index.row()].imageSource;
        break;

    default:
        return QVariant();
        break;
    }
}

void MenuListModel::addRow(QAction* action, QString imageSource)
{
    beginInsertRows(QModelIndex(), listData.count(), listData.count());

    MenuListElement el;
    el.action = action;
    el.imageSource = imageSource;

    listData.append(el);

    endInsertRows();
}
