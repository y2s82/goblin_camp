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

#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <libtcod.hpp>

#include "UI/Tooltip.hpp"

enum MenuResult {
	MENUHIT = 1,
	NOMENUHIT = 2,
	KEYRESPOND = 4,
	DISMISS = 8
};

class Drawable {
protected:
	int _x, _y, width, height;
	boost::function<bool()> visible;
	boost::function<void(int, int, Tooltip*)> getTooltip;
public:
	Drawable(int x, int y, int nwidth, int nheight):
	  _x(x), _y(y), width(nwidth), height(nheight), visible(0), getTooltip(0) {}
	virtual ~Drawable() {}

	virtual void Draw(int, int, TCODConsole *) = 0;
	virtual MenuResult Update(int x, int y, bool clicked, TCOD_key_t key)
	{return (x >= _x && x < _x + height && y >= _y && y < _y + height) ? MENUHIT : NOMENUHIT;}
	int Height() { return height; }
	bool Visible() { return !visible || visible(); }
	void SetVisible(boost::function<bool()> nvisible) { visible = nvisible; }
	virtual void GetTooltip(int x, int y, Tooltip *tooltip) { if(getTooltip) getTooltip(x, y, tooltip); }
	void SetTooltip(boost::function<void(int, int, Tooltip*)> ntooltip) { getTooltip = ntooltip; }
};

class Scrollable {
public:
	virtual ~Scrollable() {}
	virtual void Draw(int x, int y, int scroll, int width, int height, TCODConsole *) = 0;
	virtual int TotalHeight() = 0;
	virtual MenuResult Update(int x, int y, bool clicked, TCOD_key_t key) { return NOMENUHIT; }
	virtual void GetTooltip(int x, int y, Tooltip *tooltip) {};
};

class UIContainer: public Drawable {
protected:
	std::vector<Drawable *> components;
public:
	UIContainer(std::vector<Drawable *> ncomponents, int nx, int ny, int nwidth, int nheight):
	  Drawable(nx, ny, nwidth, nheight), components(ncomponents) {}
	~UIContainer();
	void AddComponent(Drawable *component);
	virtual void Draw(int, int, TCODConsole *);
	virtual MenuResult Update(int, int, bool, TCOD_key_t);
	virtual void GetTooltip(int, int, Tooltip *);
};

class Panel: public Drawable {
public:
	Panel(int nwidth, int nheight):
	  Drawable(0, 0, nwidth, nheight) {}
	void ShowModal();
	virtual void Open();
	virtual void Close();
	virtual void selected(int);
};
