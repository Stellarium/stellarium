varying vec4 SM_tex_coord;
varying vec4 frag_color;
varying vec3 world_normal;
varying vec4 world_position;
varying vec2 fragTexCoord;

uniform sampler2D tex;
uniform sampler2D smap;

uniform vec4 ambient_color;
uniform vec4 diffuse_color;
uniform vec4 light_position;

void main()
{
vec3 normal = normalize(world_normal);
vec3 position = world_position.xyz/world_position.w;

vec3 aux = light_position.xyz-position;
float dist = length(aux);
vec3 light_dir = normalize(aux);

vec3 ambient = ambient_color.xyz;

float NdotL = dot(normal, light_dir);

vec3 diffuse = frag_color.xyz * diffuse_color.xyz * max(0.0, NdotL);

vec3 tex_coords = SM_tex_coord.xyz/SM_tex_coord.w;

float depth = texture2D(smap, tex_coords.xy).x;

float factor = 0.0;

if(depth > tex_coords.z)
{
factor = 1.0;
}

gl_FragColor = vec4(texture2D(tex, fragTexCoord).xyz * (ambient + diffuse * factor), 1.0);
}