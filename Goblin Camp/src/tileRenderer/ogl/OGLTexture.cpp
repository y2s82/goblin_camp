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

#include "tileRenderer/ogl/OGLTexture.hpp"
#include "Logger.hpp"

#include <SDL/SDL_opengl.h>

namespace {
	struct TextureDeleter {
		TextureDeleter(GLuint tex) : handle(tex) {}

		void operator()( GLuint * dummyHandle )
		{
			glDeleteTextures(1, &handle);
        }

		GLuint handle;
	};
}

boost::shared_ptr<const unsigned int> CreateOGLTexture() {
	GLuint handle = 0;
    glGenTextures(1, &handle);
    if( GLenum err=glGetError() ) {
		LOG("Failed to create OGL Texture");
		return boost::shared_ptr<const unsigned int>();    
	} else {
		boost::shared_ptr<unsigned int> innerPtr((unsigned int *) 0, TextureDeleter(handle));
		return boost::shared_ptr<const unsigned int>(innerPtr, &boost::get_deleter<TextureDeleter>(innerPtr)->handle);
	}
}