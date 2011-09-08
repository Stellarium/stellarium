varying vec3 var_ldir;
varying vec3 var_vdir;

uniform sampler2D tex;
uniform sampler2D nmap;

void main()
{
	vec3 normal = texture2D(nmap, gl_TexCoord[0].xy).xyz * 2.0 - 1.0;
	normal = normalize(normal);

	vec3 ldir = normalize(var_ldir);
	vec3 vdir = normalize(var_vdir);
	vec3 rdir = normalize(-reflect(ldir, normal));

	float diffuse = max(dot(ldir, normal), 0.0);
	vec4 dcol = diffuse * texture2D(tex, gl_TexCoord[0].xy) * gl_FrontMaterial.diffuse;

	float specular = pow(max(dot(rdir, vdir), 0.0), gl_FrontMaterial.shininess);
	vec4 scol = specular * gl_FrontMaterial.specular;

	gl_FragColor = gl_FrontMaterial.ambient * gl_LightModel.ambient + dcol + scol;
}
