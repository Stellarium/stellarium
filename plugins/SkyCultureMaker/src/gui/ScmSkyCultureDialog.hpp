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
#include "types/Region.hpp"
#include <optional>
#include <qtreewidget.h>
#include <QFile>
#include <QObject>
#include <QString>
#include <QVariant>

class Ui_scmSkyCultureDialog;
class ScmAddPolygonDialog;

class ScmSkyCultureDialog : public StelDialogSeparate
{
protected:
	void createDialogContent() override;
	bool eventFilter(QObject *obj, QEvent *event) override;

public:
	ScmSkyCultureDialog(SkyCultureMaker *maker);
	~ScmSkyCultureDialog() override;

	/**
	 * @brief Sets the constellations to be displayed in the dialog.
	 *
	 * @param constellations The vector of constellations to be set.
	 */
	void setConstellations(std::vector<std::unique_ptr<scm::ScmConstellation>> *constellations);

	/**
	 * @brief Resets the constellations to an empty vector.
	 */
	void resetConstellations();

	/**
	 * @brief Resets all fields in the dialog to their default values.
	 */
	void resetDialog();

	/**	
	 * @brief Populates all UI fields from a loaded sky culture.
	 *        Call this after setSkyCulture() when entering edit mode.
	 *
	 * @param sc The sky culture to populate from (must not be nullptr).
	 */
	void populateFromSkyCulture(scm::ScmSkyCulture *sc);

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

private:
	enum class CnObjectType { Star = 0, Planet = 1, DSO = 2 };

private slots:
	void saveSkyCulture();
	void openConstellationDialog(bool isDarkConstellation);
	void editSelectedConstellation();
	void removeSelectedConstellation();
	void updateEditConstellationButton();
	void updateRemoveConstellationButton();
	void updateRemovePolygonButton();
	void saveLicense();
	void updateSkyCultureTimeValue(int year);
	void addLocation(const scm::CulturePolygon polygon);
	void removeLocation();
	void selectLocation(QTreeWidgetItem *item);
	void showAddPolygon();
	void hideAddPolygon();
	void confirmAddPolygon();
	void cancelAddPolygon();
	// (uncomment when multiple regions are used)
	//void checkMutExRegions(const QStringList checkedItems);
	void cnUpdateVisibleField(CnObjectType type);
	void cnAddNew();
	void cnSaveChanges();
	void cnRemoveEntry();
	void cnUseSelectedObject();
	void cnOnTableSelectionChanged();

private:
	Ui_scmSkyCultureDialog *ui = nullptr;
	SkyCultureMaker *maker     = nullptr;

	/// The name of the sky culture.
	QString name = "";

	/// The vector of constellations to be displayed in the dialog.
	std::vector<std::unique_ptr<scm::ScmConstellation>> *constellations = nullptr;

	/// Help text on how to digitize polygons in the map.
	const QString mapToolTip = q_(
		"Controls:\n"
		"LMB / RMB: Set a new vertex for the active polygon\n"
		"SHIFT + LMB / MMB: Navigate the Map\n"
		"Double RMB: Save the active polygon\n"
		"CTRL + Z / DELETE: Remove the last point of the active polygon\n"
		"ESC: Remove all points of the active polygon\n"
		"Scroll UP / DOWN: Zoom in / out of the map\n"
		"CTRL + Scroll: Fine grained zoom"
		);

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
	 * @brief Clears the references list and resets the helper buttons.
	 */
	void resetReferences();

	/**
	 * @brief Adds a new reference to the list and initiates editing it.
	 */
	void addNewReference();

	/**
	 * @brief Removes selected reference from the list.
	 */
	void removeReference();

	/**
	 * @brief Updates states of the buttons controlling the references list.
	 */
	void updateReferencesButtons();

	/**
	 * @brief Moves the selected reference above the previous reference, renumerating all references.
	 */
	void moveCurrentReferenceUp();

	/**
	 * @brief Moves the selected reference below the next reference, renumerating all references.
	 */
	void moveCurrentReferenceDown();

	/**
	 * @brief Updates reference numbers as shown in the list.
	 */
	void updateReferencesNumeration();

	/**
	 * @brief Gets the description from the text edit.
	 *
	 * @return The description from the text edit.
	 */
	scm::Description getDescriptionFromTextEdit() const;

	/**
	 * @brief Populates the overview/description tab fields from a Description.
	 * 
	 * @param desc The Description to populate from.
	 */
	void populateDescriptionTab(const scm::Description &desc);

	/**
	 * @brief Populates the references list from a serialised references string.
	 * 
	 * @param references The references string to populate from.
	 */
	void populateReferences(const QString &references);

	/**
	 * @brief Populates the common names table from the sky culture.
	 * 
	 * @param sc The sky culture to populate from.
	 */
	void populateCommonNames(const QMap<QString, QList<scm::ScmCulturalName>> &culturalNames);

	/**
	 * @brief Populates the geographical locations tab from the sky culture.
	 * 
	 * @param sc The sky culture to populate from.
	 */
	void populateLocationsTab(scm::ScmSkyCulture *sc);

	/**
	 * @brief Compiles the References section from all the references in the list.
	 */
	QString makeReferencesSection() const;

	/**
	 * @brief Opens the constellation dialog with data for a given constellation.
	 * @param constellationId The ID of the constellation to open the dialog for.
	 */
	void openConstellationDialog(const QString &constellationId);

	/**
	 * @brief Initialize the time of the map, slider and spinboxes.
	 *
	 */
	void initSkyCultureTime();

	/**
	 * @brief Clears the common names form.
	 */
	void cnClearForm();

	/**
	 * @brief Refreshes the common names table.
	 */
	void cnRefreshTable();

	/**
	 * @brief Reads the common name data from the form and returns it as a ScmCulturalName object.
	 */
	scm::ScmCulturalName cnReadForm() const;

	/**
	 * @brief Populates the common names form with data from a given ScmCulturalName object.
	 */
	void cnPopulateForm(const QString &key, const scm::ScmCulturalName &name);

	/**
	 * @brief Builds the normalized object key from the type combo box and the identifier line edit.
	 *        Stars: "HIP <id>", Planets: "NAME <id>", DSOs: "<id>".
	 */
	QString cnBuildKey() const;

	/// Common names entries stored as (object key, name data) pairs.
	QList<QPair<QString, scm::ScmCulturalName>> cnEntries;

	/// Index of the entry currently loaded in the form (-1 = new entry).
	int cnEditingRow = -1;

	/**
	 * @brief Validates the common names form and builds the key and name if valid.
	 *        Shows a warning message and returns false on the first validation failure.
	 * @param outKey   Receives the normalized object key on success.
	 * @param outName  Receives the cultural name data on success.
	 * @return true if all validation checks pass, false otherwise.
	 */
	bool cnValidateForm(QString &outKey, scm::ScmCulturalName &outName);

	/**
	 * @brief Returns true if cnEntries already contains an entry with the given key and special value.
	 * @param excludeRow Row index to skip during the search (-1 to check all rows).
	 */
	bool cnIsDuplicate(const QString &key, StelObject::CulturalNameSpecial special, int excludeRow = -1) const;

	/**
	 * @brief Checks whether the object identified by the key exists in the current Stellarium database.
	 *        If not found, a warning dialog is shown to the user, allowing them to either proceed with 
	 * 		  saving the entry or cancel and fix the key.
	 */
	bool cnCheckObjectExists(const QString &key);

	/// When true, the warning for objects that don't exist is suppressed for the rest of the session.
	bool cnSkipObjectExistCheck = false;
};

#endif // SCM_SKY_CULTURE_DIALOG_HPP
