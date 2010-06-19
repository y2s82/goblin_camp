#ifndef FILTH_HEADER
#define FILTH_HEADER

#include <boost/enable_shared_from_this.hpp>
#include <libtcod.hpp>

#include "Coordinate.hpp"

class FilthNode : public boost::enable_shared_from_this<FilthNode> {
    private:
        int x, y;
        int depth;
        int graphic;
        TCODColor color;
    public:
        FilthNode(int x=0, int y=0, int depth=0);
        ~FilthNode();
        void Update();
        void Draw(Coordinate);
        int Depth();
        void Depth(int);
        Coordinate Position();
};

#endif
