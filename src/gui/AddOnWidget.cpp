/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include <QCheckBox>
#include <QFileInfo>
#include <QPainter>
#include <QStringBuilder>

#include "AddOnTableView.hpp"
#include "AddOnWidget.hpp"
#include "ui_addonWidget.h"

AddOnWidget::AddOnWidget(QWidget* parent, int row, AddOn* addon)
	: QWidget(parent)
	, m_iRow(row)
	, ui(new Ui_AddOnWidget)
{
	ui->setupUi(this);
	connect(ui->listWidget, SIGNAL(itemChanged(QListWidgetItem*)),
		this, SLOT(slotItemChanged(QListWidgetItem*)));
	connect(((AddOnTableView*)parent), SIGNAL(rowChecked(int, bool)),
		this, SLOT(slotCheckAllFiles(int, bool)));

	// Authors (name, email and url)
	QStringList authors;
	foreach (AddOn::Authors a, addon->getAuthors()) {
		QString author = a.name;
		if (!a.url.isEmpty())
		{
			author = author % " <" % a.url % ">";
		}
		if (!a.email.isEmpty())
		{
			author = author % " <" % a.email % ">";
		}
		authors.append(author);
	}
	ui->txtAuthor->setText(authors.join("\n"));

	// Description
	ui->txtDescription->setText(addon->getDescription());

	// License (name and url)
	QString licensetxt = addon->getLicenseName();
	if (!addon->getLicenseURL().isEmpty())
	{
		licensetxt += " <" % addon->getLicenseURL() % ">";
	}
	ui->txtLicense->setText(licensetxt);

	// Download Size
	ui->txtSize->setText(fileSizeToString(addon->getDownloadSize()));
/*
	// Thumbnail
	QString thumbnailDir = StelApp::getInstance().getStelAddOnMgr().getThumbnailDir();
	QPixmap thumbnail(thumbnailDir % addon->getAddOnId() % ".jpg");
	if (thumbnail.isNull())
	{
		ui->thumbnail->setVisible(false);
	}
	else
	{
		thumbnail = thumbnail.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		ui->thumbnail->setPixmap(thumbnail);
		ui->thumbnail->resize(thumbnail.size());
	}
*/
	// List of files - for now, applicable only for textures
	ui->listWidget->clear();
	m_sSelectedFilesToInstall.clear();
	m_sSelectedFilesToRemove.clear();
	ui->listWidget->setVisible(false);

	if (addon->getType() == AddOn::TEXTURE)
	{
		if (addon->getAllTextures().size() > 1) // display list just when it is a texture set
		{
			foreach (QString texture, addon->getAllTextures())
			{
				QListWidgetItem* item = new QListWidgetItem(QFileInfo(texture).fileName(), ui->listWidget);
				item->setToolTip(texture);
				item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
				QCheckBox* parentCheckbox = ((AddOnTableView*)parentWidget())->getCheckBox(m_iRow-1);
				item->setCheckState(parentCheckbox->checkState());
				if (addon->getInstalledFiles().contains(texture))
				{
					item->setText(item->text() % " (installed)");
					item->setTextColor(QColor("green"));
				}
			}
			ui->listWidget->setVisible(true);
		}
	}
}

AddOnWidget::~AddOnWidget()
{
	delete ui;
	ui = NULL;
}

QString AddOnWidget::fileSizeToString(float bytes)
{
	QStringList list;
	list << "KB" << "MB" << "GB" << "TB";
	QStringListIterator i(list);
	QString unit("bytes");
	while(bytes >= 1000.0 && i.hasNext())
	{
	    unit = i.next();
	    bytes /= 1000.0;
	}
	return QString::number(bytes,'f',2) % " " % unit;
}

void AddOnWidget::paintEvent(QPaintEvent*)
{
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AddOnWidget::slotCheckAllFiles(int pRow, bool checked)
{
	if (ui->listWidget->count() && pRow+1 == m_iRow)
	{
		m_sSelectedFilesToInstall.clear();
		m_sSelectedFilesToRemove.clear();
		ui->listWidget->blockSignals(true);
		for (int i=0; i<ui->listWidget->count(); i++)
		{
			ui->listWidget->item(i)->setCheckState(checked?Qt::Checked:Qt::Unchecked);
			slotItemChanged(ui->listWidget->item(i));
		}
		ui->listWidget->blockSignals(false);
	}
}

void AddOnWidget::slotItemChanged(QListWidgetItem *item)
{
	if (ui->listWidget->count())
	{
		QString filePath = item->toolTip();

		if (item->checkState())
		{
			if (item->textColor() == QColor("green")) // installed
			{
				m_sSelectedFilesToRemove.append(filePath);
			}
			else
			{
				m_sSelectedFilesToInstall.append(filePath);
			}
		}
		else
		{
			if (item->textColor() == QColor("green")) // installed
			{
				m_sSelectedFilesToRemove.removeOne(filePath);
			}
			else
			{
				m_sSelectedFilesToInstall.removeOne(filePath);
			}
		}

		int count = m_sSelectedFilesToInstall.size() + m_sSelectedFilesToRemove.size();
		if (count == ui->listWidget->count())
		{
			emit(checkRow(m_iRow-1, 2));  // all checked
		}
		else if (count == 0)
		{
			emit(checkRow(m_iRow-1, 0));  // all unchecked
		}
		else
		{
			emit(checkRow(m_iRow-1, 1));  // partially
		}
	}
}
