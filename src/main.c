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
static EGLContext ctx = EGL_NO_CONTEXT;
static EGLConfig cfg;
static bool need_context_rebind;
extern int main(int argc, char *argv[]);

static struct android_app *g_app;
static struct android_poll_source *data;

static void handle_cmd(struct android_app *app, int32_t cmd)
{
	switch (cmd) {
	case APP_CMD_INIT_WINDOW: {
		if (app->window) {
			if (need_context_rebind) {
				EGLint displayFormat = 0;
				eglGetConfigAttrib(display, cfg, EGL_NATIVE_VISUAL_ID, &displayFormat);
				need_context_rebind = false;
				surface = eglCreateWindowSurface(display, cfg, app->window, NULL);
				eglMakeCurrent(display, surface, surface, ctx);
			} else {
				display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
				eglInitialize(display, NULL, NULL);
				EGLint ncfg;
				eglChooseConfig(display, (EGLint[]){ EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE }, &cfg, 1, &ncfg);
				ctx = eglCreateContext(display, cfg, EGL_NO_CONTEXT, (EGLint[]){ EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE });
				surface = eglCreateWindowSurface(display, cfg, app->window, NULL);
				eglMakeCurrent(display, surface, surface, ctx);
			}
		}
	} break;
	case APP_CMD_TERM_WINDOW: {
		if (display != EGL_NO_DISPLAY) {
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

			if (surface != EGL_NO_SURFACE) {
				eglDestroySurface(display, surface);
				surface = EGL_NO_SURFACE;
			}

			need_context_rebind = true;
		}

	} break;
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
	}
	ANativeActivity_setWindowFlags(g_app->activity, AWINDOW_FLAG_FULLSCREEN, 0);
	AConfiguration_setOrientation(g_app->config, ACONFIGURATION_ORIENTATION_LAND);
}

bool window_should_close()
{
	struct android_poll_source *src;
	int events;
	while (ALooper_pollOnce(0, NULL, &events, (void **)&src) >= 0) {
		if (src)
			src->process(g_app, src);
	}
	return g_app->destroyRequested;
}

int main(int argc, char *argv[])
{
	init_platform();
	glClearColor(0.392f, 0.584f, 0.929f, 1.0f);
	while (!window_should_close()) {
		glClear(GL_COLOR_BUFFER_BIT);
		eglSwapBuffers(display, surface);
	}
}
