/* File generated with Shader Minifier 1.1.4
 * http://www.ctrl-alt-test.fr
 */
#ifndef SHADER_CODE_2_H_
# define SHADER_CODE_2_H_

const char *shader_2_frag =
 "#version 420\n"
 "out vec4 f;"
 "in vec4 gl_Color;"
 "void main()"
 "{"
   "vec2 v=gl_PointCoord.xy-vec2(.5);"
   "float g=length(v);"
   "if(g>.5)"
     "{"
       "discard;"
     "}"
   "f=vec4(v.x,v.y,1.-length(v),1.);"
 "}";

#endif // SHADER_CODE_2_H_
