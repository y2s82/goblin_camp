#ifndef CONTAINER_HEADER
#define CONTAINER_HEADER

#include <boost/weak_ptr.hpp>
#include <set>

#include "Item.hpp"

class Container : public Item {
    private:
        std::set<boost::weak_ptr<Item> > items;
        int capacity;
        int reservedSpace;
	public:
        Container(Coordinate, int type, int cap=1000, int faction = 0);
		virtual ~Container();
		virtual bool AddItem(boost::weak_ptr<Item>);
		virtual void RemoveItem(boost::weak_ptr<Item>);
		void ReserveSpace(bool);
		boost::weak_ptr<Item> GetItem(boost::weak_ptr<Item>);
		std::set<boost::weak_ptr<Item> >* GetItems();
		boost::weak_ptr<Item> GetFirstItem();
        bool empty();
        int size();
        int Capacity();
        bool Full();
        std::set<boost::weak_ptr<Item> >::iterator begin();
        std::set<boost::weak_ptr<Item> >::iterator end();
};

#endif
