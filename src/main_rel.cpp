//--------------------------------------------------------------------------//
// iq / rgba  .  tiny codes  .  2008                                        //
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include "glext.h"
#include <cstdlib>

//#define tiny
#ifdef tiny
#define XRES 228
#define YRES 140
#else
#define XRES 1280
#define YRES 720
#endif
//#define FULLSCREEN
#define XSCALE ((float)YRES/(float)XRES)
#define SHADER_CHECK

#include "shader_code.h"
#include "shader_code_2.h"
#include "../4klang.h"

#include <MMSystem.h>
#include <MMReg.h>

#define USE_SOUND_THREAD
#define SPHERERAD 0.002

// MAX_SAMPLES gives you the number of samples for the whole song. we always produce stereo samples, so times 2 for the buffer
SAMPLE_TYPE	lpSoundBuffer[MAX_SAMPLES*2];  
HWAVEOUT	hWaveOut;

/////////////////////////////////////////////////////////////////////////////////
// initialized data
/////////////////////////////////////////////////////////////////////////////////

#pragma data_seg(".wavefmt")
WAVEFORMATEX WaveFMT =
{
#ifdef FLOAT_32BIT	
	WAVE_FORMAT_IEEE_FLOAT,
#else
	WAVE_FORMAT_PCM,
#endif		
    2, // channels
    SAMPLE_RATE, // samples per sec
    SAMPLE_RATE*sizeof(SAMPLE_TYPE)*2, // bytes per sec
    sizeof(SAMPLE_TYPE)*2, // block alignment;
    sizeof(SAMPLE_TYPE)*8, // bits per sample
    0 // extension not needed
};

#pragma data_seg(".wavehdr")
WAVEHDR WaveHDR = 
{
	(LPSTR)lpSoundBuffer, 
	MAX_SAMPLES*sizeof(SAMPLE_TYPE)*2,			// MAX_SAMPLES*sizeof(float)*2(stereo)
	0, 
	0, 
	0, 
	0, 
	0, 
	0
};

MMTIME MMTime = 
{ 
	TIME_SAMPLES,
	0
};

