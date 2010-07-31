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

enum MenuResult {
	MENUHIT,
	NOMENUHIT
};

class Drawable {
protected:
	int _x, _y, width, height;    
public:
    Drawable(int x, int y, int nwidth, int nheight):
    _x(x), _y(y), width(nwidth), height(nheight) {}
    virtual void Draw(int, int, TCODConsole *) = 0;
	virtual MenuResult Update(int x, int y, bool clicked, TCOD_key_t key) {return NOMENUHIT;}
};

class Scrollable {
public:
    virtual void Draw(int x, int y, int scroll, int width, int height, TCODConsole *) = 0;
    virtual int TotalHeight() = 0;
	virtual MenuResult Update(int x, int y, bool clicked, TCOD_key_t key) {return NOMENUHIT;}
};

class Label: public Drawable {
private:
    std::string text;
    TCOD_alignment_t align;
public:
    Label(std::string ntext, int x, int y, TCOD_alignment_t nalign = TCOD_CENTER) :
    Drawable(x, y, 0, 0), text(ntext), align(nalign) {}
    void Draw(int, int, TCODConsole *);
};

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

class Panel: public Drawable {
public:
    Panel(int nwidth, int nheight):
    Drawable(0, 0, nwidth, nheight) {}
    void ShowModal();
	virtual void Open();
	virtual void Close();
	virtual void selected(int);
};

class Dialog: public Panel {
private:
    std::vector<Drawable *> components;
    std::string title;
public:
    Dialog(std::vector<Drawable *>, std::string, int, int);
    ~Dialog();
    void AddComponent(Drawable *component);
    void Draw(int, int, TCODConsole *);
    MenuResult Update(int, int, bool, TCOD_key_t);
    void SetTitle(std::string ntitle);
    void SetHeight(int nheight);
};

class ScrollPanel: public Drawable {
private:
    int scroll, scrollBar;
    bool drawFrame;
    Scrollable *contents;
public:
    ScrollPanel(int x, int y, int nwidth, int nheight, Scrollable *ncontents, bool ndrawFrame = true):
    contents(ncontents), scroll(0), scrollBar(0), drawFrame(ndrawFrame), Drawable(x, y, nwidth, nheight) {}
    void Draw(int, int, TCODConsole *);
    MenuResult Update(int, int, bool, TCOD_key_t);
};


template <class T, class C = std::vector<T> >
class UIList: public Drawable, public Scrollable {
private:
    C* items;
    bool selectable;
    int selection;
    boost::function<void(T, int, int, int, bool, TCODConsole *)> draw;
    boost::function<void(int)> onclick;
public:
    UIList<T, C>(C *nitems, int x, int y, int nwidth, int nheight, boost::function<void(T, int, int, int, bool, TCODConsole *)> ndraw, boost::function<void(int)> nonclick = 0, bool nselectable = false):
        items(nitems), selectable(nselectable), selection(-1), draw(ndraw), onclick(nonclick), Drawable(x, y, nwidth, nheight) {}
    void Draw(int, int, TCODConsole *);
    void Draw(int x, int y, int scroll, int width, int height, TCODConsole *);
    int TotalHeight();
    MenuResult Update(int, int, bool, TCOD_key_t);
};

template <class T, class C>
void UIList<T, C>::Draw(int x, int y, TCODConsole *console) {
    console->setAlignment(TCOD_LEFT);
    int count = 0;
    for(typename C::iterator it = items->begin(); it != items->end(); it++) {
        T item = *it;
        draw(item, count, x + _x, y + _y + count, selection == count, console);
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
			draw(item, count, x, y + (count - scroll), selection == count, console);
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
    if (x >= _x && x < _x + width && y >= _y && y < _y + width) {
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