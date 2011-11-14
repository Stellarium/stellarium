/*
 * Stellarium
 * Copyright (C) 2011 Eleni Maria Stea
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

varying vec3 var_ldir;
varying vec3 var_vdir;
varying vec3 var_norm;
varying vec3 pos;

uniform sampler2D tex;
uniform sampler2D nmap;
uniform sampler2D permap;

uniform vec4 ambient;
uniform vec4 diffuse;

uniform vec3 ccolor;
uniform float cdensity;
uniform float cscale;
uniform float csharp;

uniform float pixw;
uniform float halfpixw;

uniform float t;
uniform vec3 cvel; 
//cvel[0] = cloud velocity x, cvel[1] = cloud velocity y, cvel[2] = cloud animation speed

float cloudCurve(float v, float cover, float sharpness);
float perlinNoise(vec3 p);
float fractalNoise(vec3 p);

void main()
{
	vec3 normal = texture2D(nmap, gl_TexCoord[0].xy).xyz * 2.0 - 1.0;
	normal = normalize(normal);

	vec3 ldir = normalize(var_ldir);
	vec3 vdir = normalize(var_vdir);
	vec3 rdir = normalize(-reflect(ldir, normal));

	float diffusel = max(dot(ldir, normal), 0.0);
	vec4 dcol = diffusel * texture2D(tex, gl_TexCoord[0].xy) * diffuse;

	//vec3 texC = vec3(mod((gl_TexCoord[0].x + t * cvel.x), 1.0), mod((gl_TexCoord[0].y + t * cvel.y), 1.0), (gl_TexCoord[0].z + t * cvel.z)) * cscale;

	vec3 rotpos = pos;

	float pn = fractalNoise(rotpos * 10000.0 * cscale) * 0.5 + 0.5;
	float cloud = cloudCurve(pn, cdensity, csharp);
	vec3 ccol = ccolor * max(dot(ldir, normal), 0.0);

	//vec3 color = (ambient * gl_LightModel.ambient).xyz + dcol.xyz;
	vec3 color = dcol.xyz;
	color = mix(color, ccol, cloud);

	gl_FragColor = vec4(color, 1.0);
}

float cloudCurve(float v, float cover, float sharpness)
{
	float c = max(cover - v, 0.0);
	return 1.0 - pow(1.0 - sharpness, c);
}

float fractalNoise(vec3 p)
{
	return perlinNoise(p) +
		perlinNoise(p * 2.0) / 2.0 +
		perlinNoise(p * 4.0) / 4.0 +
		perlinNoise(p * 8.0) / 8.0 +
		perlinNoise(p * 16.0) / 16.0;
}


float fade(float t)
{
	return t * t * (3.0 - 2.0 * t);
	/* return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); */
}


float perlinNoise(vec3 p)
{
	vec3 pi = pixw * floor(p) + halfpixw; /* Integer part, scaled so +1 moves one texel */
										/* and offset 1/2 texel to sample texel centers */
	vec3 pf = fract(p);		/* Fractional part for interpolation */

	float perm00 = texture2D(permap, pi.xy).a ;
	vec3 grad000 = texture2D(permap, vec2(perm00, pi.z)).rgb * 4.0 - 1.0;
	float n000 = dot(grad000, pf);
	vec3 grad001 = texture2D(permap, vec2(perm00, pi.z + pixw)).rgb * 4.0 - 1.0;
	float n001 = dot(grad001, pf - vec3(0.0, 0.0, 1.0));

	float perm01 = texture2D(permap, pi.xy + vec2(0.0, pixw)).a ;
	vec3 grad010 = texture2D(permap, vec2(perm01, pi.z)).rgb * 4.0 - 1.0;
	float n010 = dot(grad010, pf - vec3(0.0, 1.0, 0.0));
	vec3 grad011 = texture2D(permap, vec2(perm01, pi.z + pixw)).rgb * 4.0 - 1.0;
	float n011 = dot(grad011, pf - vec3(0.0, 1.0, 1.0));

	float perm10 = texture2D(permap, pi.xy + vec2(pixw, 0.0)).a ;
	vec3 grad100 = texture2D(permap, vec2(perm10, pi.z)).rgb * 4.0 - 1.0;
	float n100 = dot(grad100, pf - vec3(1.0, 0.0, 0.0));
	vec3 grad101 = texture2D(permap, vec2(perm10, pi.z + pixw)).rgb * 4.0 - 1.0;
	float n101 = dot(grad101, pf - vec3(1.0, 0.0, 1.0));

	float perm11 = texture2D(permap, pi.xy + vec2(pixw, pixw)).a ;
	vec3 grad110 = texture2D(permap, vec2(perm11, pi.z)).rgb * 4.0 - 1.0;
	float n110 = dot(grad110, pf - vec3(1.0, 1.0, 0.0));
	vec3 grad111 = texture2D(permap, vec2(perm11, pi.z + pixw)).rgb * 4.0 - 1.0;
	float n111 = dot(grad111, pf - vec3(1.0, 1.0, 1.0));

	vec4 n_x = mix(vec4(n000, n001, n010, n011), vec4(n100, n101, n110, n111), fade(pf.x));

	vec2 n_xy = mix(n_x.xy, n_x.zw, fade(pf.y));

	float n_xyz = mix(n_xy.x, n_xy.y, fade(pf.z));

	return n_xyz;
}
