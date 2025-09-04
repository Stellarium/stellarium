/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
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

#ifndef SCM_CONSTELLATION_DIALOG_HPP
#define SCM_CONSTELLATION_DIALOG_HPP

#include "ScmConstellationImage.hpp"
#include "SkyCultureMaker.hpp"
#include "StelDialogSeparate.hpp"
#include "types/DrawTools.hpp"
#include <array>
#include <optional>
#include <QGraphicsPixmapItem>
#include <QObject>
#include <QString>

class Ui_scmConstellationDialog;

class ScmConstellationDialog : public StelDialogSeparate
{
protected:
	void createDialogContent() override;
	void handleDialogSizeChanged(QSizeF size) override;

public:
	ScmConstellationDialog(SkyCultureMaker *maker);
	~ScmConstellationDialog() override;
	void loadFromConstellation(scm::ScmConstellation *constellation);

public slots:
	void retranslate() override;
	void close() override;

	/**
	 * @brief Resets the constellation dialog data.
	 */
	void resetDialog();

protected slots:
	void handleFontChanged();

private slots:
	void togglePen(bool checked);
	void toggleEraser(bool checked);
	void triggerUndo();
	void triggerUploadImage();
	void triggerRemoveImage();
	void bindSelectedStar();
	void tabChanged(int index);

private:
	Ui_scmConstellationDialog *ui = nullptr;
	SkyCultureMaker *maker        = nullptr;
	scm::DrawTools activeTool     = scm::DrawTools::None;
	bool isDialogInitialized      = false;

	/// Identifier of the constellation
	QString constellationId;
	/// Placeholder identifier of the constellation
	QString constellationPlaceholderId;
	/// English name of the constellation
	QString constellationEnglishName;
	/// Native name of the constellation
	std::optional<QString> constellationNativeName;
	/// Pronunciation of the constellation
	std::optional<QString> constellationPronounce;
	/// IPA representation of the constellation
	std::optional<QString> constellationIPA;
	/// The currently displayed artwork
	ScmConstellationImage *imageItem;
	/// Holds the last used directory
	QString lastUsedImageDirectory;
	#if defined(Q_OS_MAC)
		/// Help text on how to use the pen for Mac users.
		const QString helpDrawInfoPen = "Use RightClick or Control + Click to draw a connected line.\n"
						"Use Double-RightClick or Control + Double-Click to stop drawing the line.\n"
						"Use Command to disable snap to stars.\n"
						"Use Command + F to search and connect stars.";
		/// Help text on how to use the eraser for Mac users.
		const QString helpDrawInfoEraser = "Hold RightClick or Control + Click to delete the line under the cursor.\n";
	#else
		/// Help text on how to use the pen for non-Mac users.
		const QString helpDrawInfoPen = "Use RightClick to draw a connected line.\n"
						"Use Double-RightClick to stop drawing the line.\n"
						"Use CTRL to disable snap to stars.\n"
						"Use CTRL + F to search and connect stars.";
		/// Help text on how to use the eraser for non-Mac users.
		const QString helpDrawInfoEraser = "Hold RightClick to delete the line under the cursor.\n";
	#endif
	
	/// The constellation that is currently being edited
	scm::ScmConstellation *constellationBeingEdited = nullptr;

	/**
	 * @brief Checks whether the current data is enough for the constellation to be saved.
	 */
	bool canConstellationBeSaved() const;

	/**
	 * @brief Saves the constellation data as an object in the current sky culture.
	 */
	void saveConstellation();

	/**
	 * @brief Resets and closes the dialog.
	 */
	void cancel();

	/**
	 * @brief Updates the state of the artwork.
	 */
	void updateArtwork();
};

#endif // SCM_CONSTELLATION_DIALOG_HPP
