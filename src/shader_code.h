/* File generated with Shader Minifier 1.1.4
 * http://www.ctrl-alt-test.fr
 */
#ifndef SHADER_CODE_H_
# define SHADER_CODE_H_

const char *shader_frag =
 "#version 420\n"
 "out vec4 f;"
 "in vec4 gl_Color;"
 "uniform sampler2D postproc;"
 "vec3 s(sampler2D s,vec2 v)"
 "{"
   "vec2 z=vec2(1.)/vec2(1280.,720.);"
   "vec3 t=vec3(0.);"
   "float c=0.,p=texture(s,v).w;"
   "for(float w=0.;w<50.24;w+=.098125)"
     "{"
       "float x=cos(3.14/6.)/cos(mod(w,6.28/6.)-3.14/6.);"
       "vec2 r=vec2(sin(w),cos(w))*x*w*z*p;"
       "vec4 g=texture(s,v+r);"
       "r=vec2(sin(w),cos(w))*x*w*z*g.w;"
       "g=texture(s,v+r);"
       "t+=g.xyz*g.w*w;"
       "c+=g.w*w;"
     "}"
   "t=t/c;"
   "return t;"
 "}"
 "void main()"
 "{"
   "float w=gl_Color.x*3000.*50.;"
   "f=vec4(1.,0.,.2,0.);"
   "vec2 v=gl_FragCoord.xy/vec2(1280.,720.);"
   "f=vec4(s(postproc,v),0.);"
   "f=f/(f+vec4(1.));"
   "f.xyz=vec3(pow(f.x,1./2.2),pow(f.y,1./2.2),pow(f.z,1./2.2));"
   "float t=sin(w*11.);"
   "t=t*t;"
   "f.xyz*=.92+.08*(t+1.)/2.;"
 "}";

#endif // SHADER_CODE_H_
