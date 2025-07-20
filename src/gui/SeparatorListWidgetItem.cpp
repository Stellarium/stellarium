#include "SeparatorListWidgetItem.hpp"


SeparatorListWidgetItem::SeparatorListWidgetItem(const QString &text, QListWidget *parent)
	: QListWidgetItem(text, parent, 1318)
{
	//setText("Gruppierungs-Breaker");
	setFlags(Qt::NoItemFlags);
	setForeground(QBrush(Qt::white));
	//setBackground(QBrush(Qt::gray));
	setTextAlignment(Qt::AlignCenter);

}
