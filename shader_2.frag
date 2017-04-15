﻿#version 420

out vec4 f;
in vec4 gl_Color;
uniform vec2 res;

layout(size4x32,binding=0) uniform image2D imageTexture;
layout(binding=1) uniform sampler2D imageSampler;

layout(size4x32,binding=1) uniform image2D imageTexture2;
layout(binding=2) uniform sampler2D imageSampler2;

layout(size4x32,binding=2) uniform image2D outTexture;
layout(binding=3) uniform sampler2D outSampler;

// Various knobs to twiddle
#define MIN_DIST 0.01
#define STEP_MULTIPLIER 0.9
#define NORMAL_OFFSET 0.01
#define MAX_STEPS 64
#define MAX_STEPS_SHADOW 32
#define SHADOW_OFFSET 0.02
#define SHADOW_HARDNESS 32.0

// Trefoil knot positions
vec3 trefoil(float t) {
	return vec3(
        sin(t) + 2.0 * sin(2.0 * t),
        cos(t) - 2.0 * cos(2.0 * t),
        -sin(3.0 * t)
    );
}

// Distance / color combiner
vec4 distcompose(vec4 dista, vec4 distb, float softness) {
    float mixfact = clamp(0.5 + 0.5 * (distb.a - dista.a) / softness, 0.0, 1.0);
    return mix(distb, dista, mixfact) - vec4(0.0, 0.0, 0.0, softness * mixfact * (1.0 - mixfact));
}

vec3 hash33(vec3 p){ 
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}

// World
vec4 distfunc(vec3 pos) {
    float t = gl_Color.x * 3000.0 * 10.0;
    float fieldselect = gl_Color.y * 65536.0;

  	vec4 box = vec4(0.0);
    box.xyz = vec3(0.3 * pos.y, 0.3, 0.3);
    box.a = min(min(pos.y, -abs(pos.z) + 2.0), -abs(pos.x) + 2.0);;
    
    vec4 dist = box;
    
    if(fieldselect < 0.1) {
        for(int i = 0; i < 15; i++) {
    	    vec3 ballPos = trefoil(t * float(i * 0.2 + 1)) * 0.2;
            if(i % 3 == 1) {
        	    ballPos = ballPos.zxy;  
            }
        
            if(i % 3 == 2) {
        	    ballPos = ballPos.zxy;  
            } 
       	    ballPos.y += 1.2;
       	
            float radius = 0.3;
        
            radius += (sin(pos.x * 40.0) + cos(pos.z * 40.0) + sin(pos.y * 40.0)) * 0.01;
            radius += (hash33(pos) * 0.003).x;
            vec3 col = vec3(radius > 0.33 ? 200.0 : 0.2) * vec3(i * 0.1, 0.2 + (15 - i) * 0.4, i * 0.8);
            vec4 ball = vec4(col, length(ballPos - pos) - radius);
            dist = distcompose(dist, ball, 0.3);
        }
    }

    if(abs(fieldselect - 1.0) < 0.1) {
        for(int i = 0; i < 4; i++) {
            float tx = t * (i + 1) * (i % i == 0 ? -1 : 1);
            vec2 sc = vec2(sin(tx), cos(tx));
            mat3 boxRot = mat3(sc.t, 0.0, sc.s, 0.0, 1.0, 0.0, -sc.s, 0.0, sc.t) *
            mat3(sc.t, sc.s, 0.0, -sc.s, sc.t, 0.0, 0.0, 0.0, 1.0);
            vec3 boxPos =  hash33(vec3(i + 5)) * vec3(0.7, 1.0, 0.7);
            boxPos += vec3(-0.35, 0.0, -0.35);
            boxPos.y = i * 0.3 + 0.6;
            vec3 col = 200.0 * vec3(0.1 + (i * 0.8) / 4.0, 0.4, 0.1 + ((4 - i) / 4.0 * 0.8));

            float roundcube = length(max(abs((pos - boxPos)*boxRot) - 0.1, 0.0)) - 0.08;
            dist = distcompose(dist, vec4(col, roundcube), 0.02);
        }
    }
    return(dist);
}

