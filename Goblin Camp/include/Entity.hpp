#ifndef Entity_HEADER
#define Entity_HEADER

#include <set>
#include <string>
#include <boost/enable_shared_from_this.hpp>

#include "Coordinate.hpp"

class Entity: public boost::enable_shared_from_this<Entity>
{
	protected:
		unsigned int _x, _y;
		int uid;
		int zone;
		bool reserved;
		std::string name;
		int faction;
	public:
		Entity();
		virtual ~Entity();
		virtual int x();
		virtual int y();
		int Uid();
		static int uids;
		virtual Coordinate Position();
		virtual void Position(Coordinate);
		void Zone(int);
		int Zone();
		virtual void Reserve(bool);
		bool Reserved();
		virtual std::string Name();
		virtual void CancelJob(int=0);

		virtual void Faction(int);
		virtual int Faction() const;
};

#endif
