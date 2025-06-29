#ifndef SCM_CONSTELLATION_DIALOG_HPP
#define SCM_CONSTELLATION_DIALOG_HPP

#include "ScmImageAnchored.hpp"
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

public slots:
	void retranslate() override;
	void close() override;

	/**
	 * @brief Resets the constellation dialog data.
	 */
	void resetDialog();

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
	ScmImageAnchored *imageItem;
	/// Holds the last used directory
	QString lastUsedImageDirectory;
	/// Holds the help text on how to use the pen.
	const QString helpDrawInfoPen = "Use RightClick to draw a connected line.\n"
					"Use Double-RightClick to stop drawing the line.\n"
					"Use CTRL to disable snap to stars.\n"
					"Use CTRL + F to search and connect stars.";
	/// Holds the help text on how to use the eraser.
	const QString helpDrawInfoEraser = "Hold RightClick to delete the line under the cursor.\n";

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
