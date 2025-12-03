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

#ifndef SKYCULTUREMAKER_HPP
#define SKYCULTUREMAKER_HPP

#include "ScmConstellation.hpp"
#include "ScmConstellationArtwork.hpp"
#include "ScmDraw.hpp"
#include "ScmSkyCulture.hpp"
#include "StelCore.hpp"
#include "StelModule.hpp"
#include "StelObjectModule.hpp"
#include "StelTranslator.hpp"
#include "VecMath.hpp"
#include "types/DialogID.hpp"
#include <QDir>
#include <QFile>
#include <QFont>
#include <QMessageBox>

class QPixmap;
class StelButton;
class ScmSkyCultureDialog;
class ScmConstellationDialog;
class ScmStartDialog;
class ScmSkyCultureExportDialog;
class ScmHideOrAbortMakerDialog;

class SkyCultureMaker : public StelModule
{
	Q_OBJECT
	// TODO: var - getter - setter - trigger
	Q_PROPERTY(bool enabledScm READ getIsScmEnabled WRITE setIsScmEnabled NOTIFY eventIsScmEnabled)
public:
	SkyCultureMaker();
	~SkyCultureMaker() override;

	/// @brief Set the toggle value for a given action.
	/// @param toggle The toggled value to be set.
	static void setActionToggle(const QString &id, bool toggle);

	void init() override;
	// Activate only if update() does something.
	// void update(double deltaTime) override {}
	void draw(StelCore *core) override;
	double getCallOrder(StelModuleActionName actionName) const override;

	/// Handle mouse clicks. Please note that most of the interactions will be done through the GUI module.
	/// @return set the event as accepted if it was intercepted
	void handleMouseClicks(QMouseEvent *) override;

	/// Handle mouse moves. Please note that most of the interactions will be done through the GUI module.
	/// @return true if the event was intercepted
	bool handleMouseMoves(int x, int y, Qt::MouseButtons b) override;

	/// Handle key events. Please note that most of the interactions will be done through the GUI module.
	/// @param e the Key event
	/// @return set the event as accepted if it was intercepted
	void handleKeys(QKeyEvent *e) override;

	/**
	 * @brief Sets the toolbar button state.
	 * @param b The boolean value to be set.
	 */
	void setToolbarButtonState(bool b);

	/**
	 * @brief Shows the start dialog for the sky culture maker.
	 * 
	 * @param b The boolean value to be set.
	 */
	void setStartDialogVisibility(bool b);

	/**
	 * @brief Shows the sky culture dialog.
	 *
	 * @param b The boolean value to be set.
	 */
	void setSkyCultureDialogVisibility(bool b);

	/**
	 * @brief Shows the constellation dialog.
	 *
	 * @param b The boolean value to be set.
	 */
	void setConstellationDialogVisibility(bool b);

	/**
	 * @brief Sets whether the constellation dialog is for a dark constellation.
	 *
	 * @param isDarkConstellation The boolean value to be set.
	 */
	void setConstellationDialogIsDarkConstellation(bool isDarkConstellation);

	/**
	 * @brief Shows the sky culture export dialog.
	 *
	 * @param b The boolean value to be set.
	 */
	void setSkyCultureExportDialogVisibility(bool b);

	/**
	 * @brief Shows the hide or abort maker dialog.
	 *
	 * @param b The boolean value to be set.
	 */
	void setHideOrAbortMakerDialogVisibility(bool b);

	/**
	 * @brief Set the visibility of all dialogs.
	 * 
	 * @param b The boolean value to be set.
	 */
	void hideAllDialogs();

	/**
	 * @brief Toggles the usage of the line draw.
	 *
	 * @param b The boolean value to be set.
	 */
	void setIsLineDrawEnabled(bool b);

	/**
	 * @brief Triggers a single undo operation in the line draw.
	 */
	void triggerDrawUndo();

	/**
	 * @brief Sets the active used draw tool.
	 *
	 * @param tool The tool to be used.
	 */
	void setDrawTool(scm::DrawTools tool);

	/**
	 * @brief Sets a new sky culture object
	 */
	void setNewSkyCulture();

	/**
	 * @brief Gets the current set sky culture.
	 */
	scm::ScmSkyCulture *getCurrentSkyCulture();

	/**
	 * @brief Gets the SCM drawing object.
	 */
	scm::ScmDraw *getScmDraw();

	/**
	 * @brief Resets the SCM drawing object.
	 */
	void resetScmDraw();

	/**
	 * @brief Triggers an update of the sky culture dialog.
	 */
	void updateSkyCultureDialog();

	/**
	 * @brief Sets the current sky culture description.
	 * @param description The description to set.
	 */
	void setSkyCultureDescription(const scm::Description &description);

	/**
	 * @brief Sets the current sky culture start time.
	 * @param startTime The start time to set.
	 */
	void setSkyCultureStartTime(int startTime);

	/**
	 * @brief Sets the current sky culture end time.
	 * @param endTime The end time to set.
	 */
	void setSkyCultureEndTime(const QString &endTime);

	/**
	 * @brief Adds a CulturePolygon to the sky culture.
	 * @param polygon The culturePolygon to add.
	 */
	void addSkyCultureLocation(const scm::CulturePolygon &polygon);

	/**
	 * @brief Removes a CulturePolygon from the sky culture by its Id.
	 * @param id The identifier of the location.
	 */
	void removeSkyCultureLocation(int id);

