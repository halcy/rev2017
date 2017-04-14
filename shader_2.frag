#version 420

out vec4 f;
in vec4 gl_Color;
uniform vec2 res;

layout(size4x32,binding=0) uniform image2D imageTexture;
layout(binding=1) uniform sampler2D imageSampler;

layout(size4x32,binding=1) uniform image2D imageTexture2;
layout(binding=2) uniform sampler2D imageSampler2;

layout(size4x32,binding=2) uniform image2D outTexture;

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

  	vec4 box = vec4(0.0);
    box.xyz = vec3(0.3 * pos.y, 0.3, 0.3);
    box.a = min(min(pos.y, -abs(pos.z) + 2.0), -abs(pos.x) + 2.0);;
    
    vec4 dist = box;
    
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
 
    return(dist);
}

mat3 lookatmat(vec3 eye, vec3 center, vec3 up) {
	vec3 backward=normalize(eye - center);
	vec3 right=normalize(cross(up,backward));
	vec3 actualup=normalize(cross(backward,right));

	return mat3(right, actualup, backward);
}

vec3 lookat = vec3(0.0, 1.0, 0.0);
vec3 campos(float t) {
    float wobble = t * 0.2;
    return(vec3(sin(wobble) * -1.75, 1.4 + cos(0.0 * 0.3) + 2.0, cos(wobble) * -1.75));
}

// Renderer
vec4 pixel(vec2 fragCoord) {
    float t = gl_Color.x * 3000.0 * 10.0;

    // Screen -1 -> 1 coordinates
    vec2 coords = (2.0 * fragCoord.xy - res) / res;
    
    // Set up time dependent stuff
    vec3 lightpos = campos(t);

    // Camera as eye + imaginary screen at a distance
    vec3 eye = lightpos;
    eye.y -= 2.0;
    
    float sinpow = sin(t * 5.0);
    sinpow = sinpow * sinpow;
    
    eye.x += sinpow * 0.01;
    vec3 lookdir = normalize(lookat - eye);
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
    float depth = length(pos - eye);
    vec4 fragColor = vec4(colorval.xyz, 0.0);
    float coc = 0.4 * abs(1.0 - length(eye - lookat) / depth);
    coc = max(0.01 * 5.0, min(0.35 * 5.0, coc));
    return(vec4(fragColor.rgb, coc));
}

// Image
void main() {
    float t = gl_Color.x * 3000.0 * 10.0;

    // Render what?
    if(gl_Color.w < 0.5) {
        if(gl_FragCoord.y <= 10) {
            mat3 cam = lookatmat(campos(t), lookat, vec3(0.0, 1.0, 0.0));
            vec2 v = gl_FragCoord.xy / res;
	        float ms = 0.002;

            // Update particles
		    vec4 old = texture(imageSampler, v.xy);
            vec3 dir = texture(imageSampler2, v.xy).xyz;
		    vec4 new = old + vec4(dir, 0.0) * ms;

            // Block particles
		    vec3 pos = new.xyz;
	        vec2 pd = vec2(ms, 0.0);
		    vec3 displace = vec3(
			    distfunc(pos-pd.xyy).w - distfunc(pos+pd.xyy).w,
			    distfunc(pos-pd.yxy).w - distfunc(pos+pd.yxy).w,
			    distfunc(pos-pd.yyx).w - distfunc(pos+pd.yyx).w
		    );
		    /*if(distfunc(pos).w < 0.0) {
			    new.xyz -= normalize(displace) * ms * 2.0;
                dir = reflect(dir, normalize(displace)) * 0.8;
		    }*/

            // Constrain particles
            dir -= vec3(0.0, ms * 10.0, 0.0);
            if(length(dir) > ms * 1000.0) {
                dir = normalize(dir) * ms * 1000.0;
            }
		    new.xyz = mod(new.xyz + vec3(1.0), 2.0) - vec3(1.0);
		    imageStore(imageTexture, ivec2(gl_FragCoord.xy), new);
            imageStore(imageTexture2, ivec2(gl_FragCoord.xy), vec4(dir, 0.0));

            vec3 outPos = new.xyz * cam;
            imageStore(outTexture, ivec2(gl_FragCoord.xy), vec4(outPos, 0.0));
        }

        // Render box
        f = vec4(0.0);
        for(int i = 0; i < 3; i++) {
    	    f += pixel(gl_FragCoord.xy + hash33(vec3(i)).xy);
        }
        f /= 3.0;
    }
    else {
        // Render particles
        vec2 pos = gl_PointCoord.xy - vec2(0.5);
	    float radius = length(pos);
	    if(radius > 0.5) {
		    discard;
	    }
        vec3 col = vec3(0.9, 0.6, 0.4);
        if(gl_Color.y < 0.01) {
            col = vec3(0.6, 0.9, 0.4);
        }

        float coc = 0.4 * abs(1.0 - length(0.2) / gl_Color.x);
        coc = max(0.01 * 5.0, min(0.35 * 5.0, coc));

        f = vec4(col * (0.5 - radius), coc);
    }
}