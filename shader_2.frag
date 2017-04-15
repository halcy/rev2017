#version 420

out vec4 f;
in vec4 gl_Color;
uniform vec2 res;
uniform float envelope;
uniform float improve;

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

float randx(vec2 co){
    return(fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453));
}

vec4 tunnel() {
    float time = gl_Color.x * 3000.0 * 10.0;
   
    vec2 p=(2.0*gl_FragCoord.xy-res)/min(res.x,res.y)*0.5;

	float z0=time*28.0/1.0;
	float i=floor(z0);
	float offs=fract(z0);
	float shadow=1.0;
     
	for(float z=1.0;z<150.0;z+=1.0) {
		float z2=z-offs;
		float randz=z+i;
		float dadt=(rand(vec2(randz,1.0))*2.0-1.0)*0.5;

		float a=rand(vec2(randz,1.0))*2.0*3.141592+dadt*time;
		float pullback=rand(vec2(randz,3.0))*4.0+1.0;
		float r=rand(vec2(randz,4.0))*0.5+1.4;
		float g=rand(vec2(randz,5.0))*0.5+0.7;
		float b=rand(vec2(randz,6.0))*0.5+0.7;

		vec2 origin=vec2(sin(randz*0.005)+sin(randz*0.002),cos(randz*0.005)+cos(randz*0.002))*z2*0.002;
		
		vec2 dir=vec2(cos(a),sin(a));
		float dist=dot(dir,p-origin)*z2;
		float xdist=dot(vec2(-dir.y,dir.x),p-origin)*z2;
		float wobble=dist-pullback+sin(xdist*20.0)*0.05;
		if(wobble>0.0)
		{
			float dotsize=rand(vec2(randz,7.0))*0.5+0.1;
			float patternsize=rand(vec2(randz,8.0))*2.0+2.0;
			float pattern=step(dotsize,length(fract(vec2(dist,xdist)*patternsize)-0.5))*0.1+0.9;

			float bright;
			if(wobble<0.2) bright=1.2;
			else if(wobble<0.22) bright=0.9;
			else bright=1.0*pattern;

			gl_FragColor=vec4(30.0*vec3(r,g,b)*shadow/(z2+30.0)*bright,1.0);
		}
		else
		{
			shadow*=1.0-exp((dist-pullback)*2.0)*0.2;
		}
	}
	return(vec4(vec3(0.0),1.0));
}


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

float heightfield(float x,float a) {
    float time = gl_Color.x * 3000.0 * 10.0;
	x*=0.4;
	return sin(4.0*x+3.0*sin(2.0*a+time+2.0*sin(3.0*x+time+sin(time*1.4))))*0.5+0.6;
}

vec4 twister(void) {
    float time = gl_Color.x * 3000.0 * 10.0;
	vec2 p=(2.0*gl_FragCoord.xy-res)/min(res.x,res.y);
    
    float coc = abs(p.x) * 0.2;
    if(coc < 0.001) {
        coc = 0.001;
    }

	vec3 origin=vec3(time*1.0,sin(time)*0.4+0.4,-1.4);
	vec3 direction=normalize(vec3(p.x,p.y,1.2-length(p)*0.3));

	float va=0.3;
	direction.xz=mat2(cos(va),sin(va),-sin(va),cos(va))*direction.xz;

	vec3 camz=vec3(0.0,0.0,1.0);
	camz.xz=mat2(cos(va),sin(va),-sin(va),cos(va))*camz.xz;

	float a0=atan(origin.y,origin.z);
	float s=sign(origin.z*direction.y-origin.y*direction.z);

	for(float a=0.0;a<3.141592;a+=0.05)
	{
		float a2=s*a+a0;
		vec3 slicenormal=vec3(0.0,cos(a2),-sin(a2));
		float t=-dot(origin,slicenormal)/dot(direction,slicenormal);
		vec3 intersection=origin+direction*t;
		float ih=length(intersection.yz);
		float ix=intersection.x;
		float h=heightfield(ix,a2);
		if(h>ih)
		{
			float epsilon=0.01;
			float hx=heightfield(ix+epsilon,a2);
			float ha=heightfield(ix,a2+epsilon);
			vec3 p=vec3(ix,h*sin(a2),h*cos(a2));
			vec3 px=vec3(ix+epsilon,hx*sin(a2),hx*cos(a2));
			vec3 pa=vec3(ix,ha*sin(a2+epsilon),ha*cos(a2+epsilon));
			vec3 dpdx=(px-p)/epsilon;
			vec3 dpda=(pa-p)/epsilon;
			vec3 n=normalize(cross(dpdx,dpda));
			//float c=h*(1.0+dot(n,vec3(1.0,1.0,1.0))*0.3)+1.0*pow(1.0-dot(n,normalize(origin-p)),3.0);
			float c=ih*(1.0+dot(n,vec3(1.0,1.0,1.0))*0.3)+2.0*pow(1.0-abs(dot(slicenormal,normalize(origin-p))),3.0);

			return vec4(pow(c, 2.0)*vec3(0.2, 0.5, 0.6),coc);
		}
	}

    float brightfrac = fract(p.x + p.y / (2.0 + p.x)) > 0.5 ? 1.0 : 0.0;

	return(vec4(0.7 + (0.4 * (1.0 - brightfrac)), 0.2 * brightfrac, 0.1 + 0.1 * brightfrac, coc * 0.4));
}

