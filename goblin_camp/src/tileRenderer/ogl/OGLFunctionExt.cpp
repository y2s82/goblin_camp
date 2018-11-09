/* Copyright 2011 Ilkka Halila
This file is part of Goblin Camp.

Goblin Camp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Goblin Camp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.*/
#include "stdafx.hpp"

#include <SDL.h>
#include <SDL_opengl.h>
#include "tileRenderer/ogl/OGLFunctionExt.hpp"

// XXX this code is not fully conformant — void* (as returned by SDL_GL_GetProcAddress)
// is not guaranteed to be able to hold a function pointer by C++.

#ifdef __GNUC__
#	if __GNUC_MINOR__ > 5
#		pragma GCC diagnostic push
#	endif
#	pragma GCC diagnostic ignored "-pedantic"
#endif

namespace OGLFunctionExtension {
	GLhandleARB glCreateShaderObjectARB(GLenum shaderType) {
		static PFNGLCREATESHADEROBJECTARBPROC function((PFNGLCREATESHADEROBJECTARBPROC)SDL_GL_GetProcAddress("glCreateShaderObjectARB"));
		return function(shaderType);
	}

	void glGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint *params) {
		static PFNGLGETOBJECTPARAMETERIVARBPROC function((PFNGLGETOBJECTPARAMETERIVARBPROC)SDL_GL_GetProcAddress("glGetObjectParameterivARB"));
		function(obj, pname, params);
	}

	void glShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length) {
		static PFNGLSHADERSOURCEARBPROC function((PFNGLSHADERSOURCEARBPROC)SDL_GL_GetProcAddress("glShaderSourceARB"));
		function(shaderObj, count, string, length);
	}

	void glCompileShaderARB(GLhandleARB shaderObj) {
		static PFNGLCOMPILESHADERARBPROC function((PFNGLCOMPILESHADERARBPROC)SDL_GL_GetProcAddress("glCompileShaderARB"));
		function(shaderObj);
	}

	void glGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog) {
		static PFNGLGETINFOLOGARBPROC function((PFNGLGETINFOLOGARBPROC)SDL_GL_GetProcAddress("glGetInfoLogARB"));
		function(obj, maxLength, length, infoLog);
	}

	GLhandleARB glCreateProgramObjectARB() {
		static PFNGLCREATEPROGRAMOBJECTARBPROC function((PFNGLCREATEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glCreateProgramObjectARB"));
		return function();
	}

	void glAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj) {
		static PFNGLATTACHOBJECTARBPROC function((PFNGLATTACHOBJECTARBPROC)SDL_GL_GetProcAddress("glAttachObjectARB"));
		function(containerObj, obj);
	}

	void glLinkProgramARB(GLhandleARB programObj) {
		static PFNGLLINKPROGRAMARBPROC function((PFNGLLINKPROGRAMARBPROC)SDL_GL_GetProcAddress("glLinkProgramARB"));
		function(programObj);
	}

	void glUseProgramObjectARB(GLhandleARB programObj) {
		static PFNGLUSEPROGRAMOBJECTARBPROC function((PFNGLUSEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glUseProgramObjectARB"));
		function(programObj);
	}

	void glUniform2fARB(GLint location, GLfloat v0, GLfloat v1) {
		static PFNGLUNIFORM2FARBPROC function((PFNGLUNIFORM2FARBPROC)SDL_GL_GetProcAddress("glUniform2fARB"));
		function(location, v0, v1);
	}

	GLint glGetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name) {
		static PFNGLGETUNIFORMLOCATIONARBPROC function((PFNGLGETUNIFORMLOCATIONARBPROC)SDL_GL_GetProcAddress("glGetUniformLocationARB"));
		return function(programObj, name);
	}

	void glUniform1fARB(GLint location, GLfloat v0) {
		static PFNGLUNIFORM1FARBPROC function((PFNGLUNIFORM1FARBPROC)SDL_GL_GetProcAddress("glUniform1fARB"));
		function(location, v0);
	}

	void glUniform1iARB(GLint location, GLint v0) {
		static PFNGLUNIFORM1IARBPROC function((PFNGLUNIFORM1IARBPROC)SDL_GL_GetProcAddress("glUniform1iARB"));
		function(location, v0);
	}

	void glDeleteObjectARB(GLhandleARB handle) {
		static PFNGLDELETEOBJECTARBPROC function((PFNGLDELETEOBJECTARBPROC)SDL_GL_GetProcAddress("glDeleteObjectARB"));
		function(handle);
	}

	#ifdef WINDOWS
		void glActiveTexture(GLenum texture) {
			static PFNGLACTIVETEXTUREPROC function((PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture"));
			function(texture);
		}
	#endif
}

#if defined(__GNUC__) && __GNUC_MINOR__ > 5
#	pragma GCC diagnostic pop
#endif
