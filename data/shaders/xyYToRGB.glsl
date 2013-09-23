const highp float pi = 3.1415926535897931;
const highp float ln10 = 2.3025850929940459;

// Variable for the xyYTo RGB conversion
uniform highp float alphaWaOverAlphaDa;
uniform highp float oneOverGamma;
uniform highp float term2TimesOneOverMaxdLpOneOverGamma;
uniform highp float brightnessScale;

// Variables for the color computation
uniform highp vec3 sunPos;
uniform mediump float term_x, Ax, Bx, Cx, Dx, Ex;
uniform mediump float term_y, Ay, By, Cy, Dy, Ey;

// The current projection matrix
uniform mediump mat4 projectionMatrix;

// Contains the 2d position of the point on the screen (before multiplication by the projection matrix)
attribute mediump vec2 skyVertex;

// Contains the r,g,b,Y (luminosity) components.
attribute highp vec4 skyColor;

// The output variable passed to the fragment shader
varying mediump vec4 resultSkyColor;

void main()
{
	gl_Position = projectionMatrix*vec4(skyVertex, 0., 1.);
	highp vec4 color = skyColor;

	///////////////////////////////////////////////////////////////////////////
	// First compute the xy color component
	// color contains the unprojected vertex position in r,g,b
	// + the Y (luminance) component of the color in the alpha channel
	if (color[3]>0.01)
	{
		highp float cosDistSunq = sunPos[0]*color[0] + sunPos[1]*color[1] + sunPos[2]*color[2];
		highp float distSun=acos(cosDistSunq);
		highp float oneOverCosZenithAngle = (color[2]==0.) ? 9999999999999. : 1. / color[2];

		cosDistSunq*=cosDistSunq;
		color[0] = term_x * (1. + Ax * exp(Bx*oneOverCosZenithAngle))* (1. + Cx * exp(Dx*distSun) + Ex * cosDistSunq);
		color[1] = term_y * (1. + Ay * exp(By*oneOverCosZenithAngle))* (1. + Cy * exp(Dy*distSun) + Ey * cosDistSunq);
		if (color[0] < 0. || color[1] < 0.)
		{
			color[0] = 0.25;
			color[1] = 0.25;
		}
	}
	else
	{
		color[0] = 0.25;
		color[1] = 0.25;
	}
	color[2]=color[3];
	color[3]=1.;


	///////////////////////////////////////////////////////////////////////////
	// Now we have the xyY components in color, need to convert to RGB

	// 1. Hue conversion
	// if log10Y>0.6, photopic vision only (with the cones, colors are seen)
	// else scotopic vision if log10Y<-2 (with the rods, no colors, everything blue),
	// else mesopic vision (with rods and cones, transition state)
	if (color[2] <= 0.01)
	{
		// special case for s = 0 (x=0.25, y=0.25)
		color[2] *= 0.5121445;
		color[2] = pow(color[2]*pi*0.0001, alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;
		color[0] = 0.787077*color[2];
		color[1] = 0.9898434*color[2];
		color[2] *= 1.9256125;
		resultSkyColor = color*brightnessScale;
	}
	else
	{
		if (color[2]<3.9810717055349722)
		{
			// Compute s, ratio between scotopic and photopic vision
			float op = (log(color[2])/ln10 + 2.)/2.6;
			float s = op * op *(3. - 2. * op);
			// Do the blue shift for scotopic vision simulation (night vision) [3]
			// The "night blue" is x,y(0.25, 0.25)
			color[0] = (1. - s) * 0.25 + s * color[0];	// Add scotopic + photopic components
			color[1] = (1. - s) * 0.25 + s * color[1];	// Add scotopic + photopic components
			// Take into account the scotopic luminance approximated by V [3] [4]
			float V = color[2] * (1.33 * (1. + color[1] / color[0] + color[0] * (1. - color[0] - color[1])) - 1.68);
			color[2] = 0.4468 * (1. - s) * V + s * color[2];
		}

		// 2. Adapt the luminance value and scale it to fit in the RGB range [2]
		// color[2] = std::pow(adaptLuminanceScaled(color[2]), oneOverGamma);
		color[2] = pow(color[2]*pi*0.0001, alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;

		// Convert from xyY to XZY
		// Use a XYZ to Adobe RGB (1998) matrix which uses a D65 reference white
		mediump vec3 tmp = vec3(color[0] * color[2] / color[1], color[2], (1. - color[0] - color[1]) * color[2] / color[1]);
		resultSkyColor = vec4(2.04148*tmp.x-0.564977*tmp.y-0.344713*tmp.z, -0.969258*tmp.x+1.87599*tmp.y+0.0415557*tmp.z, 0.0134455*tmp.x-0.118373*tmp.y+1.01527*tmp.z, 1.);
		resultSkyColor*=brightnessScale;
	}
}
