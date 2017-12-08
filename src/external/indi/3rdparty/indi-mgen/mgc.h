/*
    INDI 3rd party driver
    Lacerta MGen Autoguider INDI driver, implemented with help from
    Tommy (teleskopaustria@gmail.com) and Zoltan (mgen@freemail.hu).

    Teleskop & Mikroskop Zentrum (www.teleskop.austria.com)
    A-1050 WIEN, Schönbrunner Strasse 96
    +43 699 1197 0808 (Shop in Wien und Rechnungsanschrift)
    A-4020 LINZ, Gärtnerstrasse 16
    +43 699 1901 2165 (Shop in Linz)

    Lacerta GmbH
    UmsatzSt. Id. Nr.: AT U67203126
    Firmenbuch Nr.: FN 379484s

    Copyright (C) 2017 by TallFurryMan (eric.dejouhanet@gmail.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

/*
 * mgencommand.h
 *
 *  Created on: 21 janv. 2017
 *      Author: TallFurryMan
 */

#ifndef MGENCOMMAND_HPP_
#define MGENCOMMAND_HPP_

#include "mgen_device.h"

class MGC
{
  public:
    /** \internal Stringifying the name of the command */
    char const *name() const { return typeid(*this).name(); }

  public:
    /** \brief Returning the character operation code of the command */
    virtual IOByte opCode() const = 0;

    /** \brief Returning the operation mode for this command */
    virtual IOMode opMode() const = 0;

  protected:
    /** \internal The I/O query buffer to be written to the device */
    IOBuffer query;

    /** \internal The I/O answer buffer to be read from the device */
    IOBuffer answer;

    /** \brief Basic verifications to call before running the actual command implementation */
    virtual IOResult ask(MGenDevice &root) //throw(IOError)
    {
        if (opMode() != OPM_UNKNOWN && opMode() != root.getOpMode())
        {
            _E("operating mode %s does not support command", MGenDevice::DBG_OpModeString(opMode()));
            return CR_FAILURE;
        }

        return CR_SUCCESS;
    }

  protected:
    MGC(IOBuffer query, IOBuffer answer) : query(query), answer(answer){};

    virtual ~MGC(){};
};

#endif /* 3RDPARTY_INDI_MGEN_MGENCOMMAND_HPP_ */
