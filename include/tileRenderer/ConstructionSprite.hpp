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

#include "tileRenderer/Sprite.hpp"
#include "Construction.hpp"
#include "ConstructionVisitor.hpp"
#include <boost/shared_ptr.hpp>

class ConstructionSprite
{
public:
	explicit ConstructionSprite();
	~ConstructionSprite();

	void AddSprite(Sprite_ptr sprite);
	void AddUnderConstructionSprite(Sprite_ptr sprite);
	void AddUnreadyTrapSprite(Sprite_ptr sprite);
	void SetWidth(int width);
	void SetOpenSprite(Sprite_ptr sprite);

	bool IsValid() const;
	bool HasUnderConstructionSprites() const;
	bool IsConnectionMap() const;
		
	// Normal draw
	void Draw(int screenX, int screenY, const Coordinate& internalPos) const;
	void DrawUnderConstruction(int screenX, int screenY, const Coordinate& internalPos) const;
	void DrawUnreadyTrap(int screenX, int screenY, const Coordinate& internalPos) const;
	void DrawOpen(int screenX, int screenY, const Coordinate& internalPos) const;

	// Connection map draw
	void Draw(int screenX, int screenY, Sprite::ConnectedFunction) const;
	void DrawUnderConstruction(int screenX, int screenY, Sprite::ConnectedFunction) const;
	void DrawUnreadyTrap(int screenX, int screenY, Sprite::ConnectedFunction) const;
	void DrawOpen(int screenX, int screenY, Sprite::ConnectedFunction) const;
private:	
	std::vector<Sprite_ptr> sprites;
	std::vector<Sprite_ptr> underconstructionSprites;
	std::vector<Sprite_ptr> unreadyTrapSprites;
	Sprite_ptr openSprite;
	int width;
};
