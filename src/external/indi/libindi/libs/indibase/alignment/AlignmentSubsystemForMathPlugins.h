
#pragma once

#include "MathPlugin.h"
#include "TelescopeDirectionVectorSupportFunctions.h"

namespace INDI
{
namespace AlignmentSubsystem
{
/*!
 * \class AlignmentSubsystemForMathPlugins
 * \brief This class encapsulates all the alignment subsystem classes that are useful to math plugin implementations.
 * Math plugins should inherit from this class.
 */
class AlignmentSubsystemForMathPlugins : public MathPlugin, public TelescopeDirectionVectorSupportFunctions
{
  public:
    /// \brief Virtual destructor
    virtual ~AlignmentSubsystemForMathPlugins() {}
};

} // namespace AlignmentSubsystem
} // namespace INDI
