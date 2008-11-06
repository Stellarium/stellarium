#include "BumpShader.h"
#include "glew.h"

namespace
{
	const char* cVertShaderSource = 
		"                                                                                           \
			uniform vec3 LightPosition;                                                             \
			varying vec2 TexCoord;                                                                  \
			varying vec3 TangentView;                                                               \
			varying vec3 TangentLight;                                                              \
			void main(void)                                                                         \
			{                                                                                       \
				vec3 position = vec3(gl_ModelViewMatrix * gl_Color);                                \
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

	const char* cFragShaderSource = 
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
	Shader(cVertShaderSource, cFragShaderSource), mTexMap(0), mNormMap(0),
	mLightPos(0.0f, 0.0f, 0.0f)
{
}

BumpShader::~BumpShader()
{
}

void BumpShader::setParams(unsigned int programObject, int startActiveTex) const
{
	glActiveTexture(GL_TEXTURE0 + startActiveTex);
    glBindTexture(GL_TEXTURE_2D, mTexMap);
	int loc = glGetUniformLocation(programObject, "TextureMap");
    glUniform1i(loc, startActiveTex);

	glActiveTexture(GL_TEXTURE0 + startActiveTex + 1);
    glBindTexture(GL_TEXTURE_2D, mNormMap);
	loc = glGetUniformLocation(programObject, "NormalMap");
    glUniform1i(loc, startActiveTex + 1);

	loc = glGetUniformLocation(programObject, "LightPosition");
	glUniform3fvARB(loc, 1, mLightPos);
}	

int BumpShader::getActiveTexCount() const
{
	return 2;
}

void BumpShader::setTexMap(unsigned int texMap)
{
	mTexMap = texMap;
}

void BumpShader::setNormMap(unsigned int normMap)
{
	mNormMap = normMap;
}

void BumpShader::setLightPos(const Vec3f& lightPos)
{
	mLightPos = lightPos;
}

BumpShader* BumpShader::instance()
{
	static BumpShader shader;
	return &shader;
}