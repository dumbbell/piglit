cmake_minimum_required(VERSION 3.2)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

INCLUDE (GNUInstallDirs)
INCLUDE (CheckCCompilerFlag)
INCLUDE (CheckCSourceCompiles)
INCLUDE (CheckCXXCompilerFlag)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckIncludeFile)
INCLUDE (FindPkgConfig)

# http://www.cmake.org/cmake/help/v3.0/policy/CMP0042.html
if (POLICY CMP0042)
	cmake_policy (SET CMP0042 NEW)
endif()

if (POLICY CMP0072)
        cmake_policy (SET CMP0072 OLD)
endif()

project (piglit)

# Require MinGW
if (MSVC)
	message (FATAL_ERROR "Windows builds require MinGW")
endif ()

find_package(Threads)
find_package(PNG)
if(PNG_FOUND)
	add_definitions(-DPIGLIT_HAS_PNG)
endif(PNG_FOUND)
find_package(X11)
if(X11_FOUND)
	set(PIGLIT_HAS_X11 True)
	add_definitions(-DPIGLIT_HAS_X11)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	set(PIGLIT_BUILD_GLES_TESTS_DEFAULT ON)
else()
	set(PIGLIT_BUILD_GLES_TESTS_DEFAULT OFF)
endif()

option(PIGLIT_BUILD_GL_TESTS "Build tests for OpenGL" ON)
option(PIGLIT_BUILD_GLES1_TESTS "Build tests for OpenGL ES1" ${PIGLIT_BUILD_GLES_TESTS_DEFAULT})
option(PIGLIT_BUILD_GLES2_TESTS "Build tests for OpenGL ES2" ${PIGLIT_BUILD_GLES_TESTS_DEFAULT})
option(PIGLIT_BUILD_GLES3_TESTS "Build tests for OpenGL ES3" ${PIGLIT_BUILD_GLES_TESTS_DEFAULT})
option(PIGLIT_BUILD_CL_TESTS "Build tests for OpenCL" OFF)

if(PIGLIT_BUILD_GL_TESTS)
	find_package(OpenGL REQUIRED)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	option(PIGLIT_USE_WAFFLE "Use Waffle in place of GLUT" ON)
else()
	option(PIGLIT_USE_WAFFLE "Use Waffle in place of GLUT" OFF)
endif()

if(PIGLIT_USE_WAFFLE)
	if (NOT WIN32)
		pkg_check_modules(Waffle REQUIRED waffle-1)

		if(NOT Waffle_FOUND)
			message(FATAL_ERROR "Failed to find Waffle. If Waffle "
			"is not packaged for your distribution, you can get "
			"it at http://www.waffle-gl.org."
		)
		endif()

		# Check the installed waffle version.
		#
		# We cannot reliably check the version with pkg_check_modules(), but
		# instead must check the version manually as below. The problem is that,
		# if one passes a required version to pkg_check_modules(), CMake
		# validates the required version at most once for the lifetime of the
		# source tree.  If someone changes the required version by editing the
		# CMakeLists, CMake fails to detect the new requirement.
		set(Waffle_REQUIRED_VERSION "1.5.0")
		if(Waffle_VERSION VERSION_LESS Waffle_REQUIRED_VERSION)
			message(FATAL_ERROR "Found waffle-${Waffle_VERSION}, but "
			"piglit requires waffle-${Waffle_REQUIRED_VERSION}")
		endif()
	else ()
		find_path(Waffle_INCLUDE_DIRS waffle.h)
		find_library(Waffle_LDFLAGS waffle-1)
		if(Waffle_INCLUDE_DIRS AND Waffle_LDFLAGS)
			set(Waffle_FOUND TRUE)
		else()
			message(FATAL_ERROR "Failed to find Waffle. Get and build Waffle from "
				"http://www.waffle-gl.org and set Waffle_INCLUDE_DIRS and "
				"Waffle_LDFLAGS variables accordingly."
			)
		endif()
	endif ()

	add_definitions(-DPIGLIT_USE_WAFFLE)
	add_definitions(-DWAFFLE_API_VERSION=0x0103)
