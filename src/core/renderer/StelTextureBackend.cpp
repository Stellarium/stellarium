#include "StelTextureBackend.hpp"


QString textureStatusName(const TextureStatus status)
{
	switch(status)
	{
		case TextureStatus_Uninitialized: return "TextureStatus_Uninitialized"; break;
		case TextureStatus_Loaded:        return "TextureStatus_Loaded"; break;
		case TextureStatus_Loading:       return "TextureStatus_Loading"; break;
		case TextureStatus_Error:         return "TextureStatus_Error"; break;
		default: Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture status");
	}
	// Avoid compiler warnings.
	return QString();
}


