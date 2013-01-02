#ifndef MENULISTMODEL_H
#define MENULISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVector>

class QAction;

//! @class MenuListModel
//! Provides a C++ model that can be used by the QML ListViews containing lists of QActions (such as PopupMenu)
class MenuListModel : public QAbstractListModel
{
    Q_OBJECT

    struct MenuListElement
    {
        QAction* action;
        QString imageSource;
    };

public:
    enum MenuListRoles
    {
        ActionRole = Qt::UserRole,
        ImageSourceRole
    };

public:
    explicit MenuListModel(QObject *parent = 0);
    MenuListModel(QObject *parent, QString header);

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    void addRow(QAction* action, QString imageSource);

    //call setRoleNames in the constructor plz
    //    use Qt::UserRole, Qt::UserRole+1, ...
    
    //rowCount
    //data
    //headerData

    /*
     *                 ListElement { action: "actionSettings_Dialog"
                               imageSource: "image://mobileGui/settings"
                               overrideActionText: true //if false or omitted, the action's text property is used instead of the "text" property here
                               text: "Settings" }
                               */
    /* Additionally: enum for special types? (Data stored in action). e.g. checkbox, slider, text entry, radio, spinner, date picker, subheadings */
    /* Since we're doing that, might as well add a isReadOnly property */
signals:
    
public slots:

private:
    const QHash<int, QByteArray> getRoleNames() const;

    QString headerText;
    QVector<MenuListElement> listData;
    
};

Q_DECLARE_METATYPE(MenuListModel*)

#endif // MENULISTMODEL_H
