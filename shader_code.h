/* File generated with Shader Minifier 1.1.4
 * http://www.ctrl-alt-test.fr
 */
#ifndef SHADER_CODE_H_
# define SHADER_CODE_H_
# define VAR_GL_COLOR "v"
# define VAR_GL_FRAGCOLOR "f"
# define VAR_IMAGETEXT "y"
# define VAR_TEX "i"

const char *shader_frag =
 "#version 420\n"
 "layout(location=0)out vec4 f;"
 "layout(location=0)in vec4 v;"
 "layout(size4x32)coherent uniform image3D y;"
 "uniform sampler3D i;"
 "vec4 e(vec3 f)"
 "{"
   "vec2 t=vec2(1.f,0.f)/128.f;"
   "return-texture(i,f)*6.+texture(i,f+t.xyy)+texture(i,f-t.xyy)+texture(i,f+t.yxy)+texture(i,f-t.yxy)+texture(i,f+t.yyx)+texture(i,f-t.yyx);"
 "}"
 "vec4 e(float i,vec3 f)"
 "{"
   "float y=i>.02f?i<.17?.6f+i:0.f:0.f,t=pow((1.f-smoothstep(.22,.24,mod(length(f),1.5f)))*i*6.f,2.);"
   "return vec4(y*.5f+t,t,y+t,y);"
 "}"
 "void main()"
 "{"
   "float t=v.x*64.;"
   "for(int r=0;r<3;r++)"
     "{"
       "int l=int(gl_FragCoord.x+gl_FragCoord.y*1280.)*3+r;"
       "ivec3 x=ivec3(l/16384%128,l/128%128,l%128);"
       "vec3 s=vec3(x)/128.f+.5/128.f;"
       "vec4 d=e(s),m=texture(i,s);"
       "float n=m.x*m.y*m.y,c=clamp(t*.001f,0.f,1.f),g=mix(.06f,.006,c),o=mix(.0609f,.033,c);"
       "vec2 u=vec2(.082f*d.x-n+g*(1.f-m.x),.041f*d.y+n-(g+o)*m.y);"
       "m.xy=m.xy+u*.1f;"
       "imageStore(y,x,m);"
     "}"
   "vec2 m=-1.+gl_FragCoord.xy/vec2(640.,360.);"
   "vec3 r=normalize(vec3(m.x,m.y,1.));"
   "mat3 n=mat3(sin(t*.05),0.,cos(t*.05),0.,1.,0.,cos(t*.05),0.,-sin(t*.05));"
   "vec3 c=vec3(.5,.5+t*.1,.5+t*.05);"
   "r*=n;"
   "float x=0.f;"
   "vec4 l=vec4(0.);"
   "for(int u=0;u<128;u++)"
     "c+=r/128.,l+=e(texture(i,c).y,c)*x,x+=1./128.f;"
   "f=l/32.f;"
 "}";

#endif // SHADER_CODE_H_
