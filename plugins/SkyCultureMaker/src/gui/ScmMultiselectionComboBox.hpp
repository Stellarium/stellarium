#ifndef SCMMULTISELECTIONCOMBOBOX_HPP
#define SCMMULTISELECTIONCOMBOBOX_HPP

#include <qpainter.h>
#include <QWidget>
#include <QComboBox>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QEvent>
#include <QMouseEvent>
#include <QStyledItemDelegate>
#include <QListView>

//! @class ScmMultiselectionComboBox
//! QComboBox of CheckBoxes that allows for selection of multiple elements.
// amalgamation of: https://gist.github.com/mistic100/c3b7f3eabc65309687153fe3e0a9a720 and https://github.com/qgis/QGIS/blob/master/src/gui/qgscheckablecombobox.h

class CheckBoxItemModel : public QStandardItemModel
{
	Q_OBJECT

public:
	CheckBoxItemModel(QObject *parent = nullptr);

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

signals:
	void itemCheckStateChanged(const QModelIndex &index);
};

class ScmMultiselectionComboBox : public QComboBox
{
	Q_OBJECT

public:

	ScmMultiselectionComboBox(QWidget *parent);
	~ScmMultiselectionComboBox();

	void hidePopup() override;
	void setDefaultText(const QString &text);
	void addItem(const QString &label, const QVariant &data, const QString &toolTip);
	void reset();
	QStringList checkedItems() const;
	QVariantList checkedItemsData() const;
	void setItemEnabledState(const QString &itemName, bool enabledState, bool invertSelection = false);


signals:

	void checkedItemsChanged(const QStringList &items);

protected:
	bool eventFilter(QObject *object, QEvent *event) override;

private:
	bool skipPopupHide;

	CheckBoxItemModel *model;
	QString defaultText;

	void updateText();
	void updateCheckedItems();
};

class CheckBoxStyledItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	CheckBoxStyledItemDelegate(QObject *parent);

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};



#endif // SCMMULTISELECTIONCOMBOBOX_HPP
