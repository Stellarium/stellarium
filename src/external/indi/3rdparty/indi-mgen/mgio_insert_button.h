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
 * mgio_insert_button.h
 *
 *  Created on: 22 mars 2017
 *      Author: TallFurryMan
 */

#ifndef _3RDPARTY_INDI_MGEN_MGIO_INSERT_BUTTON_H_
#define _3RDPARTY_INDI_MGEN_MGIO_INSERT_BUTTON_H_

#include "mgc.h"

class MGIO_INSERT_BUTTON : MGC
{
  public:
    virtual IOByte opCode() const { return 0x5D; }
    virtual IOMode opMode() const { return OPM_APPLICATION; }

  public:
    enum Button
    {
        IOB_NONE     = -1,
        IOB_ESC      = 0,
        IOB_SET      = 1,
        IOB_LEFT     = 2,
        IOB_RIGHT    = 3,
        IOB_UP       = 4,
        IOB_DOWN     = 5,
        IOB_LONG_ESC = 6,
    };

  public:
    virtual IOResult ask(MGenDevice &root) //throw(IOError)
    {
        if (CR_SUCCESS != MGC::ask(root))
            return CR_FAILURE;

        if (root.lock())
        {
            IOByte const b = query[2];
            _D("sending button %d", b);

            query[2] &= 0x7F;
            root.write(query);
            root.read(answer);

            query[2] |= 0x80;
            root.write(query);
            root.read(answer);

            _D("button %d sent", b);

            root.unlock();
        }
        return CR_SUCCESS;
    }

  public:
    MGIO_INSERT_BUTTON(Button button) : MGC(IOBuffer{ opCode(), 0x01, (unsigned char)button }, IOBuffer(2)){};
};

#endif /* _3RDPARTY_INDI_MGEN_MGIO_INSERT_BUTTON_H_ */