const static PIXELFORMATDESCRIPTOR pfd = {0,0,PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static DEVMODE screenSettings = { 
    #if _MSC_VER < 1400
    {0},0,0,148,0,0x001c0000,{0},0,0,0,0,0,0,0,0,0,{0},0,32,XRES,YRES,0,0,      // Visual C++ 6.0
    #else
    {0},0,0,156,0,0x001c0000,{0},0,0,0,0,0,{0},0,32,XRES,YRES,{0}, 0,           // Visual Studio 2005
    #endif
    #if(WINVER >= 0x0400)
    0,0,0,0,0,0,
    #if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
    0,0
    #endif
    #endif
    };

//--------------------------------------------------------------------------//

float textureData[XRES * YRES * 4];
float textureDataInitial[XRES * YRES * 4];
float textureDataInitialDuck[XRES * YRES * 4];
float textureDataInitialZero[XRES * YRES * 4];

void bind_res(int p) {
	((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p);
	GLint res_loc = ((PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation"))(p, "res");
	((PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f"))(res_loc, XRES, YRES);
}

extern "C" float get_Envelope(int instrument) {
    return(&_4klang_envelope_buffer)[((MMTime.u.sample >> 8) << 5) + instrument];
}

float env_decay = 0.0;
void send_envelope(int p) {
    float env = get_Envelope(7) + get_Envelope(6);
    env_decay = env_decay * 0.8 + env * 0.2;

    ((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p);
    GLint env_loc = ((PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation"))(p, "envelope");
    ((PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f"))(env_loc, env);
}

// Image texture binding
typedef void (APIENTRYP PFNGLBINDIMAGETEXTUREEXTPROC) (GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format);

const int create_frag_shader(char *name, const char *shader_frag, HWND hWnd) {
	// create shader
	int program_id = ((PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram"))();
	const int shader_id = ((PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader"))(GL_FRAGMENT_SHADER);
	((PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource"))(shader_id, 1, &shader_frag, 0);
	((PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader"))(shader_id);
#ifdef SHADER_CHECK
	GLint isCompiled = 0;
	((PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv"))(shader_id, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)	{
		char error_log[1024];
		((PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog"))(shader_id, 1024, NULL, error_log);
		char full_error_log[1024];
		strcpy(full_error_log, name);
		strcat(full_error_log, ": ");
		strcat(full_error_log, error_log);
		MessageBox(hWnd, full_error_log, "GLSL Error", 0);
		((PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader"))(shader_id); // Don't leak the shader.
		ExitProcess(1);
	}
#endif
	((PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader"))(program_id, shader_id);
	((PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram"))(program_id);
	return program_id;
}

void entrypoint( void )
{ 
    int PATTERN_LEN = (SAMPLES_PER_TICK * PATTERN_SIZE) / 2.0;
	int outer_width = XRES;
	int outer_height = YRES;

    // Sync stuff
    int effect_advance_at[] = {
        PATTERN_LEN * 6, // Noise
        PATTERN_LEN * 18, // Twister
        PATTERN_LEN * 36, // Balls
        PATTERN_LEN * 54, // Cubes
        PATTERN_LEN * 66, // Duck
        PATTERN_LEN * 78, // Tunnel
        PATTERN_LEN * 92  // Finis (noise)
    };

    int effect_type[] = {
        3,
        2, 
        0, 
        1,
        5,
        4,
        3
    };

    int wants_particles[] = {
        0,
        0, 
        0, 
        1,
        1,
        0,
        0
    };

#ifdef FULLSCREEN
    // full screen
    if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return; ShowCursor( 0 );

    // create windows
	HWND hWND = CreateWindow("edit", 0, WS_POPUP|WS_VISIBLE, 0, -100,XRES,YRES,0,0,0,0);
    HDC hDC = GetDC( hWND );
#else
	RECT wrect = { 0, 0, XRES, YRES };
	AdjustWindowRectEx(&wrect, WS_CAPTION | WS_VISIBLE, FALSE, 0);
	outer_width = wrect.right - wrect.left;
	outer_height = wrect.bottom - wrect.top;
	HWND hWND = CreateWindow("edit", 0, WS_CAPTION | WS_VISIBLE, 0, 0,outer_width,outer_height,0,0,0,0);
    HDC hDC = GetDC( hWND );
#endif
    // init opengl
    SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
    wglMakeCurrent(hDC, wglCreateContext(hDC));

    // create shader
	const int p = create_frag_shader("post process", shader_frag, hWND);
	const int p2 = create_frag_shader("world", shader_2_frag, hWND);

	bind_res(p);
	bind_res(p2);

    // Init particle data
    for (int i = 0; i < XRES * YRES; i++) {
        unsigned int jx = ((i * 42144323) % 34423233) % (XRES * YRES);
        unsigned int jy = ((i * 12233123) % 85653223) % (XRES * YRES);
        unsigned int js = ((i * 53764312) % 23412352) % (XRES * YRES);
        textureDataInitial[i * 4] = (((float)(jx % 1280) / 1280.0)) * 0.4;
        textureDataInitial[i * 4 + 1] = (((float)jy / 720.0) / 360.0) + 2.0;
        textureDataInitial[i * 4 + 2] = ((float)((js % (80 * 400)) / (80.0 * 400.0))) * 0.4;

        textureDataInitialDuck[i * 4] = (((float)(jx % 1280) / 1280.0)) * 0.2 - 1.2;
        textureDataInitialDuck[i * 4 + 1] = (((float)jy / 720.0) / 360.0) + 1.0;
        textureDataInitialDuck[i * 4 + 2] = ((float)((js % (80 * 400)) / (80.0 * 400.0))) * 0.2;
    }

	// Create textures
	GLuint imageTextures[5];
	glGenTextures(5, imageTextures);

	((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageTextures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, 0);

    ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, imageTextures[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, textureDataInitial);

    ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, imageTextures[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, textureDataInitialZero);

    ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, imageTextures[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, textureDataInitialZero);

    // put in textures
    ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
    ((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT"))(0, imageTextures[1], 0, 1, 0, GL_READ_WRITE, GL_RGBA32F);
    ((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT"))(1, imageTextures[2], 0, 1, 0, GL_READ_WRITE, GL_RGBA32F);
    ((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT"))(2, imageTextures[3], 0, 1, 0, GL_READ_WRITE, GL_RGBA32F);

	// Set up window
	MoveWindow(hWND, 0, 0, outer_width, outer_height, 0);

	// Sound
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_4klang_render, lpSoundBuffer, 0, 0);
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFMT, NULL, 0, CALLBACK_NULL );
	waveOutPrepareHeader(hWaveOut, &WaveHDR, sizeof(WaveHDR));
	waveOutWrite(hWaveOut, &WaveHDR, sizeof(WaveHDR));	

	// unfortunately, FBOs
	GLuint fbo, depth_rb;
	glBindTexture(GL_TEXTURE_2D, 0);
	((PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers"))(1, &fbo);
	((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, fbo);
	((PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D"))(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, imageTextures[0], 0);

    glDisable(GL_DEPTH_TEST);

    // Pointing
    glEnable(GL_POINT_SPRITE);
    glBlendFunc(GL_ONE, GL_ONE);

    glMatrixMode( GL_PROJECTION );

    // run
	int samplast = 0;
    int effselector = 0;
    int sceneselector = 0;
    do
    {
        // get sample position for timing
        waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTIME));

        // Advance?
        if (MMTime.u.sample > effect_advance_at[sceneselector]) {
            sceneselector++;

            if(wants_particles[sceneselector]) {
                if (effect_type[sceneselector] == 5) {
                    ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE1);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, textureDataInitialDuck);
                }
                else {
                    ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE1);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, textureDataInitial);
                }

                ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE2);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, textureDataInitialZero);
            }
        }

        float to_trans = ((float)(effect_advance_at[sceneselector] - MMTime.u.sample)) / ((float)PATTERN_LEN);
        to_trans = fabs(to_trans);
        to_trans = 2.0 - min(to_trans, 2.0);

        float to_trans2 = ((float)(MMTime.u.sample - effect_advance_at[sceneselector - 1])) / ((float)PATTERN_LEN);
        to_trans2 = fabs(to_trans2);
        to_trans2 = 1.0 - min(to_trans2, 1.0);

        to_trans =  to_trans + to_trans2;

        effselector = effect_type[sceneselector];

        int samplediff = ((float)(MMTime.u.sample - samplast));
        samplast = MMTime.u.sample;

        // bind FBO to render world into
        ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
		((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, fbo);

        // Draw world, use glColor to send in timing
		((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p2);
        send_envelope(p2);
        
        float improve_effect = 0.0;
        if (MMTime.u.sample > effect_advance_at[sceneselector] - PATTERN_LEN * 12) {
            improve_effect = 1.0;
        }

        if (MMTime.u.sample > effect_advance_at[sceneselector] - PATTERN_LEN * 6) {
            improve_effect = 2.0;
        }
        GLint res_loc = ((PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation"))(p2, "improve");
        ((PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f"))(res_loc, improve_effect);

        glColor4ui(MMTime.u.sample, effselector * 65536, samplediff, 0);
        glRects(-1, -1, 1, 1);

        // Make a bunch of points happen
        if(wants_particles[sceneselector]) {
            glPushMatrix();
            glFrustum(-1, 1, -1 * ((float)YRES / (float)XRES), 1 * ((float)YRES / (float)XRES), 1.0, 100.0);

            ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE3);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, textureData);
            glEnable(GL_BLEND);
            glPointSize(10.0);
            glBegin(GL_POINTS);
            for (int i = 0; i < 1280 * 10; i++) {
                glColor4f(textureData[i * 4 + 3] / 1000.0, effselector * 65536, 0.0, 1.0);
                glVertex3f(textureData[i * 4], textureData[i * 4 + 1], textureData[i * 4 + 2]);
            }
            glEnd();
            glDisable(GL_BLEND);
            glPopMatrix();
        }

        // bind screen FB
		((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, 0);

        // post-process FBO directly to screen FB, send in timing as well
        ((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p);
		((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, imageTextures[0]);
		//glColor4ui(MMTime.u.sample, 0, 0, 0);
        glColor4f(MMTime.u.sample * 0.00000000001, effect_type[sceneselector] * 0.01, to_trans, 0.0);
		glRects(-1, -1, 1, 1);

        SwapBuffers(hDC);

		PeekMessageA(0, 0, 0, 0, PM_REMOVE); 
	} while (MMTime.u.sample < MAX_PATTERNS * SAMPLES_PER_TICK * PATTERN_SIZE && !GetAsyncKeyState(VK_ESCAPE));

    ExitProcess( 0 );
}