else()
	find_package(GLUT REQUIRED)

	# The 'REQUIRED' above correctly produces an error for
	# OpenGL, but there's a bug involving FindGLUT.cmake
	# that fails to produce the error as of CMake 2.8.5.
	#
	# Instead, CMake keeps going and eventually spams
	# the console with a message for every target that used
	# e.g. the ${GLUT_INCLUDE_DIR} variable. So it
	# prints a line for basically every single test in piglit.
	#
	# Work around the bug and error out quickly here instead.
	if (NOT GLUT_FOUND)
		message(FATAL_ERROR "GLUT library not found")
	endif()
endif(PIGLIT_USE_WAFFLE)

if(PIGLIT_BUILD_GLES1_TESTS AND NOT PIGLIT_USE_WAFFLE)
	message(FATAL_ERROR "Option PIGLIT_BUILD_GLES1_TESTS requires PIGLIT_USE_WAFFLE")
endif(PIGLIT_BUILD_GLES1_TESTS AND NOT PIGLIT_USE_WAFFLE)

if(PIGLIT_BUILD_GLES2_TESTS AND NOT PIGLIT_USE_WAFFLE)
	message(FATAL_ERROR "Option PIGLIT_BUILD_GLES2_TESTS requires PIGLIT_USE_WAFFLE")
endif(PIGLIT_BUILD_GLES2_TESTS AND NOT PIGLIT_USE_WAFFLE)

if(PIGLIT_BUILD_GLES3_TESTS AND NOT PIGLIT_USE_WAFFLE)
	message(FATAL_ERROR "Option PIGLIT_BUILD_GLES3_TESTS requires PIGLIT_USE_WAFFLE")
endif(PIGLIT_BUILD_GLES3_TESTS AND NOT PIGLIT_USE_WAFFLE)

if(PIGLIT_BUILD_CL_TESTS)
	find_package(OpenCL REQUIRED)
endif(PIGLIT_BUILD_CL_TESTS)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	if(X11_FOUND AND OPENGL_gl_LIBRARY)
		# Assume the system has GLX. In the future, systems may exist
		# with libGL and libX11 but no GLX, but that world hasn't
		# arrived yet.
		set(PIGLIT_HAS_GLX True)
		add_definitions(-DPIGLIT_HAS_GLX)
	endif()

	pkg_check_modules(GBM QUIET gbm>=17.1)
	if(GBM_FOUND)
		set(PIGLIT_HAS_GBM True)
		add_definitions(-DPIGLIT_HAS_GBM)
		set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${GBM_LIBRARIES})
		CHECK_FUNCTION_EXISTS(gbm_bo_map PIGLIT_HAS_GBM_BO_MAP)
		if (PIGLIT_HAS_GBM_BO_MAP)
			add_definitions(-DPIGLIT_HAS_GBM_BO_MAP)
		endif()
	endif(GBM_FOUND)

	pkg_check_modules(WAYLAND QUIET wayland-client wayland-egl)
	if (WAYLAND_FOUND)
		set(PIGLIT_HAS_WAYLAND True)
		add_definitions(-DPIGLIT_HAS_WAYLAND)

		FIND_LIBRARY(HAVE_XKBCOMMON NAMES xkbcommon)
		if(NOT HAVE_XKBCOMMON)
			message(FATAL_ERROR "Wayland support requires xkbcommon. "
				"Failed to find xkbcommon library.")
		endif()
		pkg_check_modules(XKBCOMMON QUIET xkbcommon)
	endif()

	pkg_check_modules(LIBDRM QUIET libdrm)
	pkg_check_modules(LIBDRM_INTEL QUIET libdrm_intel)
	pkg_check_modules(XCB QUIET xcb)
	pkg_check_modules(XCB_DRI2 QUIET xcb-dri2)
	pkg_check_modules(GLPROTO QUIET glproto)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if (PIGLIT_USE_WAFFLE)
		set(PIGLIT_HAS_WGL True)
		add_definitions(-DPIGLIT_HAS_WGL)
	endif()
ENDIF()

IF(PIGLIT_HAS_GLX)
	option(PIGLIT_BUILD_GLX_TESTS "Build tests that require GLX" ON)
ELSE()
	option(PIGLIT_BUILD_GLX_TESTS "Build tests that require GLX" OFF)
ENDIF()

IF(PIGLIT_HAS_WGL)
	option(PIGLIT_BUILD_WGL_TESTS "Build tests that require WGL" ON)
