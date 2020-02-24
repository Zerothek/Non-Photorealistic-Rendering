#version 410

layout(location = 0) in vec2 texture_coord;

uniform sampler2D textureImage;
uniform ivec2 screenSize;
uniform int flipVertical;

// 0 - original
// 1 - grayscale
// 2 - blur
uniform int outputMode = 2;

// Flip texture horizontally when
vec2 textureCoord = vec2(texture_coord.x, (flipVertical != 0) ? 1 - texture_coord.y : texture_coord.y);

layout(location = 0) out vec4 out_color;

mat3 Dx = mat3(-1, 0, +1,
			-2, 0, +2,
			-1, 0, +1);

mat3 Dy = mat3(+1, +2, +1,
			0, 0, 0,
			-1, -2, -1);

vec4 grayscale()
{
	vec4 color = texture(textureImage, textureCoord);
	float gray = 0.21 * color.r + 0.71 * color.g + 0.07 * color.b; 
	return vec4(gray, gray, gray,  0);
}

vec4 sobel(float threshold)
{
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum1 = vec4(0), sum2 = vec4(0);
	for(int i = -1; i < 2; i++)
	{
		for(int j = -1; j < 2; j++)
		{
			sum1 += Dx[i+1][j+1] * texture(textureImage, textureCoord + vec2(i, j) * texelSize);
			sum2 += Dy[i+1][j+1] * texture(textureImage, textureCoord + vec2(i, j) * texelSize);
		}
	}
		
	vec4 Dxy = abs(sum1) + abs(sum2);


	if (Dxy.x > threshold) {
		Dxy.x = 1;
	} else {
		Dxy.x = texture(textureImage, textureCoord).x;
		Dxy.x = 0;
	}

	if (Dxy.y > threshold) {
		Dxy.y = 1;
	} else {
		Dxy.y = texture(textureImage, textureCoord).y;
		Dxy.y = 0;
	}

	if (Dxy.z > threshold) {
		Dxy.z = 1;
	} else {
		Dxy.z = texture(textureImage, textureCoord).z;
		Dxy.z = 0;
	}

	return Dxy;
}

vec4 sobel_coordinates(float threshold, int x, int y)
{
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum1 = vec4(0), sum2 = vec4(0);
	for(int i = -1; i < 2; i++)
	{
		for(int j = -1; j < 2; j++)
		{
			sum1 += Dx[i+1][j+1] * texture(textureImage, textureCoord + vec2(i + x, j + y) * texelSize);
			sum2 += Dy[i+1][j+1] * texture(textureImage, textureCoord + vec2(i + x, j + y) * texelSize);
		}
	}
		
	vec4 Dxy = abs(sum1) + abs(sum2);


	if (Dxy.x > threshold) {
		Dxy.x = 1;
	} else {
		Dxy.x = texture(textureImage, textureCoord).x;
		Dxy.x = 0;
	}

	if (Dxy.y > threshold) {
		Dxy.y = 1;
	} else {
		Dxy.y = texture(textureImage, textureCoord).y;
		Dxy.y = 0;
	}

	if (Dxy.z > threshold) {
		Dxy.z = 1;
	} else {
		Dxy.z = texture(textureImage, textureCoord).z;
		Dxy.z = 0;
	}

	return Dxy;
}

