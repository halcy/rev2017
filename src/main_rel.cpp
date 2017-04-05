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
// #define FULLSCREEN
#define XSCALE ((float)YRES/(float)XRES)

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

// Image texture binding
typedef void (APIENTRYP PFNGLBINDIMAGETEXTUREEXTPROC) (GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format);

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

// 3D image texture creation / binding
float textureData[XRES * YRES * 4];
float textureDataInitial[XRES * YRES * 4];
float screenData[XRES * YRES * 4];
float textData[XRES * YRES * 4];

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
	HWND hWND = CreateWindow("edit", 0, WS_POPUP|WS_VISIBLE, 0, -100,1280,720,0,0,0,0);
    HDC hDC = GetDC( hWND );
#else
	HWND hWND = CreateWindow("edit", 0,  WS_CAPTION|WS_VISIBLE, 0, 0,1280,720,0,0,0,0);
    HDC hDC = GetDC( hWND );
#endif
    // init opengl
    SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd);
    wglMakeCurrent(hDC, wglCreateContext(hDC));

    // create shader
    const int p = ((PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram"))();
    const int s = ((PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader"))(GL_FRAGMENT_SHADER);
    ((PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource"))(s, 1, &shader_frag, 0);
    ((PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader"))(s);
    ((PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader"))(p,s);
    ((PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram"))(p);

	const int p2 = ((PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram"))();
	const int s2 = ((PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader"))(GL_FRAGMENT_SHADER);
	((PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource"))(s2, 1, &shader_2_frag, 0);
	((PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader"))(s2);
	((PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader"))(p2, s2);
	((PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram"))(p2);

	// Create texture data
	for (int i = 0; i < XRES * YRES; i++) {
		unsigned int jx = ((i * 42144323) % 34423233) % (XRES * YRES);
		unsigned int jy = ((i * 12233123) % 85653223) % (XRES * YRES);
		unsigned int js = ((i * 53764312) % 23412352) % (XRES * YRES);
		textureDataInitial[i * 4] = ((float)(jx % 1280) / 640.0) - 1.0;
		textureDataInitial[i * 4 + 1] = (((float)jy / 720.0) / 360.0) - 1.0;
		// textureDataInitial[i * 4 + 2] = (float)((js % (80*400)) / (4.0 * 400.0) + 5.0);
		textureDataInitial[i * 4 + 2] = (float)((js % (80 * 400)) / (80.0 * 400.0));
	}

	// Allow pointing
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	// Create textures
	GLuint imageTextures[3];
	glGenTextures(3, imageTextures);

	((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, imageTextures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, textureDataInitial);

	((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, imageTextures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, 0);

	((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, imageTextures[2]);
	((PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D"))(GL_TEXTURE_3D, 0, GL_RGBA32F, 512, 512, 512, 0, GL_RGBA, GL_FLOAT, 0);

	/*((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, imageTextures[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);*/

	// Set up window
	MoveWindow(hWND, 0, 0, 1280, 720, 0);

	// Sound
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)_4klang_render, lpSoundBuffer, 0, 0);
	waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFMT, NULL, 0, CALLBACK_NULL );
	waveOutPrepareHeader(hWaveOut, &WaveHDR, sizeof(WaveHDR));
	waveOutWrite(hWaveOut, &WaveHDR, sizeof(WaveHDR));	
	
	// put in textures
	((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT"))(0, imageTextures[0], 0, 1, 0, GL_READ_WRITE, GL_RGBA32F);
	((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT"))(1, imageTextures[2], 0, 1, 0, GL_READ_WRITE, GL_RGBA32F);

	// unfortunately, FBOs
	GLuint fbo, depth_rb;
	glBindTexture(GL_TEXTURE_2D, 0);
	((PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers"))(1, &fbo);
	((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, fbo);
	((PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D"))(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, imageTextures[1], 0);
	/*((PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT"))(1, &depth_rb);
	((PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT"))(GL_RENDERBUFFER_EXT, depth_rb);
	((PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT"))(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, XRES, YRES);
	((PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT"))(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_rb);*/

    // run
	unsigned int samplast = 0;
	unsigned int samplerun = 0;
    do
    {
		((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);

		// get sample position for timing
		waveOutGetPosition(hWaveOut, &MMTime, sizeof(MMTIME));

		((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p2);
		for (int i = 0; i < 1280 * 30; i++) {
			float ssDist = (float)textureData[i * 4 + 2];
			float ssSize = (SPHERERAD / ssDist);
			ssSize = ssDist > 0.02 ? ssSize * (YRES/2.0) : 0;
			glPointSize(ssSize);
			glBegin(GL_POINTS);
			glColor4f(textureData[i * 4 + 3], 0.0, 0.0, 0.0);
			glVertex3f(textureData[i * 4], textureData[i * 4 + 1], textureData[i * 4 + 2]);
			glEnd();
		}

		glDisable(GL_BLEND);

		((PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer"))(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int i = 0; i < 1280 * 30; i++) {
			float ssDist = (float)textureData[i * 4 + 2];
			float ssSize = (SPHERERAD / ssDist);
			ssSize = ssDist > 0.02 ? ssSize * (YRES / 2.0) : 0;
			glPointSize(ssSize);
			glBegin(GL_POINTS);
			glColor4f(textureData[i * 4 + 3], 0.0, 0.0, 0.0);
			glVertex3f(textureData[i * 4], textureData[i * 4 + 1], textureData[i * 4 + 2]);
			glEnd();
		}

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(p);

		/*glReadPixels(0, 0, XRES, YRES, GL_RGBA, GL_FLOAT, screenData);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, XRES, YRES, 0, GL_RGBA, GL_FLOAT, screenData);*/

		((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, imageTextures[1]);

		((PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, textureData);
		glColor4ui(MMTime.u.sample, 0, 0, 0);
		glRects(-1, -1, 1, 1);

        SwapBuffers(hDC);

		PeekMessageA(0, 0, 0, 0, PM_REMOVE); 
	} while (MMTime.u.sample < SWITCH_AFTER_HALF * 9 && !GetAsyncKeyState(VK_ESCAPE));

    ExitProcess( 0 );
}
