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

template <class T, class C = std::vector<T> >
class UIList: public Drawable, public Scrollable {
private:
	C* items;
	bool selectable;
	int selection;
	boost::function<void(T, int, int, int, int, bool, TCODConsole *)> draw;
	boost::function<void(T, Tooltip *)> getTooltip;
	boost::function<void(int)> onclick;
public:
	UIList<T, C>(
		C *nitems, int x, int y, int nwidth, int nheight,
		boost::function<void(T, int, int, int, int, bool, TCODConsole *)> ndraw,
		boost::function<void(int)> nonclick = 0, bool nselectable = false,
		boost::function<void(T, Tooltip *)> ntooltip = 0
	):
		Drawable(x, y, nwidth, nheight),
		items(nitems),
		selectable(nselectable),
		selection(-1),
		draw(ndraw),
		getTooltip(ntooltip),
		onclick(nonclick) {}
	void Draw(int, int, TCODConsole *);
	void Draw(int x, int y, int scroll, int width, int height, TCODConsole *);
	int TotalHeight();
	MenuResult Update(int, int, bool, TCOD_key_t);
	void GetTooltip(int, int, Tooltip *);
	int Selected();
	void Select(int);
};

template <class T, class C>
void UIList<T, C>::Draw(int x, int y, TCODConsole *console) {
	console->setAlignment(TCOD_LEFT);
	int count = 0;
	for(typename C::iterator it = items->begin(); it != items->end() && count < height; it++) {
		T item = *it;
		draw(item, count, x + _x, y + _y + count, width, selection == count, console);
		count++;
	}
}

template <class T, class C>
void UIList<T, C>::Draw(int x, int y, int scroll, int _width, int _height, TCODConsole *console) {
	console->setAlignment(TCOD_LEFT);
	int count = 0;
	for(typename C::iterator it = items->begin(); it != items->end(); it++) {
		T item = *it;
		if (count >= scroll && count < scroll + _height) {
			draw(item, count, x, y + (count - scroll), _width, selection == count, console);
		}
		count++;
	}
}

template <class T, class C>
int UIList<T, C>::TotalHeight() {
	return items->size();
}

template <class T, class C>
MenuResult UIList<T, C>::Update(int x, int y, bool clicked, TCOD_key_t key) {
	if (x >= _x && x < _x + width && y >= _y && y < _y + height) {
		if (clicked) {
			if (selectable) {
				selection = y - _y;
			}
			if (onclick) {
				onclick(y - _y);
			}
		}
		return MENUHIT;
	}
	return NOMENUHIT;
}

template <class T, class C>
void UIList<T, C>::GetTooltip(int x, int y, Tooltip *tooltip) {
	if(getTooltip) {
		if (x >= _x && x < _x + width && y >= _y && y < _y + width && y - _y < (signed int)items->size()) {
			typename C::iterator it = items->begin();
			for(int i = 0; i < (y - _y); i++) {
				it++;
			}
			getTooltip(*it, tooltip);
		}
	}
}

template <class T, class C>
int UIList<T, C>::Selected() {
	if(selection >= 0 && selection < (signed int)items->size()) {
		return selection;
	}
	return -1;
}

template <class T, class C>
void UIList<T, C>::Select(int i) {
	selection = i;
}
