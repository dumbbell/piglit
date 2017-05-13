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

/** @file gettransformfeedback.c
 * Simple transform feedback test drawing GL_POINTS and checking that all the
 * state is well reported by the GetTransformFeedback*() functions.
 *
 * Greatly inspired from ext_transform_feedback/points.c.
 *
 * From OpenGL 4.5, section 22.4 "Transform Feedback State Queries", page 552:
 *
 * "void GetTransformFeedbackiv(uint xfb, enum pname, int *param);
 * void GetTransformFeedbacki v(uint xfb, enum pname, uint index, int *param);
 * void GetTransformFeedbacki64 v(uint xfb, enum pname,uint index,
 *                                int64 *param);
 *
 * xfb must be zero, indicating the default transform feedback object, or the
 * name of an existing transform feedback object. pname must be one of the
 * tokens listed in table 23.48, depending on the command name as shown in the
 * errors section below. For indexed state, index is the index of the transform
 * feedback stream. param is the address of a variable to receive the result of
 * the query.
 *
 * Errors
 * An INVALID_OPERATION error is generated by GetTransformFeedback* if xfb is
 *  not zero or the name of an existing transform feedback object.
 * An INVALID_ENUM error is generated by GetTransformFeedbackiv if pname is not
 *  TRANSFORM_FEEDBACK_PAUSED or TRANSFORM_FEEDBACK_ACTIVE.
 * An INVALID_ENUM error is generated by GetTransformFeedbacki v if pname is
 *  not TRANSFORM_FEEDBACK_BUFFER_BINDING.
 * An INVALID_ENUM error is generated by GetTransformFeedbacki64 v if pname is
 *  not TRANSFORM_FEEDBACK_BUFFER_START or TRANSFORM_FEEDBACK_BUFFER_SIZE.
 * An INVALID_VALUE error is generated by GetTransformFeedbacki v and
 *  GetTransformFeedbacki64 v if index is greater than or equal to the number
 *  of binding points for transform feedback, as described in section 6.7.1."
 */

#include "piglit-util-gl.h"
#include "dsa-utils.h"

PIGLIT_GL_TEST_CONFIG_BEGIN
	config.supports_gl_core_version = 31;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;
PIGLIT_GL_TEST_CONFIG_END


static GLuint prog;
static GLuint xfb_buf[3], input_buf, vao;
static bool pass = true;

static const char *vstext = {
	"#version 140\n"
	"in float valIn;"
	"out float valOut1;"
	"out float valOut2;"
	"void main() {"
	"   valOut1 = valIn + 1;"
	"   valOut2 = valIn * 2;"
	"}"
};

#define NUM_INPUTS 4
static const GLfloat inputs[NUM_INPUTS] =   {-1.0, 0.0, 1.0, 3.0};

struct context_t {
	struct tbo_state_t {
		GLuint binding;
		GLint64 start;
		GLint64 size;
	} bo_state[3];
	bool active;
	bool paused;
} ctx;

void
check_active_paused_state(const char *test_name)
{
	GLint param;

	glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_PAUSED, &param);
	piglit_check_gl_error(GL_NO_ERROR);
	PIGLIT_SUBTEST_CONDITION(param == ctx.paused, pass, "%s: paused state "
				 "valid", test_name);

	glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_ACTIVE, &param);
	piglit_check_gl_error(GL_NO_ERROR);
	PIGLIT_SUBTEST_CONDITION(param == ctx.active, pass, "%s: active state "
				 "valid", test_name);
}

void
check_binding_state(const char *test_name)
{
	GLint64 param64;
	GLint param;
	int i;

	for (i = 0; i < 3; i++) {
		glGetTransformFeedbacki_v(0,
					  GL_TRANSFORM_FEEDBACK_BUFFER_BINDING,
					  i, &param);
		piglit_check_gl_error(GL_NO_ERROR);
		PIGLIT_SUBTEST_CONDITION(param == ctx.bo_state[i].binding, pass,
				 "%s: bound buffer %i valid", test_name, i);

		glGetTransformFeedbacki64_v(0,
					    GL_TRANSFORM_FEEDBACK_BUFFER_START,
					    i, &param64);
		piglit_check_gl_error(GL_NO_ERROR);
		PIGLIT_SUBTEST_CONDITION(param64 ==  ctx.bo_state[i].start, pass,
				"%s: bound buffer %i start valid", test_name,
				 i);

		glGetTransformFeedbacki64_v(0,
					    GL_TRANSFORM_FEEDBACK_BUFFER_SIZE,
					    i, &param64);
		piglit_check_gl_error(GL_NO_ERROR);
		PIGLIT_SUBTEST_CONDITION(param64 ==  ctx.bo_state[i].size, pass,
				 "%s: bound buffer %i size valid", test_name,
				 i);
	}
}

