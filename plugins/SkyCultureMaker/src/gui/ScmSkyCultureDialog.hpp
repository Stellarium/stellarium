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
	 * @brief Sets the info label text.
	 *
	 * @param text The text to set in the info label.
	 */
	void setInfoLabel(const QString &text);

	/**
	 * @brief Resets all fields in the dialog to their default values.
	 */
	void resetDialog();

public slots:
	void retranslate() override;
	void close() override;

private slots:
	void saveSkyCulture();
	void constellationDialog();
	void removeSelectedConstellation();
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
	 * @brief Gets the description from the text edit.
	 *
	 * @return The description from the text edit.
	 */
	scm::Description getDescriptionFromTextEdit() const;
};

#endif // SCM_SKY_CULTURE_DIALOG_HPP
