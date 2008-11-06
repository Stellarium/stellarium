#include "CloudShader.h"
#include "glew.h"

namespace
{
	const char* cVertShaderSource = 
		"                                                                                           \
			uniform vec3 LightPosition;                                                             \
			varying vec2 TexCoord;                                                                  \
			varying vec3 TangentLight;                                                              \
			void main(void)                                                                         \
			{                                                                                       \
				vec3 position = vec3(gl_ModelViewMatrix * gl_Color);                                \
				vec3 normal = normalize(gl_NormalMatrix * gl_Normal);                               \
				vec3 light = normalize(LightPosition - position);                                   \
				vec3 binormal = vec3(normal.y, -normal.x, 0);                                       \
				vec3 tangent = cross(normal, binormal);                                             \
				TangentLight = vec3(dot(light, tangent), dot(light, binormal), dot(light, normal)); \
				TexCoord = gl_MultiTexCoord0.st;                                                    \
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;                             \
			}                                                                                       \
		";

	const char* cFragShaderSource = 
		"                                                                                 \
			uniform sampler2D cloudTexture;                                               \
			uniform sampler2D cloudShadowTexture;                                         \
			uniform sampler2D NormalTexture;                                              \
			varying vec2 TexCoord;                                                        \
			varying vec3 normal;                                                          \
			varying vec3 TangentLight;                                                    \
			void main(void)                                                               \
			{                                                                             \
				vec3 color = vec3(texture2D(cloudTexture, TexCoord));                     \
				float tempTransp = texture2D(cloudShadowTexture, TexCoord).x;             \
				vec3 light = normalize(TangentLight);                                     \
				vec3 normal = 2.0 * vec3(texture2D(NormalTexture, TexCoord)) - vec3(1.0); \
				float diffuse = max(dot(normal, light), 0.0);		                      \
				if (diffuse <= 0.1)                                                       \
				{                                                                         \
					gl_FragColor = vec4(diffuse * color, tempTransp / 4);                 \
				}                                                                         \
				else                                                                      \
				{                                                                         \
					gl_FragColor = vec4(diffuse * color, tempTransp);                     \
				}                                                                         \
			}                                                                             \
		";
}

CloudShader::CloudShader() : 
	Shader(cVertShaderSource, cFragShaderSource), mTexMap(0), mNormMap(0),
	mShadowMap(0), mLightPos(0.0f, 0.0f, 0.0f)
{
}

CloudShader::~CloudShader()
{
}

void CloudShader::setParams(unsigned int programObject, int startActiveTex) const
{
	glActiveTexture(GL_TEXTURE0 + startActiveTex);
    glBindTexture(GL_TEXTURE_2D, mTexMap);
	int loc = glGetUniformLocation(programObject, "cloudTexture");
    glUniform1i(loc, startActiveTex);

	glActiveTexture(GL_TEXTURE0 + startActiveTex + 1);
    glBindTexture(GL_TEXTURE_2D, mShadowMap);
	loc = glGetUniformLocation(programObject, "cloudShadowTexture");
    glUniform1i(loc, startActiveTex + 1);

	glActiveTexture(GL_TEXTURE0 + startActiveTex + 2);
    glBindTexture(GL_TEXTURE_2D, mNormMap);
	loc = glGetUniformLocation(programObject, "NormalTexture");
    glUniform1i(loc, startActiveTex + 2);

	loc = glGetUniformLocation(programObject, "LightPosition");
	glUniform3fvARB(loc, 1, mLightPos);
}	

int CloudShader::getActiveTexCount() const
{
	return 3;
}

void CloudShader::setTexMap(unsigned int texMap)
{
	mTexMap = texMap;
}

void CloudShader::setShadowMap(unsigned int shadowMap)
{
	mShadowMap = shadowMap;
}

void CloudShader::setNormMap(unsigned int normMap)
{
	mNormMap = normMap;
}

void CloudShader::setLightPos(const Vec3f& lightPos)
{
	mLightPos = lightPos;
}

CloudShader* CloudShader::instance()
{
	static CloudShader shader;
	return &shader;
}