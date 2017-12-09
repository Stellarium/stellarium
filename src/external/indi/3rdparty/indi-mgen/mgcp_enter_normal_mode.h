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

#ifndef _3RDPARTY_INDI_MGEN_MGCP_ENTER_NORMAL_MODE_H_
#define _3RDPARTY_INDI_MGEN_MGCP_ENTER_NORMAL_MODE_H_

/*
 * mgcp_enter_normal_mode.h
 *
 *  Created on: 22 mars 2017
 *      Author: TallFurryMan
 */

#include "mgc.h"

class MGCP_ENTER_NORMAL_MODE : MGC
{
  public:
    virtual IOByte opCode() const { return 0x42; }
    virtual IOMode opMode() const { return OPM_COMPATIBLE; }

  public:
    virtual IOResult ask(MGenDevice &root) //throw(IOError)
    {
        if (CR_SUCCESS != MGC::ask(root))
            return CR_FAILURE;

        if (root.lock())
        {
            root.write(query);
            sleep(1);
            root.unlock();
        }
        return CR_SUCCESS;
    }

  public:
    MGCP_ENTER_NORMAL_MODE() : MGC(IOBuffer{ opCode() }, IOBuffer()){};
};

#endif /* _3RDPARTY_INDI_MGEN_MGCP_ENTER_NORMAL_MODE_H_ */
