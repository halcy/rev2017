#version 420

out vec4 f;
in vec4 gl_Color;
uniform sampler2D postproc;
uniform vec2 res;

vec3 hash33(vec3 p){ 
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}

// blur with hexagonalish sampling pattern
// weighs samples according to coc size (so that "in focus" samples count for less)
// and according to tap nb (weighs outer samples higher)
vec3 hexablur(sampler2D tex, vec2 uv) {
    vec2 scale = vec2(1.0) / res;
    vec3 col = vec3(0.0);
    float asum = 0.0;
    float coc = texture(tex, uv).a;
    for(float t = 0.0; t < 8.0 * 2.0 * 3.14; t += 3.14 / 32.0) {
    	float r = cos(3.14 / 6.0) / cos(mod(t, 2.0 * 3.14 / 6.0) - 3.14 / 6.0);
        
        // Tap filter once for coc
        vec2 offset = vec2(sin(t), cos(t)) * r * t * scale * coc;
        vec4 samp = texture(tex, uv + offset * 1.0);
        
        // Tap filter with coc from texture
        offset = vec2(sin(t), cos(t)) * r * t * scale * samp.a;
        samp = texture(tex, uv + offset * 1.0);
        
        // weigh and save
        col += samp.rgb * samp.a * t;
        asum += samp.a * t;
        
    }
    col = col / asum;
    return(col);
}

float atten (float v) { return 1.0 - pow( abs( ( v - .5 ) * 1.9), 6.0 ); }

void main() {
	float t = gl_Color.x * 3000.0 * 50.0;

    f = vec4(1.0, 0.0, 0.2, 0.0);
	vec2 uv = gl_FragCoord.xy/res;
    f = vec4(hexablur(postproc, uv), 0.0);
    // f = texture(postproc, uv);

    // Tonemap and gamma-correct
    f = f / (f + vec4(1.0));
    f.rgb = vec3(pow(f.r, 1.0 / 2.2), pow(f.g, 1.0 / 2.2), pow(f.b, 1.0 / 2.2));
   
    float sinpow = sin(t * 11.0);
    sinpow = sinpow * sinpow;
    f.rgb *= 0.92 + 0.08 * (sinpow + 1.0) / 2.0;
	f.rgb *= (atten( uv.x ) + atten( uv.y )) * 0.5;
    
    float transition = (1.0 - gl_Color.z);
    if(abs(gl_Color.y * 100.0 - 3.0) < 0.1) {
        transition = 0.0;
    }
    vec3 transnoise = vec3(0.1, 0.1, 0.2) * hash33(vec3(uv.x, uv.y, t) / 8.0).x;
    f.rgb = mix(transnoise, f.rgb, transition);
}