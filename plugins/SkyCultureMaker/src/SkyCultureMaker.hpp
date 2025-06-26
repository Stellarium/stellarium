#ifndef SKYCULTUREMAKER_HPP
#define SKYCULTUREMAKER_HPP

#include "ScmConstellation.hpp"
#include "ScmDraw.hpp"
#include "ScmSkyCulture.hpp"
#include "StelCore.hpp"
#include "StelModule.hpp"
#include "StelObjectModule.hpp"
#include "StelTranslator.hpp"
#include "VecMath.hpp"
#include <QFile>

#include <QFont>

class QPixmap;
class StelButton;
class ScmSkyCultureDialog;
class ScmConstellationDialog;
class ScmStartDialog;
class ScmSkyCultureExportDialog;
class ScmHideOrAbortMakerDialog;

/// This is an example of a plug-in which can be dynamically loaded into stellarium
class SkyCultureMaker : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool enabledScm READ getIsScmEnabled WRITE setIsScmEnabled NOTIFY eventIsScmEnabled)
public:
	SkyCultureMaker();
	~SkyCultureMaker() override;

	enum class DialogID
	{
		None,
		StartDialog,
		SkyCultureDialog,
		SkyCultureExportDialog,
		HideOrAbortMakerDialog,
		ConstellationDialog
	};

	/// @brief Set the toggle value for a given action.
	/// @param toggle The toggled value to be set.
	static void setActionToggle(const QString &id, bool toggle);

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
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
	void hideAllDialogsAndDisableSCM();

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
	 * @brief Sets the info label text in the sky culture dialog.
	 * @param text The text to set in the info label.
	 */
	void setSkyCultureDialogInfoLabel(const QString &text);

	/**
	 * @brief Sets the current sky culture description.
	 * @param description The description to set.
	 */
	void setSkyCultureDescription(const scm::Description &description);

	/**
	 * @brief Saves the current sky culture description as markdown text.
	 * @return true if the description was saved successfully, false otherwise.
	 */
	bool saveSkyCultureDescription();

	/**
	 * @brief Gets the saving file for the sky culture description.
	 * @return The file to save the description to.
	 */
	QFile getScmDescriptionFile();

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
	QMap<DialogID, bool> scmDialogVisibilityMap = {
		{DialogID::StartDialog, false},
		{DialogID::SkyCultureDialog, false},
		{DialogID::ConstellationDialog, false},
		{DialogID::SkyCultureExportDialog, false},
		{DialogID::HideOrAbortMakerDialog, false}
	};
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
