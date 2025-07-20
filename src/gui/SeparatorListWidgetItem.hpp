#ifndef SEPARATORLISTWIDGETITEM_HPP
#define SEPARATORLISTWIDGETITEM_HPP

#include <QListWidgetItem>


//! @class SeperatorListWidgetItem
//! Special Seperator element for a QListWidget
class SeparatorListWidgetItem : public QListWidgetItem
{

public:
	SeparatorListWidgetItem(const QString &text, QListWidget *parent=nullptr);

private:

private slots:

};

#endif // SEPARATORLISTWIDGETITEM_HPP
