/**
 * @file ScmCommonName.hpp
 * @author lgrumbach
 * @brief Represents a common name of a star, planet or nonstellar object. Do not use for constellations.
 * @version 0.1
 * @date 2025-05-09
 *
 */

#ifndef SCM_COMMONNAME_HPP
#define SCM_COMMONNAME_HPP

#include <optional>
#include <vector>
#include <QJsonObject>
#include <QString>

namespace scm
{

class ScmCommonName
{
public:
	ScmCommonName(const QString &id);

	/// Sets the english name
	void setEnglishName(const QString &name);

	/// Sets the native name
	void setNativeName(const QString &name);

	/// Sets the native name in European glyphs or Pinyin for Chinese.
	void setPronounce(const QString &pronounce);

	/// Sets the native name in IPA (International Phonetic Alphabet)
	void setIpa(const QString &name);

	/// Sets the references to the sources of the name spellings
	void setReferences(const std::vector<int> &refs);

	/// Returns the english name
	QString getEnglishName() const;

	/// Returns the native name
	std::optional<QString> getNativeName() const;

	/// Returns the native name in European glyphs or Pinyin for Chinese.
	std::optional<QString> getPronounce() const;

	/// Returns the native name in IPA (International Phonetic Alphabet)
	std::optional<QString> getIpa() const;

	/// Returns the references to the sources of the name spellings
	std::optional<std::vector<int>> getReferences() const;

	/// Returns the common name as a JSON object
	QJsonObject toJson() const;

private:
	/*! Identifier of the common name
	 * 	Example:
	 *    Star  : "HIP  <code>"
	 *    Planet: "NAME <name>"
	 */
	QString id;

	/// The english name
	QString englishName;

	/// The native name
	std::optional<QString> nativeName;

	/// Native name in European glyphs, if needed. For Chinese, expect Pinyin here.
	std::optional<QString> pronounce;

	/// The native name in IPA (International Phonetic Alphabet)
	std::optional<QString> ipa;

	/// References to the sources of the name spellings
	std::optional<std::vector<int>> references;
};

} // namespace scm

#endif // SCM_COMMONNAME_HPP