void
check_invalid_queries()
{
	GLint64 param64;
	GLint param;

	if (piglit_khr_no_error)
		return;

	glGetTransformFeedbackiv(0, GL_TRANSFORM_FEEDBACK_BINDING, &param);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_ENUM, pass,
		"glGetTransformFeedbackiv: fetch invalid attribute");

	glGetTransformFeedbacki_v(0, GL_TRANSFORM_FEEDBACK_ACTIVE, 0, &param);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_ENUM, pass,
		"glGetTransformFeedbacki_v: fetch invalid attribute");

	glGetTransformFeedbacki64_v(0, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING,
				    0, &param64);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_ENUM, pass,
		"glGetTransformFeedbacki64_v: fetch invalid attribute");
}

void
piglit_init(int argc, char **argv)
{
	static const char *varyings[] = { "valOut1", "valOut2" };
	GLuint size, start;
	GLint inAttrib;
	int i;

	/* Check the driver. */
	piglit_require_extension("GL_ARB_transform_feedback3");
	piglit_require_extension("GL_ARB_direct_state_access");

	/* Create shaders. */
	prog = piglit_build_simple_program_unlinked(vstext, NULL);
	glTransformFeedbackVaryings(prog, 2, varyings,
					GL_SEPARATE_ATTRIBS);
	glLinkProgram(prog);
	if (!piglit_link_check_status(prog)) {
		glDeleteProgram(prog);
		piglit_report_result(PIGLIT_FAIL);
	}
	glUseProgram(prog);

	/* Set up the Vertex Array Buffer */
	glEnable(GL_VERTEX_ARRAY);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	/* Set up the input data buffer */
	glGenBuffers(1, &input_buf);
	glBindBuffer(GL_ARRAY_BUFFER, input_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(inputs), inputs, GL_STATIC_DRAW);
	inAttrib = glGetAttribLocation(prog, "valIn");
	piglit_check_gl_error(GL_NO_ERROR);
	glVertexAttribPointer(inAttrib, 1, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(inAttrib);

	/* set the initial state */
	ctx.active = false;
	ctx.paused = false;
	for (i = 0; i < 3; i++) {
		ctx.bo_state[i].binding = 0;
		ctx.bo_state[i].start = 0;
		ctx.bo_state[i].size = 0;
	}
	check_active_paused_state("initial state");
	check_binding_state("initial state");

	/* Set up the transform feedback buffer */
	glGenBuffers(3, xfb_buf);
	for (i = 0; i < 2; i++) {
		start = rand() & 0xFC;
		size = 0x100 + (rand() & 0xFFC);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buf[i]);
		piglit_check_gl_error(GL_NO_ERROR);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
			     start + size, NULL, GL_STREAM_READ);
		piglit_check_gl_error(GL_NO_ERROR);
		glTransformFeedbackBufferRange(0, i, xfb_buf[i], start, size);
		piglit_check_gl_error(GL_NO_ERROR);

		ctx.bo_state[i].binding = xfb_buf[i];
		ctx.bo_state[i].start = start;
		ctx.bo_state[i].size = size;
	}

	check_binding_state("post-binding state");
}

enum piglit_result
piglit_display(void)
{
	check_invalid_queries();

	glClear(GL_COLOR_BUFFER_BIT);

	glBeginTransformFeedback(GL_POINTS);

	ctx.active = true;
	check_active_paused_state("TransformFeedback started");

	glPauseTransformFeedback();

	ctx.paused = true;
	check_active_paused_state("TransformFeedback paused");

	glResumeTransformFeedback();

	ctx.paused = false;
	check_active_paused_state("TransformFeedback resumed");

	glEndTransformFeedback();

	ctx.active = false;
	check_active_paused_state("TransformFeedback ended");

	/* clean-up everything */
	glDeleteBuffers(3, xfb_buf);
	glDeleteBuffers(1, &input_buf);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(prog);

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
