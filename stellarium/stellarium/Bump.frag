uniform sampler2D TextureMap;
uniform sampler2D NormalMap;
varying vec2 TexCoord;
varying vec3 TangentView;
varying vec3 TangentLight;
void main(void)
{
   vec3 light = normalize(TangentLight);
   vec3 view = normalize(TangentView);
   vec3 normal = 2.0 * vec3(texture2D(NormalMap, TexCoord)) - vec3(1.0);   
   vec3 refl = reflect(-light, normal); 
   float diffuse = max(dot(normal, light), 0.0);
   float specular = pow(max(dot(view, refl), 0.0), 24.0);
   vec3 color = vec3(texture2D(TextureMap, TexCoord));
   gl_FragColor = vec4(diffuse * color, 1.0);
}