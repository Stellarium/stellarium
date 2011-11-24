uniform mat4 PVM;
uniform mat4 tex_mat;

uniform vec4 ambient_color;
uniform vec4 diffuse_color;
uniform vec4 light_position;

attribute vec4 vertex;
attribute vec4 color;
attribute vec3 normal;
attribute vec2 texCoord;

varying vec4 SM_tex_coord;
varying vec2 fragTexCoord;

varying vec3 ambient;
varying vec3 diffuse;

void main(void)
{
vec3 norm = normalize(gl_NormalMatrix * normal);
vec3 position = (gl_ModelViewMatrix * vertex).xyz;

vec3 light_dir = normalize(light_position.xyz-position);

float NdotL = dot(norm, light_dir);

vec3 ambient = ambient_color.xyz;
vec3 diffuse = vec3(1.0, 1.0, 1.0) * diffuse_color.xyz * max(0.0, NdotL);

//Pass texture coordinates
fragTexCoord = texCoord;

gl_Position = PVM * vertex;
SM_tex_coord = tex_mat * vertex;
}