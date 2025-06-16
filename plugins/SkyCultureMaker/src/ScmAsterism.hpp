/**
 * @file ScmAsterism.hpp
 * @author lgrumbach
 * @brief Represents an asterism in a sky culture.
 * @version 0.1
 * @date 2025-05-09
 *
 */

#ifndef SCM_ASTERISM_HPP
#define SCM_ASTERISM_HPP

#include "QString"
#include "ScmCommonName.hpp"

namespace scm
{

class ScmAsterism
{
public:
	/**
    * @brief Sets the id of the asterism
    * 
    * @param id id
    */
	void setId(const QString &id);

	/**
    * @brief Gets the id of the asterism
    * 
    * @return id
    */
	QString getId() const;

	/**
     * @brief Sets the common name of the asterism.
     *
     * @param name The common name of this asterim.
     */
	void setCommonName(const ScmCommonName &name);

	/**
     * @brief Returns the common name of the asterism.
     *
     * @return The common name of the this asterism.
     */
	ScmCommonName getCommonName() const;

private:
	/// Id of the Asterism
	QString id;

	/// Common name of the constellation
	ScmCommonName commonName;
};

} // namespace scm

#endif // SCM_ASTERISM_HPP
