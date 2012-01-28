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
#pragma once

#include <boost/noncopyable.hpp>
#include <SDL/SDL_opengl.h>

namespace OGLFunctionExtension {
	GLhandleARB glCreateShaderObjectARB(GLenum shaderType);
	void glGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint *params);
	void glShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
	void glCompileShaderARB(GLhandleARB shaderObj);
	void glGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
	GLhandleARB glCreateProgramObjectARB();
	void glAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj);
	void glLinkProgramARB(GLhandleARB programObj);
	void glUseProgramObjectARB(GLhandleARB programObj);
	void glUniform2fARB(GLint location, GLfloat v0, GLfloat v1);
	GLint glGetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name);
	void glUniform1fARB(GLint location, GLfloat v0);
	void glUniform1iARB(GLint location, GLint v0);
	void glDeleteObjectARB(GLhandleARB handle);

	// OpenGL 1.2 functions are extensions in windows
	#ifdef WINDOWS 
		void glActiveTexture(GLenum texture);
	#endif
}