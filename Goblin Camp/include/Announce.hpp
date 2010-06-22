#ifndef ANNOUNCMENTS_HEADER
#define ANNOUNCMENTS_HEADER

#include <queue>
#include <string>
#include <sstream>
#include <libtcod.hpp>

#include "Coordinate.hpp"

#define ANNOUNCE_MAX_LENGTH 50

class AnnounceMessage {
	public:
		AnnounceMessage(std::string, TCODColor = TCODColor::white, Coordinate = Coordinate(-1,-1));
		std::stringstream result;
		std::string msg;
		int counter;
		std::string ToString();
		TCODColor color;
		Coordinate target;
};

class Announce {
	private:
		Announce();
		static Announce* instance;
		std::deque<AnnounceMessage*> messageQueue;
		std::deque<AnnounceMessage*> history;
		int timer;
	public:
		static Announce* Inst();
		void AddMsg(std::string, TCODColor = TCODColor::white);
		void Update();
		void Draw(unsigned int height, TCODConsole*);
		void Draw(Coordinate, int from, int amount, TCODConsole*);
		int AnnounceAmount();
		void EmptyMessageQueue();
		Coordinate CurrentCoordinate();
};

#endif
