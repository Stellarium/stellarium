/*
 * Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SeparatorListWidgetItem.hpp"

SeparatorListWidgetItem::SeparatorListWidgetItem(const QString &text, QListWidget *parent)
	: QListWidgetItem(parent, 1318)
{
	setFlags(Qt::NoItemFlags);
	setTextAlignment(Qt::AlignCenter);

	// hacky solution: use three-em dash (U+2E3B) to draw a line
	setText("⸻ " + text + " ⸻");

	setForeground(QBrush(Qt::white));
}
