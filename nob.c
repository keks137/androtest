#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

#include "nobfiles.h"
#define GAME_NAME "androtest"
#define BIN_FOLDER "bin/"
#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

#define NDK_PATH "/home/archie/android_sdk/ndk/29.0.14206865/"
#define ANDROID_LINUX_CC "armv7a-linux-androideabi29-clang"
#define ANDROID_LINUX_SDK "/home/archie/android_sdk/"
#define ANDROID_LINUX_BUILD_TOOLS ANDROID_LINUX_SDK "build-tools/29.0.3/"

#define ANDROIDLINUXBASE BUILD_FOLDER "androidlinux/"

enum {
	PLATFORM_UNKNOW,
	PLATFORM_LINUX,
	PLATFORM_WINDOWS,

} CurrentPlatform_e;

#ifdef __linux__
#include <unistd.h>
#define CURRENT_PLATFORM PLATFORM_LINUX
#else
#define CURRENT_PLATFORM PLATFORM_UNKNOWN
#endif

void android_linux_cc_flags(Nob_Cmd *cmd)
{
	nob_cmd_append(cmd, NDK_PATH "/toolchains/llvm/prebuilt/linux-x86_64/bin/" ANDROID_LINUX_CC);
	nob_cmd_append(cmd, "-Wall");
	nob_cmd_append(cmd, "-Wextra");
	nob_cmd_append(cmd, "-L./build");
	nob_cmd_append(cmd, "-landroid");
	nob_cmd_append(cmd, "-lm");
	nob_cmd_append(cmd, "-lEGL");
	nob_cmd_append(cmd, "-llog");
	nob_cmd_append(cmd, "-lGLESv3");
	nob_cmd_append(cmd, "-u");
	nob_cmd_append(cmd, "ANativeActivity_onCreate");
	nob_cmd_append(cmd, "-shared");
	nob_cmd_append(cmd, "-fPIC");
	nob_cmd_append(cmd, "-I" NDK_PATH "/sources/android/native_app_glue");
	nob_cmd_append(cmd, "-I" NDK_PATH "/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include");
	nob_cmd_append(cmd, "-I" NDK_PATH "/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/arm-linux-androideabi");
}

bool build_linux_linux()
{
	Nob_Cmd cmd = { 0 };

	nob_cc(&cmd);
	nob_cc_flags(&cmd);
	nob_cmd_append(&cmd, "-ggdb");
	nob_cc_inputs(&cmd, SRC_FOLDER "main.c");
	nob_cc_output(&cmd, BIN_FOLDER GAME_NAME);
	;
	;

	nob_temp_reset();
	nob_cmd_append(&cmd, "-lm");

	if (!nob_cmd_run(&cmd))
		return false;
	return true;
}

