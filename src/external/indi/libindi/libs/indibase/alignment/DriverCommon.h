/*!
 * \file DriverCommon.h
 *
 * \author Roger James
 * \date 28th January 2014
 *
 */

#pragma once

#include "indilogger.h"

namespace INDI
{
namespace AlignmentSubsystem
{
#define ASSDEBUG(msg) INDI::Logger::getInstance().print("alignmentSubsystem", DBG_ALIGNMENT, __FILE__, __LINE__, msg)
#define ASSDEBUGF(msg, ...) \
    INDI::Logger::getInstance().print("AlignmentSubsystem", DBG_ALIGNMENT, __FILE__, __LINE__, msg, __VA_ARGS__)

extern int DBG_ALIGNMENT;

} // namespace AlignmentSubsystem
} // namespace INDI