mat4 lookatmat(vec3 eye, vec3 center, vec3 up) {
	vec3 backward=normalize(eye - center);
	vec3 right=normalize(cross(up,backward));
	vec3 actualup=normalize(cross(backward,right));

	return mat4(
        vec4(right, -dot(right, eye)), 
        vec4(actualup, -dot(actualup, eye)), 
        vec4(backward, -dot(backward, eye)),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

vec3 lookat = vec3(0.0, 1.0, 0.0);
vec3 campos(float t) {
    float wobble = t * 0.2;
    return(vec3(sin(wobble) * -1.75, 2.4, cos(wobble) * -1.75));
}

// Renderer
vec4 pixel(vec2 fragCoord, out float depth) {
    float t = gl_Color.x * 3000.0 * 10.0;

    // Screen -1 -> 1 coordinates
    vec2 coords = (2.0 * fragCoord.xy - res) / res.x;

    // Camera as eye + imaginary screen at a distance
    vec3 eye = campos(t);
    vec3 lightpos = eye;
    lightpos.y += 2.0;
    
    float sinpow = sin(t * 5.0);
    sinpow = sinpow * sinpow;
    
    //eye.x += sinpow * 0.01;

    //vec3 looksfsddir = normalize(lookat - eye);
    eye = (vec4(0.0, 0.0, 0.0, 1.0) * inverse(lookatmat(eye, lookat, vec3(0.0, 1.0, 0.0)))).xyz;
    vec3 lookdir = normalize((vec4(0.0, 0.0, -1.0, 0.0) * inverse(lookatmat(eye, lookat, vec3(0.0, 1.0, 0.0)))).xyz);

    vec3 left = normalize(cross(lookdir, vec3(0.0, 1.0, 0.0)));
    vec3 up = normalize(cross(left, lookdir));
    vec3 lookcenter = eye + lookdir;
	vec3 pixelpos = lookcenter + coords.x * left + coords.y * up;
    vec3 ray = normalize(pixelpos - eye);
    ray += hash33(ray * 0.01) * 0.002;
    
    // March
    vec3 pos = eye;
    float dist = 0.0;
    float curdist = 1.0;
    float iters = float(MAX_STEPS);
    for(int i = 0; i < MAX_STEPS; i++) {
        curdist = distfunc(pos).a;
        dist += curdist * STEP_MULTIPLIER;
        pos = eye + ray * dist;
        if(curdist < MIN_DIST) {
        	iters = float(i);
            break;
        }
    }
    
	// Finite-difference normals
   	vec2 d = vec2(NORMAL_OFFSET, 0.0);
    vec3 normal = normalize(vec3(
        distfunc(pos + d.xyy).a - distfunc(pos - d.xyy).a,
        distfunc(pos + d.yxy).a - distfunc(pos - d.yxy).a,
        distfunc(pos + d.yyx).a - distfunc(pos - d.yyx).a
    ));
    
    // Offset from surface
    vec3 shadowstart = eye + ray * dist + normal * SHADOW_OFFSET;
    vec3 shadowpos = shadowstart;
    
    // Shadow ray
    vec3 shadowray = normalize(lightpos - pos);
    float shadowdist = length(lightpos - pos);
    float penumbra = 1.0;
    dist = 0.0;
    for(int i = 0; i < MAX_STEPS_SHADOW; i++) {
        curdist = distfunc(shadowpos).a;
		dist += curdist * STEP_MULTIPLIER;        
        shadowpos = shadowstart + shadowray * dist;
        
        if(curdist < MIN_DIST) {
            penumbra = 0.0;
            break;
        }
        
        penumbra = min(penumbra, SHADOW_HARDNESS * curdist / dist);
        if(dist >= shadowdist) {;
        	break;   
        }
        
        if(i == MAX_STEPS_SHADOW - 1) {
        	penumbra = 0.0;   
        }
        
    }

    // Shading
    float light = max(0.0, dot(normal, shadowray)) * penumbra + 0.1;
    vec3 colorval = light * distfunc(pos).rgb * (fract(pos).x > 0.5 ? 1.0 : 0.2);
   
    // Calculate CoC (limited to a maximum size) and store
    depth = length(pos - eye);
    vec4 fragColor = vec4(colorval.xyz, 0.0);
    float coc = 0.4 * abs(1.0 - length(eye - lookat) / depth);
    coc = max(0.01 * 5.0, min(0.35 * 5.0, coc));
    return(vec4(fragColor.rgb, coc));
}

// Image
void main() {
    // Screenspace pos 
    vec2 v = gl_FragCoord.xy / res;

    float t = gl_Color.x * 3000.0 * 10.0;

    // Render what?
    if(gl_Color.w < 0.5) {
        // Render box
        f = vec4(0.0);
        float depth;
        for(int i = 0; i < 3; i++) {
    	    f += pixel(gl_FragCoord.xy + hash33(vec3(i)).xy, depth);
        }
        f /= 3.0;

        // Move particles

        // Preload dir here for HACKS reasons
        vec3 dir = texture(imageSampler2, v.xy).xyz;

        if(gl_FragCoord.y <= 10) {
            vec3 eye = campos(t);
            mat4 cam = lookatmat(eye, lookat, vec3(0.0, 1.0, 0.0));
            vec3 offset = vec3(0.0, 2.0, 0.0);

	        float ms = gl_Color.z * 10000.0;

            // Update particles
		    vec4 old = texture(imageSampler, v.xy);
		    vec4 new = old + vec4(dir, 0.0) * ms;

            // Block particles
		    vec3 pos = new.xyz + offset;
	        vec2 pd = vec2(ms, 0.0);
		    vec3 displace = vec3(
			    distfunc(pos-pd.xyy).w - distfunc(pos+pd.xyy).w,
			    distfunc(pos-pd.yxy).w - distfunc(pos+pd.yxy).w,
			    distfunc(pos-pd.yyx).w - distfunc(pos+pd.yyx).w
		    );
		    if(distfunc(pos).w < 0.0) {
			    new.xyz -= normalize(displace) * ms * 2.0;
                dir = reflect(dir, normalize(displace)) * 0.8;
		    }

            // Constrain particles
            dir -= vec3(0.0, ms * 10.0, 0.0);
            if(length(dir) > ms * 1000.0) {
                dir = normalize(dir) * ms * 1000.0;
            }
		    //new.xyz = mod(new.xyz + vec3(2.0), 4.0) - vec3(2.0);
		    imageStore(imageTexture, ivec2(gl_FragCoord.xy), new);

            vec3 outPos = (vec4(new.xyz + offset, 1.0) * cam).xyz;
            imageStore(outTexture, ivec2(gl_FragCoord.xy), vec4(outPos, length(outPos)));
        }

        // Direction of particle but also depth of fragment in w. Sorry, future reader.
        imageStore(imageTexture2, ivec2(gl_FragCoord.xy), vec4(dir, depth + 0.01));
    }
    else {
        // Frag depths
        float origDepth = texture(imageSampler2, v.xy).w;
        float partDepth = gl_Color.x * 1000.0;

        // Render particles
        vec2 pos = gl_PointCoord.xy - vec2(0.5);
	    float radius = length(pos);
	    if(radius > 0.5 || origDepth < partDepth) {
		    discard;
	    }

        vec3 col = vec3(0.9, 0.9, 0.9);

        float coc = 0.8 * abs(0.8 / partDepth);
        coc = max(0.01 * 5.0, min(0.35 * 5.0, coc));

        f = vec4(col * (0.5 - radius), coc);
    }
}