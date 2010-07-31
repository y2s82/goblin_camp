/* Copyright 2010 Ilkka Halila
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

#include "UIComponents.hpp"

class Button: public Drawable {
private:
    std::string text;
    bool selected;
    char shortcut;
    boost::function<void()> callback;
public:
    Button(std::string ntext, boost::function<void()> ncallback, int x, int y, int nwidth, char nshortcut = 0):
    text(ntext), callback(ncallback), shortcut(nshortcut), Drawable(x, y, nwidth, 0), selected(false) {}
    void Draw(int, int, TCODConsole *);
    MenuResult Update(int, int, bool, TCOD_key_t);
};

