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

#include "tileRenderer/ogl/OGLResources.hpp"
#include "Logger.hpp"
#include <boost/scoped_array.hpp>

#include <SDL/SDL_opengl.h>
#include "tileRenderer/ogl/OGLFunctionExt.hpp"

using namespace OGLFunctionExtension;

namespace {
	struct TextureDeleter {
		TextureDeleter(GLuint tex) : handle(tex) {}

		void operator()( GLuint * dummyHandle )
		{
			glDeleteTextures(1, &handle);
        }

		GLuint handle;
	};

	struct ShaderDeleter {
		ShaderDeleter(GLuint shader) : handle(shader) {}

		void operator()( GLuint * dummyHandle )
		{
			glDeleteObjectARB(handle);
        }

		GLuint handle;
	};

	struct ProgramDeleter {
		ProgramDeleter(GLuint shader, boost::shared_ptr<const unsigned int> vertShader, boost::shared_ptr<const unsigned int> fragShader) 
			: handle(shader),
		      vertShader(vertShader),
		      fragShader(fragShader)
		{}

		void operator()( GLuint * dummyHandle )
		{
			glDeleteObjectARB(handle);
        }

		GLuint handle;
		boost::shared_ptr<const unsigned int> vertShader;
		boost::shared_ptr<const unsigned int> fragShader;
	};
}

boost::shared_ptr<const unsigned int> CreateOGLTexture() {
	GLuint handle = 0;
    glGenTextures(1, &handle);
    if( glGetError() ) {
		LOG("Failed to create OGL Texture");
		return boost::shared_ptr<const unsigned int>();    
	} else {
		boost::shared_ptr<unsigned int> innerPtr((unsigned int *) 0, TextureDeleter(handle));
		return boost::shared_ptr<const unsigned int>(innerPtr, &boost::get_deleter<TextureDeleter>(innerPtr)->handle);
	}
}

boost::shared_ptr<const unsigned int> CreateOGLShaderProgram(std::string vertShaderCode, std::string fragShaderCode) {
	boost::shared_ptr<const unsigned int> vertShader(CreateOGLShader(vertShaderCode, GL_VERTEX_SHADER));
	if (*vertShader == 0) {
		return boost::shared_ptr<const unsigned int>();
	}

	boost::shared_ptr<const unsigned int> fragShader(CreateOGLShader(fragShaderCode, GL_FRAGMENT_SHADER));
	if (*fragShader == 0) {
		return boost::shared_ptr<const unsigned int>();
	}

	GLuint programHandle = glCreateProgramObjectARB();
	if (glGetError()) {
		LOG("Failed to create OGL Program Object");
		return boost::shared_ptr<const unsigned int>();    
	}

	boost::shared_ptr<unsigned int> innerPtr((unsigned int *) 0, ProgramDeleter(programHandle, vertShader, fragShader));
	boost::shared_ptr<const unsigned int> program(boost::shared_ptr<const unsigned int>(innerPtr, &boost::get_deleter<ProgramDeleter>(innerPtr)->handle));
	glAttachObjectARB(*program, *vertShader);
	glAttachObjectARB(*program, *fragShader);
	glLinkProgramARB(*program);

	int success;
	glGetObjectParameterivARB(*program, GL_LINK_STATUS, &success);
	if(success != GL_TRUE) {
		/* something went wrong */
		int infologLength = 0;
		glGetObjectParameterivARB(*program, GL_INFO_LOG_LENGTH,&infologLength);
		if (infologLength > 0) {
			boost::scoped_array<char> infoLog(new char[infologLength]);

			int charsWritten = 0;
			glGetInfoLogARB(*program, infologLength, &charsWritten, infoLog.get());
			LOG("OPENGL ERROR: Program link Error. " << std::endl << /*infoLog <<*/ std::endl); // FIXME
	    }
		return boost::shared_ptr<const unsigned int>();
	}
	return program;
}

boost::shared_ptr<const unsigned int> CreateOGLShader(std::string shader, unsigned int type) {
	GLuint handle = glCreateShaderObjectARB(type);
	boost::shared_ptr<unsigned int> innerPtr((unsigned int *) 0, ShaderDeleter(handle));
	boost::shared_ptr<const unsigned int> shaderPtr(boost::shared_ptr<const unsigned int>(innerPtr, &boost::get_deleter<ShaderDeleter>(innerPtr)->handle));
	
	const char * shaderTxt = shader.c_str();
	glShaderSourceARB(*shaderPtr, 1, &shaderTxt, 0);
	glCompileShaderARB(*shaderPtr);

	int success = 0;
	glGetObjectParameterivARB(*shaderPtr, GL_COMPILE_STATUS, &success);
	if(success != GL_TRUE) {
	    /* something went wrong */
		int infologLength = 0;
		glGetObjectParameterivARB(*shaderPtr, GL_INFO_LOG_LENGTH,&infologLength);
		if(infologLength > 0) {
			boost::scoped_array<char> infoLog(new char[infologLength]);

			int charsWritten = 0;
			glGetInfoLogARB(*shaderPtr, infologLength, &charsWritten, infoLog.get());
			LOG("GLSL ERROR: " << /*infoLog << */ std::endl);  // FIXME
		}
		return boost::shared_ptr<const unsigned int>();
	}

	return shaderPtr;
}
