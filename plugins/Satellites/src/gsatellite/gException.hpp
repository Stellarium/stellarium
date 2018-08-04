/***************************************************************************
 *   Copyright (C) 2006 by j. l. Canales                                   *
 *   jlcanales@users.sourceforge.net                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.             *
 ***************************************************************************/

#ifndef GEXCEPTION_HPP
#define GEXCEPTION_HPP

#include <stdexcept>

//Exception Codes
#define EXCP_UNKNOWN             1
#define EXCP_INCORRECTPARAM      2
#define EXCP_OPERATIONOVERFLOW   3
#define EXCP_MEMORYALLOCATION    4
#define EXCP_COMPUTEOVERFLOW     5

typedef unsigned int GEXCEPTIONTYPE;

// Exception messages for GMatrix

#define OPERATOR_X_INCOMPATIBLE_ORDER         "The matrix have incompatible order to calculate its verctorial product"
#define OPERATOR_ADD_INCOMPATIBLE_ORDER       "The matrix have incompatible order to calculate the addition"
#define OPERATOR_ADDEQUAL_INCOMPATIBLE_ORDER  "The matrix have incompatible order to calculate the addition"
#define OPERATOR_DIFF_INCOMPATIBLE_ORDER      "The matrix have incompatible order to calculate the substraction"
#define OPERATOR_DIFFEQUAL_INCOMPATIBLE_ORDER "The matrix have incompatible order to calculate the substraction"
#define DET_INCOMPATIBLE_ORDER                "The matrix is not square, the determinant can't be calculate."

#endif // GEXCEPTION_HPP
