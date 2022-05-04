//Gbuffer rendering vertex shader

#version 430

uniform sampler2D renderBuffer;
uniform sampler2D bloomBuffer;

uniform int drawFbo;

uniform int width, height;
uniform float exposure;
uniform float tone_mapping_mode;
uniform float gamma;

uniform int bloomEnabled;

layout(location = 0) out vec4 out_color;

void main() {
	vec2 uv = gl_FragCoord.xy / vec2(width, height);
	vec4 fragColor = texture(renderBuffer, uv);
	vec4 bloomColor = texture(bloomBuffer, uv);
	if (drawFbo < 13){
		out_color = fragColor;
		return;
	}
	if (drawFbo == 13){
		out_color = texture(bloomBuffer, uv) * 10;
	}
	else {
		if (bloomEnabled == 1)
			fragColor = fragColor + bloomColor;

		if (tone_mapping_mode == 0)
			fragColor.rgb = (exposure*fragColor.rgb)/(exposure*fragColor.rgb + 1);
		else
			fragColor.rgb = vec3(1.0) - exp(-fragColor.rgb*exposure);

		//Convert color back into sRGB space
		fragColor.rgb = pow(fragColor.rgb, vec3(1.0f/gamma));
		out_color = vec4(fragColor.rgb, 1.0);
	}
}