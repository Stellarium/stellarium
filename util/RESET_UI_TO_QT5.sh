#/bin/bash

#
# Author and Copyright: Georg Zotti, 2024
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
#

# The Qt constant names used in UI XML definition of user interfaces have been extended in Qt6.8.  
# This command resets the Qt constants from Qt6.8 to remain compatible with Qt5 or earlier versions of Qt6.

# Before committing  some 'myGui'.ui file into our git repository, rename it to 'myGui'_Qt68.ui, then run 
# ./RESET_UI_TO_QT5.sh 'myGui'_Qt68.ui > 'myGui'.ui
# Then check in git gui (or whatever you use) for remaining changed constant names and extend this tool where needed.

sed -e 's/Qt::FocusPolicy::NoFocus/Qt::NoFocus/g' \
	-e 's/Qt::ScrollBarPolicy::ScrollBarAlwaysOff/Qt::ScrollBarAlwaysOff/g' \
	-e 's/Qt::ScrollBarPolicy::ScrollBarAsNeeded/Qt::ScrollBarAsNeeded/g' \
	-e 's/Qt::TextFormat::RichText/Qt::RichText/g' \
	-e 's/Qt::TextFormat::PlainText/Qt::PlainText/g' \
	-e 's/Qt::Orientation::Vertical/Qt::Vertical/g' \
	-e 's/Qt::Orientation::Horizontal/Qt::Horizontal/g' \
	-e 's/Qt::AlignmentFlag::AlignLeading/Qt::AlignLeading/g' \
	-e 's/Qt::AlignmentFlag::AlignLeft/Qt::AlignLeft/g' \
	-e 's/Qt::AlignmentFlag::AlignCenter/Qt::AlignCenter/g' \
	-e 's/Qt::AlignmentFlag::AlignRight/Qt::AlignRight/g' \
	-e 's/Qt::AlignmentFlag::AlignTrailing/Qt::AlignTrailing/g' \
	-e 's/Qt::AlignmentFlag::AlignVCenter/Qt::AlignVCenter/g' \
	-e 's/QAbstractItemView::SelectionMode::SingleSelection/QAbstractItemView::SingleSelection/g' \
	-e 's/QAbstractSpinBox::ButtonSymbols::UpDownArrows/QAbstractSpinBox::UpDownArrows/g' \
	-e 's/QComboBox::InsertPolicy::NoInsert/QComboBox::NoInsert/g' \
	-e 's/QComboBox::SizeAdjustPolicy::AdjustToContentsOnFirstShow/QComboBox::AdjustToContentsOnFirstShow/g' \
	-e 's/QFrame::Shadow::Raised/QFrame::Raised/g' \
    -e 's/QFrame::Shape::StyledPanel/QFrame::StyledPanel/g' \
	-e 's/QListView::Movement::Static/QListView::Static/g' \
	-e 's/QListView::Flow::LeftToRight/QListView::LeftToRight/g' \
	-e 's/QListView::ResizeMode::Adjust/QListView::Adjust/g' \
	-e 's/QListView::LayoutMode::SinglePass/QListView::SinglePass/g' \
	-e 's/QListView::ViewMode::IconMode/QListView::IconMode/g' \
	-e 's/QSlider::TickPosition::NoTicks/QSlider::NoTicks/g' \
	-e 's/Qt::TextInteractionFlag::TextSelectableByMouse/Qt::TextSelectableByMouse/g' \
	-e 's/Qt::TextInteractionFlag::NoTextInteraction/Qt::NoTextInteraction/g' \
	   $1