vec4 sobel_dilation(float threshold)
{
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum1 = vec4(0), sum2 = vec4(0);
	for(int i = -1; i < 2; i++)
	{
		for(int j = -1; j < 2; j++)
		{
			sum1 += Dx[i+1][j+1] * texture(textureImage, textureCoord + vec2(i, j) * texelSize);
			sum2 += Dy[i+1][j+1] * texture(textureImage, textureCoord + vec2(i, j) * texelSize);
		}
	}
		
	vec4 Dxy = abs(sum1) + abs(sum2);


	if (Dxy.x > threshold) {
		Dxy.x = 1;
	} else {
		Dxy.x = texture(textureImage, textureCoord).x;
		Dxy.x = 0;
	}

	if (Dxy.y > threshold) {
		Dxy.y = 1;
	} else {
		Dxy.y = texture(textureImage, textureCoord).y;
		Dxy.y = 0;
	}

	if (Dxy.z > threshold) {
		Dxy.z = 1;
	} else {
		Dxy.z = texture(textureImage, textureCoord).z;
		Dxy.z = 0;
	}


	//DILATION
	vec4 up  = sobel_coordinates(threshold, -1, 0);
	vec4 down  = sobel_coordinates(threshold, 1, 0);
	vec4 left  = sobel_coordinates(threshold, 0, -1);
	vec4 right  = sobel_coordinates(threshold, 0, 1);

	vec4 out_rgb = up + down + left + right + Dxy;

	if (out_rgb.x > 1)
		out_rgb.x = 1;

	if (out_rgb.y > 1)
		out_rgb.y = 1;

	if (out_rgb.z > 1)
		out_rgb.z = 1;

	return out_rgb;
}

vec4 blur(int blurRadius)
{
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum = vec4(0);
	for(int i = -blurRadius; i <= blurRadius; i++)
	{
		for(int j = -blurRadius; j <= blurRadius; j++)
		{
			sum += texture(textureImage, textureCoord + vec2(i, j) * texelSize);
		}
	}
		
	float samples = pow((2 * blurRadius + 1), 2);
	return sum / samples;
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 rgb2hsv(vec3 rgb) {
 	float Cmax = max(rgb.r, max(rgb.g, rgb.b));
 	float Cmin = min(rgb.r, min(rgb.g, rgb.b));
 	float delta = Cmax - Cmin;

 	vec3 hsv = vec3(0., 0., Cmax);

 	if (Cmax > Cmin) {
 		hsv.y = delta / Cmax;

 		if (rgb.r == Cmax)
 			hsv.x = (rgb.g - rgb.b) / delta;
 		else {
 			if (rgb.g == Cmax)
 				hsv.x = 2. + (rgb.b - rgb.r) / delta;
 			else
 				hsv.x = 4. + (rgb.r - rgb.g) / delta;
 		}
 		hsv.x = fract(hsv.x / 6.);
 	}
 	return hsv;
 }

vec4 color_interval() {
	vec3 color_rgb = texture(textureImage, textureCoord).xyz;
	vec3 color_hsv = rgb2hsv(color_rgb);

	float hue = color_hsv.x, saturation = color_hsv.y, value = color_hsv.z;

	if ((hue >= 0 && hue <= float(30.0/360.0)) || (hue >= float(300.0/360.0) && hue <= 360.0/360.0)) {
		hue = 0;
	} else if (hue > 30.0/360.0 && hue <= 60.0/360.0) {
		hue = 30.0/360.0;
	} else if (hue > 60.0/360.0 && hue <= 90.0/360.0) {
		hue = 60.0/360.0;
	} else if (hue > 90.0/360.0 && hue <= 150.0/360.0) {
		hue = 120.0/360.0;
	} else if (hue > 150.0/360.0 && hue <= 240.0/360.0) {
		hue = 240.0/360.0;
	} else if (hue > 240.0/360.0 && hue < 300.0/360.0) {
		hue = 270.0/360.0;
	}

	if (saturation < 0.05) {
		return vec4(1);
	}

	if (value < 0.2) {
		return vec4(0);
	}

	color_hsv = vec3(1);
	color_hsv.x = hue;
	return vec4(hsv2rgb(color_hsv), 0);
}

void main()
{
	vec4 black = vec4(0, 0, 0, 0);
	switch (outputMode)
	{
		case 1:
		{
			vec4 edge = sobel(0.7);
			if (edge.xyz == black.xyz) {
				out_color = texture(textureImage, textureCoord);
			} else {
				out_color = edge;
			}
			//out_color = sobel(0.7);
			break;
		}

		case 2:
			{
			vec4 edge = sobel_dilation(0.7);
			if (edge.xyz == black.xyz) {
				out_color = texture(textureImage, textureCoord);
			} else {
				out_color = edge;
			}
			break;
		}

		case 3:
		{
			out_color = color_interval();
			break;
		}

		default:
			out_color = texture(textureImage, textureCoord);
			break;
	}
}