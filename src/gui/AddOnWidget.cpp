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

AddOnWidget::AddOnWidget(QWidget* parent, int row)
	: QWidget(parent)
	, m_iRow(row)
	, ui(new Ui_AddOnWidget)
	, m_pStelAddOnDAO(StelApp::getInstance().getStelAddOnMgr().getStelAddOnDAO())
	, m_sThumbnailDir(StelApp::getInstance().getStelAddOnMgr().getThumbnailDir())
{
	ui->setupUi(this);
	connect(ui->listWidget, SIGNAL(itemChanged(QListWidgetItem*)),
		this, SLOT(slotItemChanged(QListWidgetItem*)));
	connect(((AddOnTableView*)parent), SIGNAL(rowChecked(int, bool)),
		this, SLOT(slotCheckAllFiles(int, bool)));
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

void AddOnWidget::init(int addonId)
{
	StelAddOnDAO::WidgetInfo info = m_pStelAddOnDAO->getAddOnWidgetInfo(addonId);

	// Authors (name, email and url)
	QString author1 = info.a1Name;
	if (!info.a1Url.isEmpty())
		author1 = author1 % " <" % info.a1Url % ">";
	if (!info.a1Email.isEmpty())
		author1 = author1 % " <" % info.a1Email % ">";

	QString author2 = info.a2Name;
	if (!info.a2Url.isEmpty())
		author2 = author2 % " <" % info.a2Url % ">";
	if (!info.a2Email.isEmpty())
		author2 = author2 % " <" % info.a2Email % ">";

	ui->txtAuthor->setText(author2.isEmpty() ? author1 : author1 % "\n" % author2);

	// Description
	ui->txtDescription->setText(info.description);

	// License (name and url)
	ui->txtLicense->setText(info.licenseName % " <" % info.licenseUrl % ">");

	// Download Size
	ui->txtSize->setText(info.downloadSize % " MB");

	// Thumbnail
	QPixmap thumbnail(m_sThumbnailDir % info.idInstall % ".jpg");
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
	ui->listWidget->setVisible(false);
	if (parentWidget()->objectName() == "texturesTableView")
	{
		m_sSelectedFilesToInstall.clear();
		m_sSelectedFilesToRemove.clear();

		// <textures, installed>
		QPair<QStringList, QStringList> set = m_pStelAddOnDAO->getListOfTextures(addonId);
		if (set.first.size() > 1) // display list just when it is a texture set
		{
			for (int i=0; i < set.first.size(); i++)
			{
				QString text = set.first.at(i);
				QListWidgetItem* item = new QListWidgetItem(text, ui->listWidget);
				item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
				QCheckBox* parentCheckbox = ((AddOnTableView*)parentWidget())->getCheckBox(m_iRow-1);
				item->setCheckState(parentCheckbox->checkState());
				if (set.second.at(i).toInt())
				{
					text = text % " (installed)";
					item->setText(text);
					item->setTextColor(QColor("green"));
				}
			}
			ui->listWidget->setVisible(true);
		}
	}
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
				m_sSelectedFilesToRemove.append(item->text());
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
				m_sSelectedFilesToRemove.removeOne(item->text());
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