	/**
	 * @brief Saves the current sky culture description as markdown text.
	 * @param directory The directory to save the description in.
	 * @return true if the description was saved successfully, false otherwise.
	 */
	bool saveSkyCultureDescription(const QDir &directory);

	/**
	 * @brief Saves the visibility state of the SCM dialogs.
	 */
	void saveScmDialogVisibilityState();

	/**
	 * @brief Restores the visibility state of the SCM dialogs.
	 */
	void restoreScmDialogVisibilityState();

	/**
	 * @brief Checks if any SCM dialog is currently hidden.
	 */
	bool isAnyDialogHidden() const;

	/**
	 * @brief Resets all SCM dialogs content and visibility states.
	*/
	void resetScmDialogs();

	/**
	 * @brief Resets the visibility state of the SCM dialogs.
	 */
	void resetScmDialogsVisibilityState();

	/**
	 * @brief Checks if any SCM dialog is currently visible.
	 * @return true if any dialog is visible, false otherwise.
	 */
	bool isAnyDialogVisible() const;
	/**
	 * @brief Sets the temporary artwork that should be drawn.
	 * @param artwork The artwork to draw.
	 */
	void setTempArtwork(const scm::ScmConstellationArtwork *artwork);

	/**
	 * @brief Opens the constellation dialog with data for a given constellation.
	 * @param constellationId The ID of the constellation to open the dialog for.
	 */
	void openConstellationDialog(const QString &constellationId);

	/**
	 * @brief Displays an information message to the user.
	 * 
	 * @param parent The parent widget of the message box.
	 * @param dialogName The name of the dialog to be shown in the title bar.
	 * @param message The message to be displayed.
	 */
	void showUserInfoMessage(QWidget *parent, const QString &dialogName, const QString &message);

	/**
	 * @brief Displays a warning message to the user.
	 * 
	 * @param parent The parent widget of the message box.
	 * @param dialogName The name of the dialog to be shown in the title bar.
	 * @param message The message to be displayed.
	 */
	void showUserWarningMessage(QWidget *parent, const QString &dialogName, const QString &message);

	/**
	 * @brief Displays an error message to the user.
	 * 
	 * @param parent The parent widget of the message box.
	 * @param dialogName The name of the dialog to be shown in the title bar.
	 * @param message The message to be displayed.
	 */
	void showUserErrorMessage(QWidget *parent, const QString &dialogName, const QString &message);

signals:
	void eventIsScmEnabled(bool b);

public slots:
	bool getIsScmEnabled() const { return isScmEnabled; }

	void setIsScmEnabled(bool b);

private:
	const QString groupId      = N_("Sky Culture Maker");
	const QString actionIdLine = "actionShow_SkyCultureMaker_Line";

	/// Indicates that SCM creation process is enabled (QT Signal)
	bool isScmEnabled = false;

	/// Indicates that line drawing can be done (QT Signal)
	bool isLineDrawEnabled = false;

	/// The button to activate line drawing.
	StelButton *toolbarButton = nullptr;

	/// Font used for displaying our text
	QFont font;

	/// The object used for drawing constellations
	scm::ScmDraw *drawObj = nullptr;

	/// Toogle SCM creation process on
	void startScmProcess();

	/// Toogle SCM creation process off
	void stopScmProcess();

	/// Dialog for starting/editing/cancel creation process
	ScmStartDialog *scmStartDialog = nullptr;

	/// Dialog for creating/editing a sky culture
	ScmSkyCultureDialog *scmSkyCultureDialog = nullptr;

	/// Dialog for creating/editing a constellation
	ScmConstellationDialog *scmConstellationDialog = nullptr;

	/// Dialog for exporting a sky culture
	ScmSkyCultureExportDialog *scmSkyCultureExportDialog = nullptr;

	/// Dialog for hiding or aborting maker process
	ScmHideOrAbortMakerDialog *scmHideOrAbortMakerDialog = nullptr;

	/// The current sky culture
	scm::ScmSkyCulture *currentSkyCulture = nullptr;

	/**
	 * Store the visibility state of the SCM dialogs
	 * The key is the dialog ID, the value is true if the dialog was hidden, false if newly created.
	 */
	QMap<scm::DialogID, bool> scmDialogVisibilityMap = {
		{scm::DialogID::StartDialog,            false},
		{scm::DialogID::SkyCultureDialog,       false},
		{scm::DialogID::ConstellationDialog,    false},
		{scm::DialogID::SkyCultureExportDialog, false},
		{scm::DialogID::HideOrAbortMakerDialog, false}
        };

	/// The artwork to temporary draw on the sky.
	const scm::ScmConstellationArtwork *tempArtwork = nullptr;

	/**
	 * @brief Initializes a setting with a default value if it does not exist.
	 * This does not open or close the settings group.
	 * @param key The key of the setting.
	 * @param defaultValue The default value to set if the setting does not exist.
	 */
	void initSetting(QSettings *conf, const QString key, const QVariant &defaultValue);
};

#include "StelPluginInterface.hpp"
#include <QObject>

/// This class is used by Qt to manage a plug-in interface
class SkyCultureMakerStelPluginInterface : public QObject,
					   public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule *getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	QObjectList getExtensionList() const override { return QObjectList(); }
};

#endif /* SKYCULTUREMAKER_HPP */
