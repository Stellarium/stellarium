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

#ifndef _3RDPARTY_INDI_MGEN_MGEN_H_
#define _3RDPARTY_INDI_MGEN_MGEN_H_

/*
 * mgen.h
 *
 *  Created on: 8 mars 2017
 *      Author: TallFurryMan
 */

#include <exception>
#include <string>
#include <typeinfo>

/** \brief A protocol mode in which the command is valid */
enum IOMode
{
    OPM_UNKNOWN,     /*!< Unknown mode, no exchange done yet or connection error */
    OPM_COMPATIBLE,  /*!< Compatible mode, to check device state */
    OPM_APPLICATION, /*!< Normal applicative mode */
};

/** \brief The result of a command */
enum IOResult
{
    CR_SUCCESS, /*!< Command is successful, result is available through helpers or in field 'answer' */
    CR_FAILURE, /*!< Command is not successful, no acknowledge or unexpected data returned */
};

/** \brief Exception returned when there is I/O malfunction with the device */
class IOError : std::exception
{
  protected:
    std::string const _what;

  public:
    virtual const char *what() const noexcept { return _what.c_str(); }
    IOError(int code) : std::exception(), _what(std::string("I/O error code ") + std::to_string(code)){};
    virtual ~IOError(){};
};

/** \brief One word in the I/O protocol */
typedef unsigned char IOByte;

/** \brief A buffer of protocol words */
typedef std::vector<IOByte> IOBuffer;

/** \internal Logging helpers */
/** @{ */
#define _S(msg, ...)                                                                                                   \
    INDI::Logger::getInstance().print(MGenAutoguider::instance().getDeviceName(), INDI::Logger::DBG_SESSION, __FILE__, \
                                      __LINE__, "%s::%s: " msg, __FUNCTION__, typeid(*this).name(), __VA_ARGS__)
#define _D(msg, ...)                                                                                                 \
    INDI::Logger::getInstance().print(MGenAutoguider::instance().getDeviceName(), INDI::Logger::DBG_DEBUG, __FILE__, \
                                      __LINE__, "%s::%s: " msg, __FUNCTION__, typeid(*this).name(), __VA_ARGS__)
#define _E(msg, ...)                                                                                                 \
    INDI::Logger::getInstance().print(MGenAutoguider::instance().getDeviceName(), INDI::Logger::DBG_ERROR, __FILE__, \
                                      __LINE__, "%s::%s: " msg, __FUNCTION__, typeid(*this).name(), __VA_ARGS__)
/** @} */

class MGenAutoguider;
class MGenDevice;

#endif /* _3RDPARTY_INDI_MGEN_MGEN_H_ */
