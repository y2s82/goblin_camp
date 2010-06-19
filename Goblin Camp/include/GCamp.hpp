#ifndef GCAMP_HEADER
#define GCAMP_HEADER

#include <boost/multi_array.hpp>
#include <boost/thread/thread.hpp>

#include "Tile.hpp"
#include "Coordinate.hpp"

#define UPDATES_PER_SECOND 25

void mainLoop();
int distance(int,int,int,int);
#endif
