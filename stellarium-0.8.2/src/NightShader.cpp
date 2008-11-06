#include "NightShader.h"
#include "glew.h"

namespace
{
	const char* cVertShaderSource = 
		"                                                               \
			varying vec2 TexCoord;                                      \
			varying vec3 normal;                                        \
			varying vec3 position;                                      \
			void main()                                                 \
			{                                                           \
				position = vec3(gl_ModelViewMatrix * gl_Vertex);        \
				normal = normalize(gl_NormalMatrix * gl_Normal);        \
				TexCoord = gl_MultiTexCoord0.st;                        \
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \
			}                                                           \
		";

	const char* cFragShaderSource = 
		"                                                                             \
			uniform sampler2D DayTexture;                                             \
			uniform sampler2D NightTexture;                                           \
			uniform sampler2D SpecularTexture;                                        \
			uniform vec3 light;                                                       \
			varying vec2 TexCoord;                                                    \
			varying vec3 normal;                                                      \
			varying vec3 position;                                                    \
			void main(void)                                                           \
			{                                                                         \
				vec3 daytime = vec3(texture2D(DayTexture, TexCoord));                 \
				vec3 nighttime = vec3(texture2D(NightTexture, TexCoord));             \
				light = normalize(-position);                                         \
				float diffuse = max(dot(normal, light), 0.0);                         \
				vec3 daycolor = diffuse * daytime;                                    \
				vec3 nightcolor = nighttime + vec3(0.1);                              \
				vec3 color = daycolor;                                                \
				if (diffuse <= 0.1)                                                   \
				{                                                                     \
					color = mix(nightcolor, daycolor, (diffuse + 0.1) * 5.0);         \
				}                                                                     \
				gl_FragColor = vec4(mix(color,vec3(0.3, 0.3, 0.4),(1.0 ) / 4.0),1.0); \
			}                                                                         \
		";
}

NightShader::NightShader() : 
	Shader(cVertShaderSource, cFragShaderSource), mDayTexMap(0), mNightTexMap(0),
	mSpecularTexMap(0), mLightPos(0.0f, 0.0f, 0.0f)
{
}

NightShader::~NightShader()
{
}

void NightShader::setParams(unsigned int programObject, int startActiveTex) const
{
	glActiveTexture(GL_TEXTURE0 + startActiveTex);
    glBindTexture(GL_TEXTURE_2D, mDayTexMap);
	int loc = glGetUniformLocation(programObject, "DayTexture");
    glUniform1i(loc, startActiveTex);

	glActiveTexture(GL_TEXTURE0 + startActiveTex + 1);
    glBindTexture(GL_TEXTURE_2D, mNightTexMap);
	loc = glGetUniformLocation(programObject, "NightTexture");
    glUniform1i(loc, startActiveTex + 1);

	glActiveTexture(GL_TEXTURE0 + startActiveTex + 2);
    glBindTexture(GL_TEXTURE_2D, mSpecularTexMap);
	loc = glGetUniformLocation(programObject, "SpecularTexture");
    glUniform1i(loc, startActiveTex + 2);

	loc = glGetUniformLocation(programObject, "light");
	glUniform3fvARB(loc, 1, mLightPos);
}

int NightShader::getActiveTexCount() const
{
	return 3;
}

void NightShader::setLightPos(const Vec3f& lightPos)
{
	mLightPos = lightPos;
}

void NightShader::setDayTexMap(unsigned int dayTexMap)
{
	mDayTexMap = dayTexMap;
}

void NightShader::setNightTexMap(unsigned int nightTexMap)
{
	mNightTexMap = nightTexMap;
}

void NightShader::setSpecularTexMap(unsigned int specularTexMap)
{
	mSpecularTexMap = specularTexMap;
}

NightShader* NightShader::instance()
{
	static NightShader shader;
	return &shader;
}