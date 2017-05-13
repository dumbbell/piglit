/*
 * Copyright 2015 Intel Corporation
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

/** @file namedrenderbuffer.c
 *
 * Tests NamedRenderbufferStorage* functions to see if they behaves in the
 * expected way, throwing the correct errors, etc. Also test
 * glGetRenderbufferParameteriv.
 *
 * From OpenGL 4.5, section 9.2.4 "Renderbuffer Objects", page 298:
 *
 * "void NamedRenderbufferStorageMultisample(uint renderbuffer, sizei samples,
 *                                           enum internalformat,sizei width,
 *                                           sizei height );
 *
 * [...]
 * For NamedRenderbufferStorageMultisample, renderbuffer is the name of the
 * renderbuffer object. internalformat must be color-renderable,
 * depth-renderable, or stencilrenderable (as defined in section 9.4). width
 * and height are the dimensions in pixels of the renderbuffer. Upon success,
 * RenderbufferStorageMultisample deletes any existing data store for the
 * renderbuffer image, and the contents of the data store are undefined.
 * RENDERBUFFER_WIDTH is set to width, RENDERBUFFER_HEIGHT is set to
 * height, and RENDERBUFFER_INTERNAL_FORMAT is set to internalformat.
 * If samples is zero, then RENDERBUFFER_SAMPLES is set to zero. Otherwise
 * samples represents a request for a desired minimum number of samples. Since
 * different implementations may support different sample counts for
 * multisampled rendering, the actual number of samples allocated for the
 * renderbuffer image is implementation-dependent. However, the resulting value
 * for RENDERBUFFER_SAMPLES is guaranteed to be greater than or equal to
 * samples and no more than the next larger sample count supported by the
 * implementation. A GL implementation may vary its allocation of internal
 * component resolution based on any *RenderbufferStorageMultisample parameter
 * (except target and renderbuffer), but the allocation and chosen internal
 * format must not be a function of any other state and cannot be changed once
 * they are established.
 *
 * Errors
 * An INVALID_ENUM error is generated by RenderbufferStorageMultisample
 *  if target is not RENDERBUFFER.
 * An INVALID_OPERATION error is generated by NamedRenderbufferStorageMultisample
 *  if renderbuffer is not the name of an existing renderbuffer object.
 * An INVALID_VALUE error is generated if samples, width, or height is negative.
 * An INVALID_OPERATION error is generated if samples is greater than the
 *  maximum number of samples supported for internalformat (see
 *  GetInternalformativ in section 22.3).
 * An INVALID_ENUM error is generated if internalformat is not one of the
 *  color-renderable, depth-renderable, or stencil-renderable formats defined
 *  in section 9.4.
 * An INVALID_VALUE error is generated if either width or height is greater
 *  than the value of MAX_RENDERBUFFER_SIZE.
 *
 * void NamedRenderbufferStorage(uint renderbuffer, enum internalformat,
 *                               sizei width, sizei height);
 * are equivalent to [...] NamedRenderbufferStorageMultisample(renderbuffer, 0,
 * internalformat, width, height);"
 */

#include "piglit-util-gl.h"
#include "dsa-utils.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_core_version = 31;

	config.window_visual = PIGLIT_GL_VISUAL_RGBA |
		PIGLIT_GL_VISUAL_DOUBLE;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

void
piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_ARB_direct_state_access");
	piglit_require_extension("GL_ARB_framebuffer_object");
}

enum piglit_result
piglit_display(void)
{
	bool pass = true;
	GLuint ids[10], genID;
	GLint size, width;

	/* Test retrieving information about an unexisting buffer */
	glCreateRenderbuffers(10, ids);
	piglit_check_gl_error(GL_NO_ERROR);

	if (piglit_khr_no_error)
		goto valid_calls;

	/* Check some various cases of errors */
	glNamedRenderbufferStorageMultisample(1337, 0, GL_RGBA, 1024, 768);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_OPERATION, pass, "set unexisting "
			     "renderbuffer");

	glGetNamedRenderbufferParameteriv(1337, GL_RENDERBUFFER_WIDTH, &width);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_OPERATION, pass, "get unexisting "
			     "renderbuffer");

	glGetNamedRenderbufferParameteriv(ids[0], GL_TRUE, &width);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_ENUM, pass, "get unexisting parameter");

	/* Test retrieving information on an un-initialized buffer */
	glGenRenderbuffers(1, &genID);
	piglit_check_gl_error(GL_NO_ERROR);

	glGetNamedRenderbufferParameteriv(1337, GL_RENDERBUFFER_WIDTH, &width);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_OPERATION, pass, "get uninitialized "
			     "renderbuffer");

	/* Test the width/heights limits */
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &size);

	glNamedRenderbufferStorageMultisample(ids[0], 0, GL_RGBA, -1, 768);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass, "width < 0");

	glNamedRenderbufferStorageMultisample(ids[0], 0, GL_RGBA, size + 1,
					      768);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass,
			     "width == MAX_RENDER_SIZE(%d) + 1", size);

	glNamedRenderbufferStorageMultisample(ids[0], 0, GL_RGBA, 1024, -1);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass, "height < 0");

	glNamedRenderbufferStorageMultisample(ids[0], 0, GL_RGBA, 1024,
					      size + 1);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass,
			     "height == MAX_RENDER_SIZE(%d) + 1", size);

	/* Test the samples limits */
	glGetIntegerv(GL_MAX_SAMPLES, &size);

	glNamedRenderbufferStorageMultisample(ids[0], -1, GL_RGBA, 1024, 768);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass, "samples < 0");

	glNamedRenderbufferStorageMultisample(ids[0], size + 1, GL_RGBA, 1024,
					      768);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_OPERATION, pass,
			     "samples == MAX_SAMPLES(%d) + 1", size);

	/* Misc tests */
	glNamedRenderbufferStorageMultisample(ids[0], 0, GL_TRUE, 1024, 768);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_ENUM, pass, "invalid internalformat");

valid_calls:
	/* bind one buffer so as we can check we never change its state */
	glBindRenderbuffer(GL_RENDERBUFFER, ids[1]);
	piglit_check_gl_error(GL_NO_ERROR);

	/* Test to change the parameters of an unbound renderbuffer */
	glNamedRenderbufferStorageMultisample(ids[0], 0, GL_RGBA, 1024,
					      768);
	PIGLIT_SUBTEST_ERROR(GL_NO_ERROR, pass, "update unbound buffer");

	glGetNamedRenderbufferParameteriv(ids[0], GL_RENDERBUFFER_WIDTH,
			&width);
	piglit_check_gl_error(GL_NO_ERROR);
	PIGLIT_SUBTEST_CONDITION(width == 1024, pass,
			 "width of the unbound buffer updated");

	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH,
			&width);
	piglit_check_gl_error(GL_NO_ERROR);
	PIGLIT_SUBTEST_CONDITION(width == 0, pass,
			 "width of the bound buffer unchanged");

	/* clean up */
	glDeleteRenderbuffers(10, ids);

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
