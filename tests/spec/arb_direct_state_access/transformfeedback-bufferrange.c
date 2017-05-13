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

/** @file transformfeedback-bufferrange.c
 * Simple transform feedback test drawing GL_POINTS and making use of the
 * transform-feedback-related direct state access entry points.
 *
 * Greatly inspired from ext_transform_feedback/points.c.
 *
 * From OpenGL 4.5, section 13.2.2 "Transform Feedback Primitive Capture",
 * page 422:
 *
 * "void TransformFeedbackBufferRange(uint xfb, uint index, uint buffer,
 *                                    intptr offset, sizeiptr size);
 *
 * xfb must be zero, indicating the default transform feedback object, or the
 * name of an existing transform feedback object. buffer must be zero or the
 * name of an existing buffer object.
 *
 * TransformFeedbackBufferRange and TransformFeedbackBufferBase behave
 * similarly to BindBufferRange and BindBufferBase, respectively, except
 * that the target of the operation is xfb, and they do not affect any binding
 * to the generic TRANSFORM_FEEDBACK_BUFFER target.
 *
 * Errors
 * An INVALID_OPERATION error is generated if xfb is not zero or the name
 *  of an existing transform feedback object.
 * An INVALID_VALUE error is generated if buffer is not zero or the name of
 *  an existing buffer object.
 * An INVALID_VALUE error is generated if index is greater than or equal
 *  to the number of binding points for transform feedback, as described in
 *  section 6.7.1.
 * An INVALID_VALUE error is generated by TransformFeedbackBufferRange
 *  if offset is negative.
 * An INVALID_VALUE error is generated by TransformFeedbackBufferRange
 *  if size is less than or equal to zero.
 * An INVALID_VALUE error is generated by TransformFeedbackBufferRange
 *  if offset or size do not satisfy the constraints described for those
 * parameters for transform feedback array bindings, as described in
 *  section 6.7.1."
 */

#include "piglit-util-gl.h"
#include "dsa-utils.h"

PIGLIT_GL_TEST_CONFIG_BEGIN
	config.supports_gl_core_version = 31;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;
PIGLIT_GL_TEST_CONFIG_END


static GLuint prog;
static GLuint xfb_buf[3], input_buf, vao;
static const int xfb_buf_size = 500;
static const int offset = 16;

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
static const GLfloat out1_ret[NUM_INPUTS] = { 0.0, 1.0, 2.0, 4.0};
static const GLfloat out2_ret[NUM_INPUTS] = {-2.0, 0.0, 2.0, 6.0};

void
piglit_init(int argc, char **argv)
{
	static const char *varyings[] = { "valOut1", "valOut2" };
	GLint inAttrib;

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
}

static bool
equal(float a, float b)
{
	return fabsf(a - b) < piglit_tolerance[0];
}

enum piglit_result
piglit_display(void)
{
	GLint max_bind_points = 0;
	GLuint q, num_prims;
	bool pass = true, test = true;
	GLfloat *v, *w;
	int i;

	/* init the transform feedback buffers */
	glGenBuffers(3, xfb_buf);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buf[2]);
	piglit_check_gl_error(GL_NO_ERROR);

	/* Fetch the number of bind points */
	glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_bind_points);
	PIGLIT_SUBTEST_ERROR(GL_NO_ERROR, pass,
			     "fetch maximum number of bind points");

	if (piglit_khr_no_error)
		goto valid_calls;

	/* bind a non-existing transform feedback BO */
	glTransformFeedbackBufferRange(1337, 0, 0, 0, 4096);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_OPERATION, pass,
			     "bind non-existing transform feedback BO");

	/* bind a non-existing output BO */
	glTransformFeedbackBufferRange(0, 0, 1337, 0, 4096);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass,
			     "bind a non-existing output BO");

	/* bind to a negative index */
	glTransformFeedbackBufferRange(0, -1, xfb_buf[2], 0, 4096);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass, "bind negative index");

	/* bind to an index == max */
	glTransformFeedbackBufferRange(0, max_bind_points, xfb_buf[2], 0,
			4096);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass, "bind to index == "
			     "max_bind_points (%i)", max_bind_points);

	/* bind at a non-aligned offset */
	glTransformFeedbackBufferRange(0, 0, xfb_buf[2], 3, 4096);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass,
			     "bind at a non-aligned offset");

	/* bind with a non-aligned size */
	glTransformFeedbackBufferRange(0, 0, xfb_buf[2], 0, 4095);
	PIGLIT_SUBTEST_ERROR(GL_INVALID_VALUE, pass,
			     "bind with a non-aligned size");

valid_calls:
	/* Set up the transform feedback buffer */
	for (i = 0; i < 2; i++) {
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buf[i]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
				 xfb_buf_size, NULL, GL_STREAM_READ);
		glTransformFeedbackBufferRange(0, i, xfb_buf[i], offset,
					       xfb_buf_size);
		piglit_check_gl_error(GL_NO_ERROR);
	}

	/* Set up the query that checks the # of primitives handled */
	glGenQueries(1, &q);
	glBeginQuery(GL_PRIMITIVES_GENERATED, q);
	piglit_check_gl_error(GL_NO_ERROR);

	/* do the transform feedback */
	glBeginTransformFeedback(GL_POINTS);
	glBindBuffer(GL_ARRAY_BUFFER, input_buf);
	glDrawArrays(GL_POINTS, 0, NUM_INPUTS);
	glEndTransformFeedback();

	glEndQuery(GL_PRIMITIVES_GENERATED);

	/* check the number of primitives */
	glGetQueryObjectuiv(q, GL_QUERY_RESULT, &num_prims);
	glDeleteQueries(1, &q);
	printf("%u primitives generated:\n", num_prims);
	if (num_prims != NUM_INPUTS) {
		printf("Incorrect number of prims generated.\n");
		printf("Found %u, expected %u\n", num_prims, NUM_INPUTS);
		pass = false;
		test = false;
	}

	/* check the result */
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buf[0]);
	v = (GLfloat *)((GLbyte *)glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY) + offset);
	piglit_check_gl_error(GL_NO_ERROR);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buf[1]);
	w = (GLfloat *)((GLbyte *)glMapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY) + offset);
	piglit_check_gl_error(GL_NO_ERROR);

	for (i = 0; i < num_prims; i++) {
		printf("%2d: (%2.0g, %2.0g) : ", i, v[i], w[i]);
		if (!equal(v[i], out1_ret[i]) || !equal(w[i], out2_ret[i])) {
			printf("NOK, expected (%2.0g, %2.0g)\n",
			       out1_ret[i], out2_ret[i]);
			test = false;
		} else
			printf("OK\n");
	}
	PIGLIT_SUBTEST_CONDITION(test, pass, "general test");

	piglit_present_results();

	/* clean-up everything */
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buf[0]);
	glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_buf[1]);
	glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	glDeleteBuffers(3, xfb_buf);
	glDeleteBuffers(1, &input_buf);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(prog);

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
