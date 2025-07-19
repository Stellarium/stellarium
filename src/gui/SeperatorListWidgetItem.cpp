#include "SeperatorListWidgetItem.hpp"


SeperatorListWidgetItem::SeperatorListWidgetItem(const QString &text, QListWidget *parent)
	: QListWidgetItem(text)
{
	//setText("Gruppierungs-Breaker");
	setFlags(Qt::NoItemFlags);
	setForeground(QBrush(Qt::white));
	//setBackground(QBrush(Qt::gray));
	setTextAlignment(Qt::AlignCenter);
}
