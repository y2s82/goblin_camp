#include <libtcod.hpp>
#include <boost/multi_array.hpp>
#include <queue>

#include "Graphics.hpp"

Graphics::Graphics() {}

Graphics* Graphics::instance = 0;

Graphics* Graphics::Inst() {
	if (!instance) instance = new Graphics();
	return instance;
}

void Graphics::DrawDebug(std::string message) {
	TCODConsole::root->setBackgroundColor(TCODColor::blue);
	TCODConsole::root->setForegroundColor(TCODColor::red);

	TCODConsole::root->print(2, 2, message.c_str());
}