ELSE()
	option(PIGLIT_BUILD_WGL_TESTS "Build tests that require WGL" OFF)
ENDIF()


# Choose to build tests that use dma_buf.
#
# Piglit's dma_buf utilities require xcb-dri2 to gain DRM authentication.
#
# The presence of libdrm is not sufficient. At least one libdrm_${hardware}
# library is also needed.
#
# When building for Intel, libdrm_intel>=2.4.38 is required because support for
# drm-prime arrived in that version.
#
if(LIBDRM_FOUND AND XCB_DRI2_FOUND AND
   ((LIBDRM_INTEL_VERSION VERSION_GREATER "2.4.37") OR
     PIGLIT_HAS_GBM_BO_MAP))
	set(PIGLIT_BUILD_DMA_BUF_TESTS_IS_VALID true)
else()
	set(PIGLIT_BUILD_DMA_BUF_TESTS_IS_VALID false)
endif()

if(PIGLIT_BUILD_DMA_BUF_TESTS_IS_VALID)
	option(PIGLIT_BUILD_DMA_BUF_TESTS "Build tests that use dma_buf" ON)
else()
	option(PIGLIT_BUILD_DMA_BUF_TESTS "Build tests that use dma_buf" OFF)
endif()

# If the users has updated PIGLIT_BUILD_DMA_BUF_TESTS in the CMakeCache, then
# we need to validate it.
if(PIGLIT_BUILD_DMA_BUF_TESTS AND NOT PIGLIT_BUILD_DMA_BUF_TESTS_IS_VALID)
	message(FATAL_ERROR
		"PIGLIT_BUILD_DMA_BUF_TESTS require libdrm, "
		"libdrm_intel>=2.4.38, and xcb-dri2")
endif()

IF(PIGLIT_BUILD_GLX_TESTS)
	pkg_check_modules(GLPROTO REQUIRED glproto)
ENDIF()

set(Python_ADDITIONAL_VERSIONS
    3.7 3.6 3.5 3.4 2.7)
find_package(PythonInterp REQUIRED)
find_package(PythonSix 1.5.2 REQUIRED)
find_package(PythonNumpy 1.7.0 REQUIRED)

# CMake doesn't have a VERSION_GREATER_EQUAL function, at least as of 3.0,
# And mako 1.0.2 contains bug fixes required for python 3.5 to work, so
# the only solution (short of having a series of "OR" statements, is this)
if (PYTHON_VERSION_STRING VERSION_GREATER 3.4.999999)
	find_package(PythonMako 1.0.2 REQUIRED)
else ()
	find_package(PythonMako 0.8.0 REQUIRED)
endif (PYTHON_VERSION_STRING VERSION_GREATER 3.4.999999)

find_package(bash-completion NO_MODULE)

# Default to compiling with debug information (`gcc -g`):
if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Debug CACHE STRING
	"May be one of: None Debug RelWithDebInfo Release MinSizeRel" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if (NOT MSVC)
	CHECK_C_COMPILER_FLAG("-Wall" C_COMPILER_FLAG_WALL)
	IF (C_COMPILER_FLAG_WALL)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
	ENDIF (C_COMPILER_FLAG_WALL)
	CHECK_CXX_COMPILER_FLAG("-Wall" CXX_COMPILER_FLAG_WALL)
	IF (CXX_COMPILER_FLAG_WALL)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	ENDIF (CXX_COMPILER_FLAG_WALL)

	# Target C99.  GCC's default is gnu11 for 5.0 and newer, gnu89 for
	# older versions.
	check_c_compiler_flag ("-std=gnu99" C_COMPILER_FLAG_STD_GNU99)
	if (C_COMPILER_FLAG_STD_GNU99)
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
	endif ()
	# MSVC does not support C99 variable length arrays
	CHECK_C_COMPILER_FLAG("-Werror=vla" C_COMPILER_FLAG_WEVLA)
	IF (C_COMPILER_FLAG_WEVLA)
		SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=vla")
	ENDIF ()
	# GCC allows void pointer arithmetic, but it is not part of ISO C and
	# in particular MSVC will throw `C2036: 'void *' : unknown size`
	check_c_compiler_flag ("-Werror=pointer-arith" C_COMPILER_FLAG_WEPOINTER_ARITH)
	if (C_COMPILER_FLAG_WEPOINTER_ARITH)
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=pointer-arith")
	endif ()
	# MSVC only supports C99 variadic macros.  It doesn't support the
	# non-standard GNU named variadic macro syntax that's documented in
	# https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
	#
	# XXX: on older GCC version this option has no effect unless -Wpedantic
	# is set, but this should be fixed on future GCC versions, per
	# https://gcc.gnu.org/ml/gcc-patches/2014-04/msg01459.html
	check_c_compiler_flag ("-Werror=variadic-macros" C_COMPILER_FLAG_WVARIADIC_MACROS)
	if (C_COMPILER_FLAG_WVARIADIC_MACROS)
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=variadic-macros")
	endif ()

	CHECK_CXX_COMPILER_FLAG("-Wno-narrowing" CXX_COMPILER_FLAG_WNO_NARROWING)
	IF (CXX_COMPILER_FLAG_WNO_NARROWING)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")
	ENDIF (CXX_COMPILER_FLAG_WNO_NARROWING)