float sdEllipsoid( in vec3 p, in vec3 r ) {
    return (length( p/r ) - 1.0) * min(min(r.x,r.y),r.z);
}

vec4 compose_ellipse(in vec4 dist, in vec3 pos, in vec3 obj_pos, in vec3 offset, in vec3 scale, in vec3 color, in float size) {
	float object_dist = sdEllipsoid( pos - (obj_pos + size*offset), size*scale );
	return distcompose(dist, vec4(color, object_dist), 0.02);
}

vec4 render_ducky(in float t, in vec4 dist, in vec3 pos ) {
    vec2 sc = vec2(sin(t), cos(t));
    mat3 boxRot = mat3(sc.t, 0.0, sc.s,   0.0,  1.0, 0.0, -sc.s, 0.0, sc.t) *
			        mat3(sc.t, sc.s, 0.0, -sc.s, sc.t, 0.0,   0.0, 0.0,  1.0);
    vec3 boxPos = vec3(-0.35, 0.6, -0.35);
    vec3 eye_col = vec3(20.8, 0.8, 0.1);
	vec3 body_col = vec3(0.8, 0.8, 0.1);
	float duck_size = .5;
	dist = compose_ellipse(dist, pos, boxPos, vec3(0.0),            vec3(0.8,0.3,0.6), body_col, duck_size); // body
	dist = compose_ellipse(dist, pos, boxPos, vec3(-0.7,0.5,0.0),   vec3(0.3),         body_col, duck_size); // head
	dist = compose_ellipse(dist, pos, boxPos, vec3(-1.1,0.5,0.0),   vec3(0.1,0.02,0.1), body_col, duck_size); // mouth
	dist = compose_ellipse(dist, pos, boxPos, vec3(-1.1,0.6,0.05),  vec3(0.02),        eye_col,  duck_size); // eye
	dist = compose_ellipse(dist, pos, boxPos, vec3(-1.1,0.6,-0.05), vec3(0.02),         eye_col, duck_size); // eye
	return dist;
}

// World
vec4 distfunc(vec3 pos) {
    float t = gl_Color.x * 3000.0 * 10.0;
    float effselect = gl_Color.y * 65536.0;

  	vec4 box = vec4(0.0);
    box.xyz = vec3(0.3 * pos.y, 0.3, 0.3) * (fract(pos).x > 0.5 ? 1.0 : 0.2);
    box.a = min(min(pos.y, -abs(pos.z) + 2.0), -abs(pos.x) + 2.0);;
    
    vec4 dist = box;
    
    if(effselect < 0.1) {
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
            float stripes = (fract(pos * 5.0).z > 0.5 ? 1.0 : 0.2);
            col = col * stripes;
            vec4 ball = vec4(col, (length(ballPos - pos) - radius));
            if(improve < 2.0) {
                dist = distcompose(dist, ball, 0.3 + (stripes * improve) * 0.3);
            }
            else {
                dist = distcompose(dist, ball, 0.3 + (stripes * improve) * 0.7);
            }
        }
    }

    if(abs(effselect - 1.0) < 0.1) {
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
    vec3 colorval = light * distfunc(pos).rgb;
   
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

    float effselect = gl_Color.y * 65536.0;
    if(abs(effselect - 2.0) < 0.1) {
        f = twister() * vec4(0.3, 0.3, 0.3, 1.0);
    }
}