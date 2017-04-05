//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//
/*
const static char *vertexShader=\
    "varying vec4 v;"
    "void main()"
    "{"
        "v=vec4(gl_Vertex.xy,64.0*gl_Color.xy);"
        "gl_Position=gl_Vertex;"
    "}";
*/
/*
const static char *fragmentShader=\
    "#version 420\n"
    "#extension GL_ARB_gpu_shader5 : enable\n"
    "#extension GL_EXT_shader_image_load_store : enable\n"
    "layout(location = 0) out vec4 gl_FragColor;\n"
    "layout(location = 0) in vec4 gl_Color;\n"
    "layout(size4x32) coherent uniform image3D imagetext;\n"
    "vec4 colf(vec2 v){"
       	"vec3 r = normalize(vec3(1.0-0.003*gl_FragCoord.x,1.6-0.003*gl_FragCoord.y,1.0));"
	      "vec3 pos = vec3(0,-0.5,-15);"
	      "float c = 0.0;"
	      "float dist = 200;"
	      "while( c < 255 && dist > 0.01) {"
		    "dist = length(pos-vec3(0,0,sin(gl_Color.x*30.0)*10.0))-2.0;"
		    "dist = min(dist,sin(pos.x) + cos(pos.z) - pos.y + 2.0);"
		    "pos += r * dist;"
		    "c++;"
	      "}"
        "c=(255.0-c)/255.0;"
	      "return vec4(c,c,c,pos.z+15.0);"
    "}"
    "void main()"
    "{"
        // Coords
		    "vec2 v=-1.+gl_FragCoord.xy/vec2(400.,300.);"
		    
        // Generate
        "vec4 col=colf(v);"
        
        // Save
        "imageStore(imagetext,ivec3(gl_FragCoord.xy,0),col);"

        // Precalc
        "float depth=min(1.0,abs(col.a/4.0+8.0)/20.0);"

        // A pass
        "int width = int(abs(depth*20-11));"
        "for(int x=-width;x<=width;x++){"
        "for(int y=-width;y<=width;y++){"
        "col += imageLoad(imagetext,ivec3(gl_FragCoord.xy+ivec2(x,y)*4,0))*cos(length(vec2(x,y)/(width==0?1:width)));"
        "}}"
        "col *= (width != 0 ? pow(1.0/width,1.6) : 2.4) * 0.4;"
        "vec4 cola = col;"

        // Reload
        "col=imageLoad(imagetext,ivec3(gl_FragCoord.xy,0));"

        // B pass
        "width = int(abs(depth*20-11))+1;"
        "for(int x=-width;x<=width;x++){"
        "for(int y=-width;y<=width;y++){"
        "col += imageLoad(imagetext,ivec3(gl_FragCoord.xy+ivec2(x,y)*4,0))*cos(length(vec2(x,y)/(width==0?1:width)));"
        "}}"
        "col *= (width != 0 ? pow(1.0/width,1.6) : 2.4) * 0.4;"

        // Interpolate
        "float inter = abs(depth*20-11) - int(abs(depth*20-11));"
        "col = col * inter + cola * (1.0 - inter);"
        
        // Display
        "gl_FragColor=max(vec4(0),col/3.0);"
    "}";
*/

