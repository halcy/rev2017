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

#define XRES 1280
#define YRES 720
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

void bind_res(int p) {
	((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p);
	GLint res_loc = ((PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation"))(p, "res");
	((PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f"))(res_loc, XRES, YRES);
}

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
	//#define SWITCH_AFTER (338688 * 2)
	//#define SWITCH_AFTER_HALF 338688
	int SWITCH_AFTER = 338688 * 2;
	int SWITCH_AFTER_HALF = SWITCH_AFTER / 2;
#ifdef FULLSCREEN
    // full screen
    if( ChangeDisplaySettings(&screenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL) return; ShowCursor( 0 );

    // create windows
	HWND hWND = CreateWindow("edit", 0, WS_POPUP|WS_VISIBLE, 0, -100,XRES,YRES,0,0,0,0);
    HDC hDC = GetDC( hWND );
#else
	HWND hWND = CreateWindow("edit", 0,  WS_CAPTION|WS_VISIBLE, 0, 0,XRES,YRES,0,0,0,0);
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

	// Create textures
	GLuint imageTextures[1];
	glGenTextures(1, imageTextures);

	((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageTextures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, 0);

	// Set up window
	MoveWindow(hWND, 0, 0, XRES, YRES, 0);

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

    // run
	unsigned int samplast = 0;
	unsigned int samplerun = 0;
    do
    {
        // get sample position for timing
        waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTIME));

        // bind FBO to render world into
        ((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
		((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, fbo);

        // Draw world, use glColor to send in timing
		((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p2);
        glColor4ui(MMTime.u.sample, 0, 0, 0);
        glRects(-1, -1, 1, 1);

        // bind screen FB
		((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, 0);

        // post-process FBO directly to screen FB, send in timing as well
        ((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p);
		((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, imageTextures[0]);
		glColor4ui(MMTime.u.sample, 0, 0, 0);
		glRects(-1, -1, 1, 1);

        SwapBuffers(hDC);

		PeekMessageA(0, 0, 0, 0, PM_REMOVE); 
	} while (MMTime.u.sample < SWITCH_AFTER_HALF * 9 && !GetAsyncKeyState(VK_ESCAPE));

    ExitProcess( 0 );
}
