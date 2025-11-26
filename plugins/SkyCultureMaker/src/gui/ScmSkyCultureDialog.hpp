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

#ifndef SCM_SKY_CULTURE_DIALOG_HPP
#define SCM_SKY_CULTURE_DIALOG_HPP

#include "ScmConstellation.hpp"
#include "SkyCultureMaker.hpp"
#include "StelDialogSeparate.hpp"
#include "types/Classification.hpp"
#include "types/Description.hpp"
#include "types/License.hpp"
#include <optional>
#include <QFile>
#include <QObject>
#include <QString>
#include <QVariant>

class Ui_scmSkyCultureDialog;

class ScmSkyCultureDialog : public StelDialogSeparate
{
protected:
	void createDialogContent() override;

public:
	ScmSkyCultureDialog(SkyCultureMaker *maker);
	~ScmSkyCultureDialog() override;

	/**
	 * @brief Sets the constellations to be displayed in the dialog.
	 *
	 * @param constellations The vector of constellations to be set.
	 */
	void setConstellations(std::vector<scm::ScmConstellation> *constellations);

	/**
	 * @brief Resets the constellations to an empty vector.
	 */
	void resetConstellations();

	/**
	 * @brief Resets all fields in the dialog to their default values.
	 */
	void resetDialog();

	/**
	 * @brief Updates the add constellation button state.
	 *
	 * @param enabled Whether the button should be enabled or disabled.
	 */
	void updateAddConstellationButtons(bool enabled);

public slots:
	void retranslate() override;
	void close() override;

protected slots:
	void handleFontChanged();

private slots:
	void saveSkyCulture();
	void openConstellationDialog(bool isDarkConstellation);
	void editSelectedConstellation();
	void removeSelectedConstellation();
	void updateEditConstellationButton();
	void updateRemoveConstellationButton();
	void saveLicense();

private:
	Ui_scmSkyCultureDialog *ui = nullptr;
	SkyCultureMaker *maker     = nullptr;

	/// The name of the sky culture.
	QString name = "";

	/// The vector of constellations to be displayed in the dialog.
	std::vector<scm::ScmConstellation> *constellations = nullptr;

	/**
	 * @brief Gets the display name from a constellation.
	 *
	 * @param constellation The constellation to get the display name from.
	 * @return The display name of the constellation.
	 */
	QString getDisplayNameFromConstellation(const scm::ScmConstellation &constellation) const;

	/**
	 * @brief Sets the id of the sky culture from the name.
	 *
	 * @param name The name to set the id from.
	 */
	void setIdFromName(QString &name);

	/**
	 * @brief Compiles the Constellations section from descriptions of all constellations.
	 */
	QString makeConstellationsSection() const;

	/**
	 * @brief Gets the description from the text edit.
	 *
	 * @return The description from the text edit.
	 */
	scm::Description getDescriptionFromTextEdit() const;
};

#endif // SCM_SKY_CULTURE_DIALOG_HPP
