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
	ui->txtLicense->setText(addon->getLicenseName() % " <" % addon->getLicenseURL() % ">");

	// Download Size
	ui->txtSize->setText(QString::number(addon->getDownloadSize()) % " KBytes");

	// Thumbnail
	QString thumbnailDir = StelApp::getInstance().getStelAddOnMgr().getThumbnailDir();
	QPixmap thumbnail(thumbnailDir % addon->getInstallId() % ".jpg");
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

	// List of files - for now, applicable only for textures
	ui->listWidget->clear();
	m_sSelectedFilesToInstall.clear();
	m_sSelectedFilesToRemove.clear();
	ui->listWidget->setVisible(false);

	if (parentWidget()->objectName() == "texturesTableView")
	{
		if (addon->getAllTextures().size() > 1) // display list just when it is a texture set
		{
			foreach (QString texture, addon->getAllTextures())
			{
				QListWidgetItem* item = new QListWidgetItem(texture, ui->listWidget);
				item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
				QCheckBox* parentCheckbox = ((AddOnTableView*)parentWidget())->getCheckBox(m_iRow-1);
				item->setCheckState(parentCheckbox->checkState());
				if (addon->getInstalledTextures().contains(texture))
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
		if (item->checkState())
		{
			if (item->textColor() == QColor("green")) // installed
			{
				QString texture = item->text();
				texture = texture.left(texture.indexOf(" (installed)"));
				m_sSelectedFilesToRemove.append(texture);
			}
			else
			{
				m_sSelectedFilesToInstall.append(item->text());
			}
		}
		else
		{
			if (item->textColor() == QColor("green")) // installed
			{
				QString texture = item->text();
				texture = texture.left(texture.indexOf(" (installed)"));
				m_sSelectedFilesToRemove.removeOne(texture);
			}
			else
			{
				m_sSelectedFilesToInstall.removeOne(item->text());
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
