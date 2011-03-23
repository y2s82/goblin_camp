/* Copyright 2010-2011 Ilkka Halila
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

#include "MapRenderer.hpp"

class TCODMapRenderer : public MapRenderer
{
public:
	TCODMapRenderer(TCODConsole * mapConsole);
	~TCODMapRenderer();

	void PreparePrefabs();
	
	Coordinate TileAt(int screenX, int screenY, float focusX, float focusY, int viewportX, int viewportY, int viewportW , int viewportH) const;
	void DrawMap(Map* map, float focusX, float focusY, int viewportX, int viewportY, int viewportW, int viewportH) ;
	float ScrollRate() const;
	
	void SetCursorMode(CursorType mode);
	void SetCursorMode(const NPCPreset& preset);
	void SetCursorMode(const ItemPreset& preset);
	void SetCursorMode(int other);
	void DrawCursor(const Coordinate& pos, bool placeable);
	void DrawCursor(const Coordinate& start, const Coordinate& end, bool placeable);

	void SetTranslucentUI(bool);

private:
	TCODConsole * console;
	int cursorChar;
	Coordinate upleft;
};
