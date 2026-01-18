#ifndef __GLIBC_USE
#define __GLIBC_USE(x) 0
#endif
#include <stddef.h>
#include <android_native_app_glue.h>
#include <android/window.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
static EGLDisplay display = EGL_NO_DISPLAY;
static EGLSurface surface = EGL_NO_SURFACE;
extern int main(int argc, char *argv[]);

static struct android_app *g_app;
static struct android_poll_source *data;

static void handle_cmd(struct android_app *app, int32_t cmd)
{
	if (cmd == APP_CMD_INIT_WINDOW && app->window) {
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		eglInitialize(display, NULL, NULL);
		EGLConfig cfg;
		EGLint ncfg;
		eglChooseConfig(display, (EGLint[]){ EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE }, &cfg, 1, &ncfg);
		EGLContext ctx = eglCreateContext(display, cfg, EGL_NO_CONTEXT, (EGLint[]){ EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE });
		surface = eglCreateWindowSurface(display, cfg, app->window, NULL);
		eglMakeCurrent(display, surface, surface, ctx);
	}
}

void android_main(struct android_app *app)
{
	g_app = app;

	char arg0[] = "androtest";

	(void)main(1, (char *[]){ arg0, NULL });

	ANativeActivity_finish(app->activity);

	int pollResult = 0;
	int pollEvents = 0;

	while (!app->destroyRequested) {
		while ((pollResult = ALooper_pollOnce(0, NULL, &pollEvents, (void **)&data)) > ALOOPER_POLL_TIMEOUT) {
			if (data != NULL)
				data->process(app, data);
		}
	}
}

void init_platform()
{
	g_app->onAppCmd = handle_cmd;
	while (!g_app->window) {
		int pollResult = 0;
		int pollEvents = 0;
		while ((pollResult = ALooper_pollOnce(0, NULL, &pollEvents, ((void **)&data)) > ALOOPER_POLL_TIMEOUT)) {
			if (data != NULL)
				data->process(g_app, data);
		}

		// struct android_poll_source *src;
		// if (ALooper_pollAll(-1, NULL, NULL, (void **)&src) >= 0)
		// 	if (src)
		// 		src->process(g_app, src);
	}
	ANativeActivity_setWindowFlags(g_app->activity, AWINDOW_FLAG_FULLSCREEN, 0);
	AConfiguration_setOrientation(g_app->config, ACONFIGURATION_ORIENTATION_LAND);
}
int main(int argc, char *argv[])
{
	init_platform();
	glClearColor(0.392f, 0.584f, 0.929f, 1.0f);
	while (1) {
		glClear(GL_COLOR_BUFFER_BIT);
		eglSwapBuffers(display, surface);
	}
}
