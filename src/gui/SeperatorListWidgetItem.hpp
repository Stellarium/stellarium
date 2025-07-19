#ifndef SEPERATORLISTWIDGETITEM_HPP
#define SEPERATORLISTWIDGETITEM_HPP

#include <QListWidgetItem>


//! @class SeperatorListWidgetItem
//! Special Seperator element for a QListWidget
class SeperatorListWidgetItem : public QListWidgetItem
{

public:
	SeperatorListWidgetItem(const QString &text, QListWidget *parent=nullptr);

private:

private slots:

};

#endif // SEPERATORLISTWIDGETITEM_HPP
