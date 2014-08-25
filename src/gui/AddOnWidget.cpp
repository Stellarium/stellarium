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

#include <QPainter>
#include <QStringBuilder>

#include "AddOnWidget.hpp"
#include "ui_addonWidget.h"

AddOnWidget::AddOnWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui_AddOnWidget)
	, m_pStelAddOnDAO(StelApp::getInstance().getStelAddOnMgr().getStelAddOnDAO())
	, m_sThumbnailDir(StelApp::getInstance().getStelAddOnMgr().getThumbnailDir())
{
	ui->setupUi(this);
	connect(ui->listWidget, SIGNAL(itemChanged(QListWidgetItem*)),
		this, SLOT(slotItemChanged(QListWidgetItem*)));
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

	// List of files - applicable only for textures
	m_textureState.clear();
	ui->listWidget->setVisible(false);
	if (parentWidget()->objectName() == "texturesTableView")
	{
		// <textures, installed>
		QPair<QStringList, QStringList> set = m_pStelAddOnDAO->getListOfTextures(addonId);
		if (set.first.size() > 1) // display list just when it is a texture set
		{
			for (int i=0; i < set.first.size(); i++)
			{
				QString text = set.first.at(i);
				QListWidgetItem* item = new QListWidgetItem(text, ui->listWidget);
				item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
				item->setCheckState(Qt::Unchecked);
				m_textureState.append(0);
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

void AddOnWidget::slotItemChanged(QListWidgetItem *item)
{
	if (ui->listWidget->isVisible())
	{

		m_textureState[ui->listWidget->row(item)] = item->checkState();
		if (!m_textureState.contains(0))
			emit(textureChecked(2));
		else if (!m_textureState.contains(2))
			emit(textureChecked(0));
		else
			emit(textureChecked(1));
	}
}
