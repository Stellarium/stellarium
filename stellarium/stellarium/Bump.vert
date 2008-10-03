uniform vec3 LightPosition;
varying vec2 TexCoord; 
varying vec3 TangentView;
varying vec3 TangentLight; 
void main(void)
{
   vec3 position = vec3(gl_ModelViewMatrix * gl_Vertex);
   vec3 normal = normalize(gl_NormalMatrix * gl_Normal);  
   vec3 view = normalize(-position);
   vec3 light = normalize(LightPosition - position);
   vec3 binormal = vec3(normal.y,-normal.x,0);
   vec3 tangent = cross(normal,binormal);
   TangentLight = vec3(dot(light, tangent), dot(light, binormal), dot(light, normal));
   TangentView = vec3(dot(view, tangent), dot(view, binormal), dot(view, normal));
   TexCoord = gl_MultiTexCoord0.st;
   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}