else ()
	# Adjust warnings
	add_definitions (-W3)
	add_definitions (-wd4018) # signed/unsigned mismatch
	add_definitions (-wd4244) # conversion from 'type1' to 'type2', possible loss of data
	add_definitions (-wd4305) # truncation from 'type1' to 'type2'
	add_definitions (-wd4800) # forcing value to bool 'true' or 'false' (performance warning)

	add_definitions (-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
	add_definitions (-D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif ()

if (MINGW)
	# Match MSVC default stack size
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,1048576")

	# Avoid depending on MinGW runtime DLLs
	check_cxx_compiler_flag (-static-libgcc HAVE_STATIC_LIBGCC_FLAG)
	if (HAVE_STATIC_LIBGCC_FLAG)
		set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
		set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc")
		set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libgcc")
	endif ()
	check_cxx_compiler_flag (-static-libstdc++ HAVE_STATIC_LIBSTDCXX_FLAG)
	if (HAVE_STATIC_LIBSTDCXX_FLAG)
		set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
		set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++")
		set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libstdc++")
	endif ()
endif ()

if (${CMAKE_C_COMPILER_ID} STREQUAL "SunPro")
	# Use C++ to link C files.
	# http://developers.sun.com/solaris/articles/mixing.html#linking
	# Modified rule from Modules/CMakeCInformation.cmake.
	set (CMAKE_C_LINK_EXECUTABLE
		"<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
endif()

# Always enable GNU C extensions.  Non-GNU platforms will need to
# define wrappers for them.
add_definitions(-D_GNU_SOURCE)

if (WIN32)
	# MSVC & MinGW only define & use APIENTRY
	add_definitions (-DGLAPIENTRY=__stdcall)

	# Avoid namespace pollution when including windows.h
	# http://support.microsoft.com/kb/166474
	add_definitions (-DWIN32_LEAN_AND_MEAN=1)

	# Don't define min/max macros
	add_definitions (-DNOMINMAX)

	# Define M_PI and others
	add_definitions (-D_USE_MATH_DEFINES)
endif (WIN32)

if (APPLE)
	# Don't warn about using deprecated OpenGL/GLUT functions.
	# TODO: It would be nice to silence just the deprecation macros from
	# OpenGLAvailability.h as opposed to all deprecated functions.
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif ()

if (OPENGL_FOUND)
	if (APPLE)
		find_path(GLEXT_INCLUDE_DIR
			NAMES OpenGL/glext.h
			PATHS ${OPENGL_INCLUDE_DIR}
			DOC "Include for OpenGL/glext.h on OSX"
		)
	else (APPLE)
		find_path(GLEXT_INCLUDE_DIR
			NAMES GL/glext.h
			PATHS ${OPENGL_INCLUDE_DIR}
			DOC "Include for GL/glext.h"
		)
	endif (APPLE)
endif()

if(CMAKE_USE_PTHREADS_INIT)
	set(PIGLIT_HAS_PTHREADS true)
	add_definitions(-DPIGLIT_HAS_PTHREADS)
endif()

FIND_LIBRARY(HAVE_LIBRT NAMES rt)
if(HAVE_LIBRT)
	set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} rt)
endif()

check_c_source_compiles(
	"
	#define _POSIX_C_SOURCE 200112L
	#include <time.h>
	int main() { return clock_gettime(CLOCK_MONOTONIC, NULL); }
	"
	PIGLIT_HAS_POSIX_CLOCK_MONOTONIC
)

if(PIGLIT_HAS_PTHREADS AND PIGLIT_HAS_POSIX_CLOCK_MONOTONIC)
	check_c_source_compiles(
		"
		#include <signal.h>
		#include <time.h>
		static void timeout(union sigval val) { }
		int main() {
			struct sigevent sev = {
				.sigev_notify = SIGEV_THREAD,
				.sigev_notify_function = timeout,
			};
			timer_t timerid;
			return timer_create(CLOCK_MONOTONIC, &sev, &timerid);
		}
		"
		PIGLIT_HAS_POSIX_TIMER_NOTIFY_THREAD
	)
endif()

set(CMAKE_REQUIRED_LIBRARIES)

if(PIGLIT_HAS_POSIX_CLOCK_MONOTONIC)
	add_definitions(-DPIGLIT_HAS_POSIX_CLOCK_MONOTONIC)
endif()

if(PIGLIT_HAS_POSIX_TIMER_NOTIFY_THREAD)
	add_definitions(-DPIGLIT_HAS_POSIX_TIMER_NOTIFY_THREAD)
endif()

if(GBM_FOUND)
find_package(Libcaca)
if(LIBCACA_FOUND)
	add_definitions(-DPIGLIT_HAS_LIBCACA)
endif(LIBCACA_FOUND)
endif(GBM_FOUND)

if(PIGLIT_USE_WAFFLE AND (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD"))
	pkg_check_modules(EGL egl>=11.0)
endif()

if(EGL_FOUND)
	add_definitions(-DPIGLIT_HAS_EGL)
	include_directories(${EGL_INCLUDE_DIRS})
	add_definitions (${EGL_CFLAGS_OTHER})
endif()

if(PIGLIT_BUILD_GLES1_TESTS AND NOT EGL_FOUND)
	message(FATAL_ERROR "Option PIGLIT_BUILD_GLES1_TESTS requires EGL. "
			    "Failed to find EGL library.")
endif()

if(PIGLIT_BUILD_GLES2_TESTS AND NOT EGL_FOUND)
	message(FATAL_ERROR "Option PIGLIT_BUILD_GLES2_TESTS requires EGL. "
			    "Failed to find EGL library.")
endif()

if(PIGLIT_BUILD_GLES3_TESTS AND NOT EGL_FOUND)
	message(FATAL_ERROR "Option PIGLIT_BUILD_GLES3_TESTS requires EGL. "
			    "Failed to find EGL library.")
endif()

# Put all executables into the bin subdirectory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${piglit_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${piglit_BINARY_DIR}/lib)

# Do the same for MSVC, regardless of the build type. This only works correctly
# for CMake 2.8.1 and above.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${piglit_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${piglit_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${piglit_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${piglit_BINARY_DIR}/bin)

check_function_exists(asprintf  HAVE_ASPRINTF)
check_function_exists(ffs       HAVE_FFS)
check_function_exists(strchrnul HAVE_STRCHRNUL)
check_function_exists(strndup   HAVE_STRNDUP)
if(NOT MINGW)
check_function_exists(fopen_s   HAVE_FOPEN_S)
endif()
check_function_exists(setrlimit HAVE_SETRLIMIT)

check_include_file(sys/time.h  HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/resource.h  HAVE_SYS_RESOURCE_H)
check_include_file(sys/stat.h  HAVE_SYS_STAT_H)
check_include_file(unistd.h    HAVE_UNISTD_H)
check_include_file(fcntl.h     HAVE_FCNTL_H)
check_include_file(linux/sync_file.h HAVE_LINUX_SYNC_FILE_H)

if(DEFINED PIGLIT_INSTALL_VERSION)
	set(PIGLIT_INSTALL_VERSION_SUFFIX
	    "-${PIGLIT_INSTALL_VERSION}")
else()
	set(PIGLIT_INSTALL_VERSION_SUFFIX "")
endif()
set(PIGLIT_INSTALL_LIBDIR
    "${CMAKE_INSTALL_LIBDIR}/piglit${PIGLIT_INSTALL_VERSION_SUFFIX}")
if(NOT IS_ABSOLUTE ${PIGLIT_INSTALL_LIBDIR})
	set(PIGLIT_INSTALL_FULL_LIBDIR "${CMAKE_INSTALL_PREFIX}/${PIGLIT_INSTALL_LIBDIR}")
else()
	set(PIGLIT_INSTALL_FULL_LIBDIR "${PIGLIT_INSTALL_LIBDIR}")
endif()

SET(CMAKE_INSTALL_RPATH "${PIGLIT_INSTALL_FULL_LIBDIR}/lib")

configure_file(
	"${piglit_SOURCE_DIR}/tests/util/config.h.in"
	"${piglit_BINARY_DIR}/tests/util/config.h"
)

include(cmake/piglit_util.cmake)
include(cmake/piglit_dispatch.cmake)

include_directories(src)
add_subdirectory(cmake/target_api)
add_subdirectory(generated_tests)


##############################################################################
# Packaging

install (
	FILES
		COPYING
		README.md
		RELEASE
	DESTINATION share/doc/piglit${PIGLIT_INSTALL_VERSION_SUFFIX}
)

install (
	DIRECTORY framework
	DESTINATION ${PIGLIT_INSTALL_LIBDIR}
	FILES_MATCHING PATTERN "*.py"
)

install (
	DIRECTORY templates
	DESTINATION ${PIGLIT_INSTALL_LIBDIR}
)

install (
	DIRECTORY tests
	DESTINATION ${PIGLIT_INSTALL_LIBDIR}
	FILES_MATCHING REGEX ".*\\.(xml|xml.gz|py|program_test|shader_test|shader_source|frag|vert|geom|tesc|tese|comp|ktx|cl|txt|inc)$"
	REGEX "CMakeFiles|CMakeLists|serializer.py|opengl.py|cl.py|quick_gl.py|glslparser.py|shader.py|quick_shader.py|no_error.py|llvmpipe_gl.py|sanity.py" EXCLUDE
)

install (
	DIRECTORY ${CMAKE_BINARY_DIR}/tests
	DESTINATION ${PIGLIT_INSTALL_LIBDIR}
	FILES_MATCHING REGEX ".*\\.xml.gz"
)

install (
	DIRECTORY ${CMAKE_BINARY_DIR}/generated_tests
	DESTINATION ${PIGLIT_INSTALL_LIBDIR}
	FILES_MATCHING REGEX ".*\\.(shader_test|program_test|frag|vert|geom|tesc|tese|comp|cl|txt)$"
	REGEX "CMakeFiles|CMakeLists" EXCLUDE
)

install (
	DIRECTORY generated_tests
	DESTINATION ${PIGLIT_INSTALL_LIBDIR}
	FILES_MATCHING REGEX ".*\\.inc$"
	REGEX "CMakeFiles|CMakeLists" EXCLUDE
)

if (BASH_COMPLETION_FOUND)
	install(
		FILES completions/bash/piglit
		DESTINATION ${CMAKE_INSTALL_PREFIX}/${BASH_COMPLETION_COMPLETIONSDIR}/
	)
endif (BASH_COMPLETION_FOUND)

if (WIN32)
	set (PYTHON_SUFFIX ".py")
else ()
	set (PYTHON_SUFFIX "")
endif ()
install (
	PROGRAMS piglit RENAME piglit${PIGLIT_INSTALL_VERSION_SUFFIX}${PYTHON_SUFFIX}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
)


set (CPACK_PACKAGE_VERSION_MAJOR "1")
set (CPACK_PACKAGE_VERSION_MINOR "0")

# Use current date in YYYYMMDD format as patch number
execute_process (
	COMMAND ${PYTHON_EXECUTABLE} -c "import time, sys; sys.stdout.write(time.strftime('%Y%m%d'))"
	OUTPUT_VARIABLE CPACK_PACKAGE_VERSION_PATCH
)

# cpack mistakenly detects Mingw-w64 as win32
if (MINGW)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set (CPACK_SYSTEM_NAME win64)
	endif ()
endif ()

# See http://www.vtk.org/Wiki/CMake:CPackPackageGenerators
if (WIN32)
	set (CPACK_GENERATOR "ZIP")
else ()
	set (CPACK_GENERATOR "TBZ2")
endif ()

include(CPack)
