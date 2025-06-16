#ifndef SCM_CONSTELLATION_DIALOG_HPP
#define SCM_CONSTELLATION_DIALOG_HPP

#include "SkyCultureMaker.hpp"
#include "StelDialogSeparate.hpp"
#include "types/DrawTools.hpp"
#include <optional>
#include <QObject>
#include <QString>

class Ui_scmConstellationDialog;

class ScmConstellationDialog : public StelDialogSeparate
{
protected:
	void createDialogContent() override;

public:
	ScmConstellationDialog(SkyCultureMaker *maker);
	~ScmConstellationDialog() override;

public slots:
	void retranslate() override;
	void close() override;

private slots:
	void togglePen(bool checked);
	void toggleEraser(bool checked);
	void triggerUndo();

private:
	Ui_scmConstellationDialog *ui = nullptr;
	SkyCultureMaker *maker        = nullptr;
	scm::DrawTools activeTool     = scm::DrawTools::None;

	/// Identifier of the constellation
	QString constellationId;
	/// Placeholder identifier of the constellation
	QString constellationPlaceholderId;
	/// English name of the constellation
	QString constellationEnglishName;
	/// Native name of the constellation
	std::optional<QString> constellationNativeName;
	/// Pronounciation of the constellation
	std::optional<QString> constellationPronounce;
	/// IPA representation of the constellation
	std::optional<QString> constellationIPA;

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
	 * @brief Resets the constellation dialog data.
	 */
	void resetDialog();
};

#endif // SCM_CONSTELLATION_DIALOG_HPP
