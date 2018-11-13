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

#include "tileRenderer/ogl/OGLViewportLayer.hpp"

ViewportLayer::ViewportLayer()
: data(), width(0), height(0)
{}

ViewportLayer::ViewportLayer(int width, int height)
:
	data(width * height * 4, 0),
	width(width),
	height(height)
{}

void ViewportLayer::Reset() {
	for (int i = 3; i < 4 * width * height; i += 4) {
		data[i] = 0;
	}
}

void ViewportLayer::SetTile(int x, int y, unsigned int tile) {
	int index = 4 * (x + y * width);
	data[index] = tile & 0xff;
	data[index + 1] = (tile >> 8) & 0xff;
	data[index + 2] = (tile >> 16) & 0xff;
	data[index + 3] = 0xff;
}

unsigned char * ViewportLayer::operator*() {
	return &data[0];
}

bool ViewportLayer::IsTileSet(int x, int y) const {
	return data[4 * (x + y * width) + 3] != 0;
}

int ViewportLayer::GetTile(int x, int y) const {
	int index = 4 * (x + y * width);
	return data[index] | (data[index + 1] << 8) | (data[index + 2] << 16);
}
