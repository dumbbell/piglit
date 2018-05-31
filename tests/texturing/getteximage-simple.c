/**
 * @file getteximage-simple.c
 *
 * Extremely basic test to check whether image data can be retrieved.
 *
 * Note that the texture is used in a full frame of rendering before
 * the readback, to ensure that buffer manager related code for uploading
 * texture images is executed before the readback.
 *
 * This used to crash for R300+bufmgr.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

static int test_getteximage(GLubyte *data)
{
	GLubyte compare[4096];
	int i;

	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, compare);

	for(i = 0; i < 4096; ++i) {
		if (data[i] != compare[i]) {
			const unsigned pixel = i / 4;
			const unsigned channel = i % 4;
			printf("GetTexImage() returns incorrect data in byte %i\n", i);
			printf("    corresponding to (%i,%i) channel %i\n", pixel % 64, pixel / 64, channel);
			printf("    expected: %i\n", data[i]);
			printf("    got: %i\n", compare[i]);
			return 0;
		}
	}

	return 1;
}

enum piglit_result
piglit_display(void)
{
	int pass;

	GLubyte data[4096]; /* 64*16*4 */
	for(int i = 0; i < 4096; ++i)
		data[i] = rand() & 0xff;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(1, 0);
	glVertex2f(1, 0);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(0, 1);
	glEnd();

	piglit_present_results();

	pass = test_getteximage();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void piglit_init(int argc, char **argv)
{
	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	piglit_gen_ortho_projection(0.0, 1.0, 0.0, 1.0, -2.0, 6.0, GL_FALSE);
}
