#version 420

out vec4 f;
in vec4 gl_Color;
layout(size4x32) uniform image2D imageTexture;
uniform sampler2D imageSampler;
layout(binding=1) uniform sampler2D shadeMe;

float field(vec3 pos, float tval) {
	pos.x /= (720.0 / 1280.0);
	vec3 posrot = pos;

	// Torus
	vec2 q = vec2(length(posrot.xz) - 0.8, posrot.y);
	float dist = length(q) - 0.25;

	return(dist);
}

void main() {
	vec2 v = gl_FragCoord.xy/vec2(1280., 720.);
	float t = gl_Color.y * 3000.0 * 10.0;
	float ms = 0.002;
	
    vec4 temp = texture(shadeMe, v.xy);

	vec2 pd = vec2(ms, 0.0);
	if(gl_FragCoord.y < 30) {
        // Move particles
		vec4 old = texture(imageSampler, v.xy);
        vec3 dir = vec3(0.0, -1.0, 0.0);
		vec4 new = old + vec4(normalize(dir), 0.0) * ms;

        // Block particles
		vec3 pos = new.xyz;
		vec3 displace = vec3(
			field(pos-pd.xyy, t) - field(pos+pd.xyy, t),
			field(pos-pd.yxy, t) - field(pos+pd.yxy, t),
			field(pos-pd.yyx, t) - field(pos+pd.yyx, t)
		);
		if(field(pos, t) > 0.0) {
			new.xyz += normalize(displace) * ms * 2.0;
		}

        // Constrain particles
		new.xyz = mod(new.xyz + vec3(0.5), 1.0) - vec3(0.5);


		imageStore(imageTexture, ivec2(gl_FragCoord.xy), new);
	}

	//float gauss[] = {0.06136, 0.24477, 0.38774, 0.24477, 0.06136};
	float gauss[] = {0.000229, 0.005977, 0.060598, 0.241732, 0.382928, 0.241732, 0.060598, 0.005977, 0.000229};
	float thicc = temp.w;
	temp = vec4(0);
	float transp = 0.0;
	if(thicc != 0) {
		float kernsum = 0;
		for(int x = -4; x <= 4; x++) {
			for(int y = -4; y <= 4; y++) {
				temp += texture(shadeMe, (gl_FragCoord.xy + vec2(x, y)) / vec2(1280., 720.)) * gauss[x + 4] * gauss[y + 4];
				kernsum += gauss[x + 4] * gauss[y + 4];
			}
		}
	
		temp = temp / kernsum;
		transp = clamp((thicc / 30.0), 0.0, 1.0);
	}

	f = vec4(temp.z / 10.0);
}
