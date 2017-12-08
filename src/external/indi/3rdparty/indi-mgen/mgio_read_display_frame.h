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
 * mgio_read_display_frame.h
 *
 *  Created on: 22 mars 2017
 *      Author: TallFurryMan
 */

#ifndef _3RDPARTY_INDI_MGEN_MGIO_READ_DISPLAY_FRAME_H_
#define _3RDPARTY_INDI_MGEN_MGIO_READ_DISPLAY_FRAME_H_

#include "mgc.h"

class MGIO_READ_DISPLAY_FRAME : MGC
{
  public:
    virtual IOByte opCode() const { return 0x5D; }
    virtual IOMode opMode() const { return OPM_APPLICATION; }

  protected:
    static std::size_t const frame_size = (128 * 64) / 8;
    IOBuffer bitmap_frame;

  public:
    typedef std::array<unsigned char, frame_size * 8> ByteFrame;
    ByteFrame &get_frame(ByteFrame &frame) const
    {
        /* A display byte is 8 display bits shaping a column, LSB at the top
         *
         *      C0      C1      C2      --    C127
         * L0  D0[0]   D1[0]   D2[0]    --   D127[0]
         * L1  D0[1]   D1[1]   D2[1]    --   D127[1]
         * |
         * L7  D0[7]   D1[7]   D2[7]    --   D127[7]
         * L8  D128[0] D129[0] D130[0]  --   D255[0]
         * L9  D128[1] D129[1] D130[1]  --   D255[1]
         * |
         * L15 D128[7] D129[7] D130[7]  --   D255[7]
         * ...
         */
        for (unsigned int i = 0; i < frame.size(); i++)
        {
            unsigned int const c = i % 128;
            unsigned int const l = i / 128;
            unsigned int const B = c + (l / 8) * 128;
            unsigned int const b = l % 8;

            frame[i] = ((bitmap_frame[B] >> b) & 0x01) ? '0' : ' ';
        }
#if 0
        _D("    0123456789|123456789|123456789|123456789|123456789|123456789|123456789|123456789|123456789|123456789|123456789|123456789|1234567","");
        for(unsigned int i = 0; i < frame.size()/128; i++)
        {
            char line[128];
            for(unsigned int j = 0; j < 128; j++)
                line[j] = frame[i*128+j];
            _D("%03d %128.128s", i, line);
        }
#endif
        return frame;
    };

  public:
    virtual IOResult ask(MGenDevice &root) //throw(IOError)
    {
        if (CR_SUCCESS != MGC::ask(root))
            return CR_FAILURE;

        bitmap_frame.clear();

        /* Sorted out from spec and experiment:
         * Query:  IO_FUNC SUBFUNC ADDR_L ADDR_H COUNT for each block
         * Answer: IO_FUNC D0 D1 D2... (COUNT bytes)
         *
         * To finish communication (not exactly perfect, but keeps I/O synced)
         * Query:  IO_FUNC 0xFF
         * Answer: IO_FUNC
         */

        /* We'll read 8 blocks of 128 bytes, not optimal, but it's working */
        answer.resize(1 + 128);

        if (root.lock())
        {
            _D("reading UI frame",0);

            for (unsigned int block = 0; block < 8 * 128; block += 128)
            {
                /* Query is using 10 bits of the address over two bytes, then 1 byte for the count */
                IOByte const length = 128;
                query[2]            = (unsigned char)((block & 0x03FF) >> 0);
                query[3]            = (unsigned char)((block & 0x03FF) >> 8);
                query[4]            = length;

                root.write(query);
                /* Reply is SUBFUNC plus the frame block */
                if (root.read(answer) < length + 1)
                    _E("failed reading frame block, pushing back nonetheless", "");
                if (opCode() != answer[0])
                    _E("failed acking frame block, command is desynced, pushing back nonetheless", "");
                bitmap_frame.insert(bitmap_frame.end(), answer.begin() + 1, answer.end());
            }

            /* FIXME: don't ever try to reuse this command after ask() was called... */
            /* Finish with an invalid address to prevent breaking device sync */
            query.resize(2);
            query[1] = 0xFF;
            root.write(query);
            /* Device replies with opcode only, and subfunc is done */
            answer.resize(1);
            root.read(answer);

            _D("done reading UI frame",0);

            root.unlock();
        }

        return CR_SUCCESS;
    }

  public:
    MGIO_READ_DISPLAY_FRAME() : MGC(IOBuffer{ opCode(), 0x0D, 0, 0, 0 }, IOBuffer(1)), bitmap_frame(frame_size){};
};

#endif /* _3RDPARTY_INDI_MGEN_MGIO_READ_DISPLAY_FRAME_H_ */
