#version 420

out vec4 f;
in vec4 gl_Color;

void main() {
	vec2 pos = gl_PointCoord.xy - vec2(0.5);
	float radius = length(pos);
	if(radius > 0.5) {
		discard;
	}
	f = vec4(pos.x, pos.y, 1.0 - length(pos), 1.0);
}
