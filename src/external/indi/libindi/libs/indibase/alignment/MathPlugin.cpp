/*!
 * \file MathPlugin.cpp
 *
 * \author Roger James
 * \date 13th November 2013
 *
 */

#include "MathPlugin.h"

namespace INDI
{
namespace AlignmentSubsystem
{
bool MathPlugin::Initialise(InMemoryDatabase *pInMemoryDatabase)
{
    MathPlugin::pInMemoryDatabase = pInMemoryDatabase;
    return true;
}

} // namespace AlignmentSubsystem
} // namespace INDI
