#include "SeparatorListWidgetItem.hpp"


SeparatorListWidgetItem::SeparatorListWidgetItem(const QString &text, QListWidget *parent)
	: QListWidgetItem(parent, 1318)
{
	setFlags(Qt::NoItemFlags);
	setTextAlignment(Qt::AlignCenter);

	setText("⸻ " + text + " ⸻");

	setForeground(QBrush(Qt::white));
	//setBackground(QBrush(Qt::gray));
}