/*const static char *fragmentShader=\
    "#version 420\n"
    "#extension GL_ARB_gpu_shader5 : enable\n"
    "#extension GL_EXT_shader_image_load_store : enable\n"
    "layout(location = 0) out vec4 gl_FragColor;\n"
    "layout(location = 0) in vec4 gl_Color;\n"
    "layout(size4x32) coherent uniform image3D imagetext;\n"
    "void main()"
    "{"
		    "vec2 v=-1.+gl_FragCoord.xy/vec2(400.,300.);"
		    // tunnel
        "float r=pow(pow(abs(v.x),18.)+pow(abs(v.y),10.),1.8/28.);"
        "vec2 t=vec2(gl_Color.x*64.+1./r,atan(v.x,v.y));"
        // texture
        "t=fract(2*t)-.5;"
        "vec4 col=((1-pow(dot(t.xy,t.xy),.3))*vec4(2,1.8,1.6,0)+vec4(.3,.2,.1,0)+.12*vec4(v,0,0))/(9.*r);"
        // final color
        "int width = 2;"
        //"int z = (int(gl_Color.x*401.0)%10)+(int(gl_FragCoord.x*501.0)%10)+(int(gl_FragCoord.y*501.0)%10);"
        "int zr = int(t*415.0)%30;"
        "int zw = int(t*231.0)%30;"
        "for(int x=-width;x<=width;x++){"
        "for(int y=-width;y<=width;y++){"
          "vec4 temp = imageLoad(imagetext,ivec3(gl_FragCoord.xy+ivec2(x,y)*2,zr));"
          "temp = temp * temp.a;"
          "imageStore(imagetext,ivec3(gl_FragCoord.xy+ivec2(x,y)*2,zw),(temp+col*2.0)/3.0);"
        "}"
        "}"
        "gl_FragColor=vec4(0);"
        "for(int zl=0;zl<30;zl++){"
        "gl_FragColor+=imageLoad(imagetext,ivec3(gl_FragCoord.xy,zl));"
        "}"
        "gl_FragColor/=30.0;"
    "}";*/

/*const static char *fragmentShader=\
    "#version 420\n"
    "#extension GL_ARB_gpu_shader5 : enable\n"
    "#extension GL_EXT_shader_image_load_store : enable\n"
    "layout(location = 0) out vec4 gl_FragColor;\n"
    "layout(location = 0) in vec4 gl_Color;\n"
    "layout(size4x32) coherent uniform image3D imagetext;\n"
    "float quasi(vec2 tc,float k,float stripes,float phase){"
	    "float theta=atan(-tc.y,tc.x);"
	    "float r=log(length(tc));"
	    "float C=0.0;"
	    "for(float t=0.0; t<k; t++) {"
		    "C+=cos((theta*cos(t*(3.14f/k))-r*sin(t*(3.14f/k)))*stripes+phase);"
		    "C+=cos((tc.x*cos(t*(3.14f/k))+tc.y*sin(t*(3.14f/k)))*2.0*3.14f*stripes+phase);"
	    "}"
	    "float c=((C+k)/(k*2.0));"
      //"return c;"
      "return float(int(c+0.6));"
    "}"
    "void main()"
    "{"
		    "vec2 v=-1.+gl_FragCoord.xy/vec2(400.,300.);"
        "float t=gl_Color.x*64.;t/=64.0;"
        "vec4 col=vec4(quasi(v*5.,15.0f,4.0f,t.x));"
        // final color
        "int width = 2;"
        //"int zr = (int(gl_Color.x*401.0)%10)+(int(gl_FragCoord.x*501.0)%10)+(int(gl_FragCoord.y*501.0)%10);"
        "int zr = int(t*10000.0+1230.0)%64;"
        "int zw = int(t*20001.0)%64;"
        "for(int x=-width;x<=width;x++){"
          "for(int y=-width;y<=width;y++){" 
            "vec4 temp = imageLoad(imagetext,ivec3(gl_FragCoord.xy+ivec2(x,y)*8,(zr+x*312+y*123)%64));"
            "imageStore(imagetext,ivec3(gl_FragCoord.xy+ivec2(x,y)*8,(zr+x*312+y*123)%64),(temp+col*2.0)/3.0);"
          "}"
        "}"
        "gl_FragColor=vec4(0);"
        "for(int zl=0;zl<1;zl++){"
        "gl_FragColor+=imageLoad(imagetext,ivec3(gl_FragCoord.xy,zl));"
        "}"
        "gl_FragColor/=1.0;"
    "}";*/


