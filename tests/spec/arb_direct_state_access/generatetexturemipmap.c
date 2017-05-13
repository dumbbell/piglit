/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file generatetexturemipmaps.c
 * Verify the side-effect-free operation of glGenerateTextureMipmap
 *
 * After calling glGenerateTextureMipmap, there are 3 things that need to be
 * verified:
 *
 * 1. The texture bindings have not been modified.
 *
 * 2. Textures not passed to glGenerateTextureMipmap have not had their data
 *    modified.
 *
 * 3. The texture that was passed to glGenerateTextureMipmap has the correct
 *    data in its mipmaps.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_core_version = 31;
	config.supports_gl_compat_version = 20;

	config.window_visual = PIGLIT_GL_VISUAL_RGBA | 
		PIGLIT_GL_VISUAL_DOUBLE;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

/* 2x2 block of red pixels. */
static const float red[2 * 2 * 4] = {
	1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0,
	1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0
};

/* 4x4 block of green pixels. */
static const float green[4 * 4 * 4] = {
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
	0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0
};

/* 4x4 block of blue pixels. */
static const float blue[4 * 4 * 4] = {
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,
	0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0
};

void
piglit_init(int argc, char **argv)
{
	GLuint tex[2];
	GLuint bound;
	bool pass = true;
	unsigned i;

	piglit_require_extension("GL_ARB_direct_state_access");

	/* Create two textures.  Populate both with full mipmap stacks.
	 * Ensure that the data for levels > 0 is not the data that would be
	 * generated by glGenerateMipmap or glGenerateTextureMipmap.
	 */
	glGenTextures(2, tex);

	for (i = 0; i <= 1; i++) {
		const float *const base_color = (i == 0) ? blue : green;

		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA,
			     GL_FLOAT, base_color);
		glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 2, 2, 0, GL_RGBA,
			     GL_FLOAT, red);
		glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA8, 1, 1, 0, GL_RGBA,
			     GL_FLOAT, red);
		pass = piglit_check_gl_error(GL_NO_ERROR) && pass;
	}

	/* Generate the mipmap stack for the texture that is not currently
	 * bound to unit 0 while unit 0 is the active unit.
	 */
	glActiveTexture(GL_TEXTURE0);
	glGenerateTextureMipmap(tex[1]);
	pass = piglit_check_gl_error(GL_NO_ERROR) && pass;

	/* Verify that the correct textures are still bound to the correct
	 * texture units.
	 */
	for (i = 0; i <= 1; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *) &bound);
		pass = piglit_check_gl_error(GL_NO_ERROR) && pass;

		if (bound != tex[i]) {
			fprintf(stderr,
				"Wrong texture bound to unit %u.  Got %d, "
				"but expected %d (other texture is %d).\n",
				i, bound, tex[i], tex[1 - i]);
			pass = false;
		}
	}

	/* Verify that each texture has the correct data in levels > 0.
	 *
	 * For the texture bound to unit 0 (which was not passed to
	 * glGenerateTextureMipmap), this should be the original red color.
	 *
	 * For the texture bound to unit 1 (which was passed to
	 * glGenerateTextureMipmap), this should be green generated from the
	 * base level.
	 */
	for (i = 0; i <= 1; i++) {
		const float *const level_color = (i == 0) ? red : green;

		glActiveTexture(GL_TEXTURE0 + i);
		printf("Check level 1 of texture %u...\n", i);
		pass = piglit_probe_texel_rect_rgba(GL_TEXTURE_2D, 1,
						    0, 0, 2, 2,
						    level_color) && pass;
		printf("Check level 2 of texture %u...\n", i);
		pass = piglit_probe_texel_rect_rgba(GL_TEXTURE_2D, 2,
						    0, 0, 1, 1,
						    level_color) && pass;
	}

	piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);
}

enum piglit_result
piglit_display(void)
{
	return PIGLIT_FAIL;
}

