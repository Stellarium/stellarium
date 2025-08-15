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