bool build_android_linux()
{
	Nob_Cmd cmd = { 0 };

	nob_mkdir_if_not_exists(ANDROIDLINUXBASE);
	nob_mkdir_if_not_exists(ANDROIDLINUXBASE "lib/");
	nob_mkdir_if_not_exists(ANDROIDLINUXBASE "lib/armeabi-v7a");

	nob_cmd_append(&cmd, "pwd");

	if (!nob_cmd_run(&cmd))
		return false;

	android_linux_cc_flags(&cmd);
	nob_cmd_append(&cmd, "-ggdb");
	nob_cc_inputs(&cmd, SRC_FOLDER "main.c");
	nob_cmd_append(&cmd, NDK_PATH "/sources/android/native_app_glue/android_native_app_glue.c");
	nob_cc_output(&cmd, ANDROIDLINUXBASE "lib/armeabi-v7a/libmain.so");

	nob_temp_reset();
	nob_cmd_append(&cmd, "-lm");
	if (!nob_cmd_run(&cmd))
		return false;

	chdir(ANDROIDLINUXBASE);
	const char *stringsxmlpath = "res/values/strings.xml";
	if (!nob_file_exists(stringsxmlpath)) {
		nob_mkdir_if_not_exists("res");
		nob_mkdir_if_not_exists("res/values");
		FILE *fp = fopen(stringsxmlpath, "w+");
		if (fp == NULL)
			return false;
		fprintf(fp, "%s", stringsxml);
		fclose(fp);
	}
	const char *androidmanifestxmlpath = "AndroidManifest.xml";
	if (!nob_file_exists(androidmanifestxmlpath)) {
		FILE *fp = fopen(androidmanifestxmlpath, "w+");
		if (fp == NULL)
			return false;
		fprintf(fp, "%s", androidmanifestxml);
		fclose(fp);
	}

	nob_cmd_append(&cmd, ANDROID_LINUX_BUILD_TOOLS "/aapt");
	nob_cmd_append(&cmd, "package");
	nob_cmd_append(&cmd, "-f");
	nob_cmd_append(&cmd, "-F", "base.apk");
	nob_cmd_append(&cmd, "-I", ANDROID_LINUX_SDK "platforms/android-29/android.jar");
	nob_cmd_append(&cmd, "-M", androidmanifestxmlpath);
	if (!nob_cmd_run(&cmd))
		return false;

	nob_cmd_append(&cmd, "unzip", "-o", "base.apk", "-d");
	nob_cmd_append(&cmd, "apk_temp");
	if (!nob_cmd_run(&cmd))
		return false;
	nob_cmd_append(&cmd, "cp", "-r", "lib", "apk_temp");
	if (!nob_cmd_run(&cmd))
		return false;
	chdir("apk_temp");
	nob_cmd_append(&cmd, "zip", "-r", "../unsigned.apk", ".");
	if (!nob_cmd_run(&cmd))
		return false;
	chdir("..");

	nob_cmd_append(&cmd, ANDROID_LINUX_BUILD_TOOLS "zipalign", "-f", "-p", "4", "unsigned.apk", "aligned.apk");
	if (!nob_cmd_run(&cmd))
		return false;

	if (!nob_file_exists("debug.keystore")) {
		nob_cmd_append(&cmd, "keytool", "-genkey", "-v", "-keystore", "debug.keystore",
			       "-alias", "androiddebugkey", "-storepass", "android", "-keypass", "android",
			       "-keyalg", "RSA", "-keysize", "2048", "-validity", "10000",
			       "-dname", "CN=Android Debug,O=Android,C=US");

		if (!nob_cmd_run(&cmd))
			return false;
	}

	nob_cmd_append(&cmd, ANDROID_LINUX_BUILD_TOOLS "apksigner", "sign", "--ks", "debug.keystore", "--ks-pass", "pass:android", "--key-pass", "pass:android", "aligned.apk");
	if (!nob_cmd_run(&cmd))
		return false;
	chdir("../..");
	nob_cmd_append(&cmd, "cp", ANDROIDLINUXBASE "aligned.apk", BIN_FOLDER GAME_NAME ".apk");
	if (!nob_cmd_run(&cmd))
		return false;

	return true;
}

bool build_current_platform()
{
	bool ret = false;
	switch (CURRENT_PLATFORM) {
	case PLATFORM_LINUX:
		ret = build_linux_linux();
		break;
	case PLATFORM_UNKNOW:
		nob_log(NOB_ERROR, "Unknown Platform");
		break;
	}
	return ret;
}

int main(int argc, char **argv)
{
	NOB_GO_REBUILD_URSELF(argc, argv);

	if (!nob_mkdir_if_not_exists(BIN_FOLDER))
		return 1;
	if (!nob_mkdir_if_not_exists(BUILD_FOLDER))
		return 1;

	if (!build_android_linux())
		return 1;
	// if (argc >= 2) {
	// 	if (strcmp("android", argv[1]) == 0) {
	// 		if (!build_android_linux())
	// 			return 1;
	// 	} else {
	// 		nob_log(NOB_ERROR, "Unknown target platform: %s", argv[1]);
	// 	}
	// } else {
	// 	if (!build_current_platform())
	// 		return 1;
	// }

	return 0;
}
