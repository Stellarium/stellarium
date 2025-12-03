#include "ScmMultiselectionComboBox.hpp"
#include <qapplication.h>

ScmMultiselectionComboBox::ScmMultiselectionComboBox(QWidget *parent)
	: QComboBox(parent)
{
	model = new CheckBoxItemModel(this);
	setModel(model);
	setItemDelegate(new CheckBoxStyledItemDelegate(this));

	setEditable(true);

	QLineEdit *lineEdit = new QLineEdit(this);
	lineEdit->setReadOnly(true);
	QPalette palette = QApplication::palette();
	palette.setBrush(QPalette::Base, palette.button());
	lineEdit->setPalette(palette);
	setLineEdit(lineEdit);
	lineEdit->installEventFilter(this);

	view()->viewport()->installEventFilter(this);

	connect(model, &QStandardItemModel::rowsInserted, this, [this](const QModelIndex &, int, int) {updateText();});
	connect(model, &QStandardItemModel::rowsRemoved, this, [this](const QModelIndex &, int, int) {updateText();});
	connect(model, &QStandardItemModel::dataChanged, this, [this](const QModelIndex &, const QModelIndex &, const QVector<int> &) {updateText();});
}

ScmMultiselectionComboBox::~ScmMultiselectionComboBox()
{
	delete model;
}

void ScmMultiselectionComboBox::setDefaultText(const QString &text)
{
	defaultText = text;
	updateText();
}

void ScmMultiselectionComboBox::addItem(const QString &label, const QVariant &data, const QString &toolTip)
{
	QComboBox::addItem(label, data);
	setItemData(count() - 1, Qt::Unchecked, Qt::CheckStateRole);
	setItemData(count() - 1, toolTip, Qt::ToolTipRole);
}

void ScmMultiselectionComboBox::reset()
{
	// set all items to unchecked
	for (int index = 0; index < count(); index++)
	{
		setItemData(index, Qt::Unchecked, Qt::CheckStateRole);
	}
	updateCheckedItems();
}

bool ScmMultiselectionComboBox::eventFilter(QObject *object, QEvent *event)
{
	if (object == lineEdit())
	{
		if (event->type() == QEvent::MouseButtonPress && object == lineEdit())
		{
			skipPopupHide = true;
			showPopup();
		}
		else if (event->type() == QEvent::MouseButtonRelease && object == lineEdit())
		{
			hidePopup();
		}
	}
	else if ((event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) && object == view()->viewport())
	{
		skipPopupHide = true;

		if (event->type() == QEvent::MouseButtonRelease)
		{
			const QModelIndex index = view()->indexAt( static_cast<QMouseEvent *>( event )->pos() );
			if ( index.isValid() && model->item(index.row())->isEnabled())
			{
				QStandardItem *item = model->itemFromIndex(index);
				item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
				updateCheckedItems();
			}

			return true;
		}
	}

	return QComboBox::eventFilter(object, event);
}

void ScmMultiselectionComboBox::hidePopup()
{
	if (!skipPopupHide)
	{
		QComboBox::hidePopup();
	}
	skipPopupHide = false;
}

void ScmMultiselectionComboBox::updateText()
{
	QString text;

	const QStringList items = checkedItems();
	if ( items.isEmpty() )
	{
		text = defaultText;
	}
	else
	{
		text = items.join(", ");
	}

	const QRect rect = lineEdit()->rect();
	const QFontMetrics fontMetrics( font() );
	text = fontMetrics.elidedText(text, Qt::ElideRight, rect.width());
	setEditText(text);
}

void ScmMultiselectionComboBox::updateCheckedItems()
{
	const QStringList items = checkedItems();
	updateText();
	emit checkedItemsChanged(items);
}

QStringList ScmMultiselectionComboBox::checkedItems() const
{
	QStringList items;

	if (auto *currentModel = model)
	{
		const QModelIndex index = model->index(0, modelColumn(), rootModelIndex());
		const QModelIndexList indexes = model->match(index, Qt::CheckStateRole, Qt::Checked, -1, Qt::MatchExactly);
		for (const QModelIndex &index : indexes)
		{
			items.append(index.data().toString());
		}
	}

	return items;
}

QVariantList ScmMultiselectionComboBox::checkedItemsData() const
{
	QVariantList data;

	if (auto *lModel = model)
	{
		const QModelIndex index = lModel->index(0, modelColumn(), rootModelIndex());
		const QModelIndexList indexes = lModel->match(index, Qt::CheckStateRole, Qt::Checked, -1, Qt::MatchExactly);
		for (const QModelIndex &index : indexes)
		{
			data.append(index.data(Qt::UserRole));
		}
	}

	return data;
}

void ScmMultiselectionComboBox::setItemEnabledState(const QString &itemName, bool enabledState, bool invertSelection)
{
	for (int index = 0; index < count(); index++)
	{
		bool itemFound;
		if (invertSelection)
		{
			itemFound = model->indexFromItem(model->item(index)).data().toString() != itemName;
		}
		else
		{
			itemFound = model->indexFromItem(model->item(index)).data().toString() == itemName;
		}

		if (itemFound)
		{
			model->item(index)->setEnabled(enabledState);
		}
	}
}

// ==================  CheckBoxStyledItemDelegate  ==================

CheckBoxStyledItemDelegate::CheckBoxStyledItemDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
}

void CheckBoxStyledItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItem & styleOption = const_cast<QStyleOptionViewItem &>(option);
	styleOption.showDecorationSelected = false;
	QStyledItemDelegate::paint(painter, styleOption, index);
}

// ==================  CheckBoxItemModel  ==================

CheckBoxItemModel::CheckBoxItemModel(QObject *parent)
	: QStandardItemModel(0, 1, parent)
{
}

QVariant CheckBoxItemModel::data(const QModelIndex &index, int role) const
{
	QVariant value = QStandardItemModel::data(index, role);

	if (index.isValid() && role == Qt::CheckStateRole && !value.isValid())
	{
		value = Qt::Unchecked;
	}

	return value;
}

bool CheckBoxItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	const bool ok = QStandardItemModel::setData(index, value, role);

	if (ok && role == Qt::CheckStateRole)
	{
		emit itemCheckStateChanged(index);
	}

	emit dataChanged(index, index);
	return ok;
}