const static char *fragmentShader=\
    "#version 420\n"
    "layout(location = 0) out vec4 gl_FragColor;\n"
    "layout(location = 0) in vec4 gl_Color;\n"
    "layout(size4x32) coherent uniform image3D imagetext;\n"
    "uniform sampler3D tex;\n"
	"vec4 lapras(vec3 p) {"
		"vec2 o = vec2(1.0f,0.0f)/128.0f;"
		"return -texture(tex,p) * 6.0 +"
			 "texture(tex,p+o.xyy) + texture(tex,p-o.xyy) +"
			 "texture(tex,p+o.yxy) + texture(tex,p-o.yxy) +"
			 "texture(tex,p+o.yyx) + texture(tex,p-o.yyx);"
	"}"
	"vec4 transfer(float inf, vec3 p) {"
		"float val = inf > 0.02f ? inf < 0.17 ? 0.6f + inf : 0.0f : 0.0f;"
		"float val2 = pow((1.0f - smoothstep(0.22, 0.24, mod(length(p), 1.5f) )) * inf * 6.0f,2.0);"
		"return vec4(val*0.5f+val2, 0.0f+val2, val+val2, val);"
	"}"
    "void main()"
    "{"
		"float t=gl_Color.x*64.;"
		"for(int i = 0; i < 3; i++) {"
			"int idx = int(gl_FragCoord.x+gl_FragCoord.y*1280.0)*3+i;"
			//"if(idx < 128*128*128) {"
				"ivec3 pi = ivec3((idx/(128*128))%128, (idx/128)%128, idx%128);"
				"vec3 pd = vec3(pi)/128.0f+0.5/128.0f;"
				"vec4 lap = lapras(pd);"
				"vec4 cen = texture(tex, pd);"
				"float help = cen.x*cen.y*cen.y;"
				"float tinter = clamp(t*0.001f, 0.0f, 1.0f);"
				"float f = mix(0.0600f, 0.0060, tinter);"
				"float k = mix(0.0609f, 0.0330, tinter);"
				//"vec2 rd = vec2(0.082f * lap.x - help + 0.0620f * (1.0f - cen.x), 0.041f * lap.y + help - (0.0620f + 0.0609f) * cen.y);"
				//"vec2 rd = vec2(0.082f * lap.x - help + 0.0060f * (1.0f - cen.x), 0.041f * lap.y + help - (0.0060f + 0.0330f) * cen.y);"
				"vec2 rd = vec2(0.082f * lap.x - help + f * (1.0f - cen.x), 0.041f * lap.y + help - (f + k) * cen.y);"
				//"cen.zw = clamp(cen.xy + rd * 0.8f, vec2(0.0f), vec2(1.0f));"
				"cen.xy = cen.xy + rd * 0.1f;"
				"imageStore(imagetext, pi, cen);"
			//"}"
		"}"

		//"memoryBarrier();"

		/*"for(int i = 0; i < 3; i++) {"
			"int idx = int(gl_FragCoord.x+gl_FragCoord.y*1280.0)*3+i;"
			"if(idx < 128*128*128) {"
				"ivec3 pi = ivec3(idx/(128*128), (idx/128)%128, idx%128);"
				"imageStore(imagetext, pi, imageLoad(imagetext, pi).zwzw);"
			"}"
		"}"*/

		//"memoryBarrier();"

		"vec2 v=-1.+gl_FragCoord.xy/vec2(640.0,360.);"
        "vec3 r=normalize(vec3(v.x,v.y,1.0));"
        "mat3 rot = mat3(sin(t*0.05), 0.0, cos(t*0.05), 0.0, 1.0, 0.0, cos(t*0.05), 0.0, -sin(t*0.05));"
		"vec3 p=vec3(0.5,0.5+t*0.1,0.5+t*0.05);"
        "r*=rot;"
        //"p=p-r-vec3(0.0,0.0,0.0f*sin(t)*0.2);"
        "float weight = 0.0f;"
		"vec4 c = vec4(0.0);"
		"for(int i=0; i < 128; i++) {"
            "p+=r/128.0;"
			"c+=(transfer(texture(tex,p).y, p) * weight);"
            "weight += (1.0/128.0f);"
        "}"
		"gl_FragColor=c / 32.0f;"
    "}";