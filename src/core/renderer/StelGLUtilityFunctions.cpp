#include <QDebug>

#include "StelFileMgr.hpp"
#include "StelGLUtilityFunctions.hpp"


GLint glAttributeType(const AttributeType type)
{
	switch(type)
	{
		case AttributeType_Vec2f:
		case AttributeType_Vec3f:
		case AttributeType_Vec4f:
			return GL_FLOAT;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute type");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

const char* glslAttributeName(const AttributeInterpretation interpretation)
{
	switch(interpretation)
	{
		case AttributeInterpretation_Position: return "vertex";
		case AttributeInterpretation_TexCoord: return "texCoord";
		case AttributeInterpretation_Normal:   return "normal";
		case AttributeInterpretation_Color:    return "color";
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute interpretation");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return NULL;
}

GLenum gl1AttributeEnum(const AttributeInterpretation interpretation)
{
	switch(interpretation)
	{
		case AttributeInterpretation_Position: return GL_VERTEX_ARRAY;
		case AttributeInterpretation_TexCoord: return GL_TEXTURE_COORD_ARRAY;
		case AttributeInterpretation_Normal:   return GL_NORMAL_ARRAY;
		case AttributeInterpretation_Color:    return GL_COLOR_ARRAY;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown vertex attribute interpretation");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return GL_VERTEX_ARRAY;
}


GLint glPrimitiveType(const PrimitiveType type)
{
	switch(type)
	{
		case PrimitiveType_Points:        return GL_POINTS;
		case PrimitiveType_Triangles:     return GL_TRIANGLES;
		case PrimitiveType_TriangleStrip: return GL_TRIANGLE_STRIP;
		case PrimitiveType_TriangleFan:   return GL_TRIANGLE_FAN;
		case PrimitiveType_Lines:         return GL_LINES;
		case PrimitiveType_LineStrip:     return GL_LINE_STRIP;
		case PrimitiveType_LineLoop:      return GL_LINE_LOOP;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown graphics primitive type");
	
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

GLenum glIndexType(const IndexType indexType)
{
	if(indexType == IndexType_U16)      {return GL_UNSIGNED_SHORT;}
	else if(indexType == IndexType_U32) {return GL_UNSIGNED_INT;}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown index type");
	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

GLint glTextureWrap(const TextureWrap wrap)
{
	switch(wrap)
	{
		case TextureWrap_Repeat:      return GL_REPEAT;
		case TextureWrap_ClampToEdge: return GL_CLAMP_TO_EDGE;
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown texture wrap mode");

	// Prevents GCC from complaining about exiting a non-void function:
	return -1;
}

QString glErrorToString(const GLenum error)
{
	switch(error)
	{
		case GL_NO_ERROR:                      return "GL_NO_ERROR";
		case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE"; 
		case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION"; 
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION"; 
		case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY"; 
	}
	Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown GL error");

	// Prevents GCC from complaining about exiting a non-void function:
	return QString();
}

GLint glGetPixelFormat(const QImage& image)
{
	const bool gray  = image.isGrayscale();
	const bool alpha = image.hasAlphaChannel();
	if(gray) {return alpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;}
	else     {return alpha ? GL_RGBA : GL_RGB;}
}

void glEnsureTextureSizeWithinLimits(QImage& image)
{
	//TODO 
	//GL_MAX_TEXTURE_SIZE is buggy on some implementations,
	//and max texture size also depends on pixel format.
	//Implement a better solution based on texture proxy.
	//(determine if a texture with exactly this width, height, format fits.
	//If not, divide width and height by 2, try again. Ad infinitum, until 0)
	GLint size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);

	if (size < image.width())
	{
		image = image.scaledToWidth(size, Qt::FastTransformation);
	}
	if (size < image.height())
	{
		image = image.scaledToHeight(size, Qt::FastTransformation);
	}
}

//! Utility function that returns a filename with changed file extension.
static QString withFileExtension(const QString& filename, const QString& newExt)
{
	const int lastSep = filename.lastIndexOf('/');
	const int extStart = filename.lastIndexOf('.') + 1;
	// If there's no dot in the filename, just append the extension.
	if(extStart < lastSep) {return filename + "." + newExt;}
	QString result = filename;
	result.replace(extStart, filename.length() - extStart, newExt);
	return result;
}

//! Utility wrapper function to get full path of a file.
//!
//! @param filename File name to get full path for.
//! @param outFullPath Will store full path on success.
//! @param outErrorMessage Will store full path on failure.
//!
//! @return true on success, false on failure.
static bool findTextureFile(const QString& filename, QString& outFullPath,
                            QString& outErrorMessage)
{
	try
	{
		outFullPath = StelFileMgr::findFile(filename);
		return true;
	}
	catch (std::runtime_error er)
	{
		outErrorMessage = er.what();
		return false;
	}
}

QString glFileSystemTexturePath(const QString& filename, const bool pvrSupported)
{
	QString result, errorMessage;

	// Compressed PVR versions are preferred if they exist.
	// If the texture is not found, we look for it in "textures/"
	
	if(pvrSupported)
	{
		const QString pvr1 = withFileExtension(filename, "pvr");
		const QString pvr2 = withFileExtension("textures/" + filename, "pvr");
		if(findTextureFile(pvr1, result, errorMessage)) {return result;}
		if(findTextureFile(pvr2, result, errorMessage)) {return result;}
	}

	if(findTextureFile(filename, result, errorMessage)) {return result;}
	if(findTextureFile("textures/" + filename, result, errorMessage)) {return result;}

	qWarning() << "WARNING : Couldn't find texture file " 
	           << filename << ": " << result;
	return QString();
}

void checkGLErrors()
{
	const GLenum glError = glGetError();
	if(glError == GL_NO_ERROR) {return;}

	qWarning() << "OpenGL error detected: " << glErrorToString(glError);
}
