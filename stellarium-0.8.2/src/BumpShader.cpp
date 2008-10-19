#include "BumpShader.h"

namespace
{
	const char* gVertShaderSource = 
		"                                                                                           \
			uniform vec3 LightPosition;                                                             \
			varying vec2 TexCoord;                                                                  \
			varying vec3 TangentView;                                                               \
			varying vec3 TangentLight;                                                              \
			void main(void)                                                                         \
			{                                                                                       \
				vec3 position = vec3(gl_ModelViewMatrix * gl_Vertex);                               \
				vec3 normal = normalize(gl_NormalMatrix * gl_Normal);                               \
				vec3 view = normalize(-position);                                                   \
				vec3 light = normalize(LightPosition - position);                                   \
				vec3 binormal = vec3(normal.y,-normal.x,0);                                         \
				vec3 tangent = cross(normal,binormal);                                              \
				TangentLight = vec3(dot(light, tangent), dot(light, binormal), dot(light, normal)); \
				TangentView = vec3(dot(view, tangent), dot(view, binormal), dot(view, normal));     \
				TexCoord = gl_MultiTexCoord0.st;                                                    \
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;                             \
			}                                                                                       \
		";

	const char* gFragShaderSource = 
		"                                                                             \
			uniform sampler2D TextureMap;                                             \
			uniform sampler2D NormalMap;                                              \
			varying vec2 TexCoord;                                                    \
			varying vec3 TangentView;                                                 \
			varying vec3 TangentLight;                                                \
			void main(void)                                                           \
			{                                                                         \
				vec3 light = normalize(TangentLight);                                 \
				vec3 view = normalize(TangentView);                                   \
				vec3 normal = 2.0 * vec3(texture2D(NormalMap, TexCoord)) - vec3(1.0); \
				vec3 refl = reflect(-light, normal);                                  \
				float diffuse = max(dot(normal, light), 0.0);                         \
				float specular = pow(max(dot(view, refl), 0.0), 24.0);                \
				vec3 color = vec3(texture2D(TextureMap, TexCoord));                   \
				gl_FragColor = vec4(diffuse * color, 1.0);                            \
			}                                                                         \
		";
}

BumpShader::BumpShader() : 
	SourceShader(gVertShaderSource, gFragShaderSource), mTexMap(0), mBumpMap(0),
	mLightPos(0.0f, 0.0f, 0.0f)
{
	mLightPosLoc = glGetUniformLocation(getProgramObject(), "LightPosition");
	mTexMapLoc = glGetUniformLocation(getProgramObject(), "TextureMap");
	mBumpMapLoc = glGetUniformLocation(getProgramObject(), "NormalMap");
}

BumpShader::~BumpShader()
{
}

void BumpShader::setParams()
{
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexMap);
    glUniform1i(mTexMapLoc, 0);

	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mBumpMap);
    glUniform1i(mBumpMapLoc, 1);

	glUniform3fvARB(mLightPosLoc, 1, mLightPos);
}	

void BumpShader::setTexMap(GLuint texMap)
{
	mTexMap = texMap;
}

void BumpShader::setBumpMap(GLuint bumpMap)
{
	mBumpMap = bumpMap;
}

void BumpShader::setLightPos(const Vec3f& lightPos)
{
	mLightPos = lightPos;
}

BumpShader* BumpShaderPtr::sBumpShader;

BumpShaderPtr::BumpShaderPtr()
{
	static BumpShader bumpShader;
	sBumpShader = &bumpShader;
}

BumpShader* BumpShaderPtr::operator ->()
{
	return sBumpShader;
}