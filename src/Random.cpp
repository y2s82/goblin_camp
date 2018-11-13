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
#include "stdafx.hpp"

#include <boost/random.hpp>
#include <ctime>
#include <cmath>
#include <algorithm>

#include "Random.hpp"
#include "Logger.hpp"

namespace Globals {
	/**
		The global pseudo-random number generator.
		Seeded in \ref Random::Init.
	*/
	Random::Generator generator;
}

namespace {
	/**
		Common code for \ref Random::Generator functions.
		
		\tparam    G            Generator implementation type.
		\tparam    D            Distribution implementation type.
		\param[in] generator    Generator implementation.
		\param[in] distribution Distribution implementation.
		\returns                Random number.
	*/
	template <typename G, typename D>
	inline typename D::result_type InternalGenerate(G& generator, D distribution) {
		return boost::variate_generator<G&, D>(generator, distribution)();
	}
	
	/**
		Returns standard time-based seed.
		
		\returns Seed value.
	*/
	unsigned int GetStandardSeed() {
		return static_cast<unsigned int>(time(NULL));
	}
}

/**
	A simplified interface to Boost.Random.
*/
namespace Random {
	/**
		\class Dice
			Represents a single or multiple dice roll.
	*/
	/**
		Creates the dice of format <tt>\<multiplier\> * \<dices\>d\<faces\> + \<offset\></tt>.
		
		\param[in] faces      Dice faces.
		\param[in] dices      Number of dice rolls.
		\param[in] multiplier Result modifier.
		\param[in] offset     Result modifier.
	*/
	Dice::Dice(unsigned int faces, unsigned int dices, float multiplier, float offset) :
		dices(std::max(1U, dices)), faces(std::max(1U, faces)), multiplier(multiplier), offset(offset)
	{
	}
	
	/**
		Creates the dice out of TCOD_dice_t.
		
		\param[in] dice TCOD_dice_t to convert.
	*/
	Dice::Dice(const TCOD_dice_t& dice) :
		dices(std::max(1U, (unsigned)dice.nb_rolls)), faces(std::max(1U, (unsigned)dice.nb_faces)),
		multiplier(dice.multiplier), offset(dice.addsub)
	{
	}
	
	/**
		Rolls the dice.
		
		\returns Result of the roll(s).
	*/
	int Dice::Roll() {
		int result = 0;
		
		for (unsigned int i = 0; i < dices; ++i) {
			result += Generate(1U, std::max(1U, faces));
		}
		
		// round(x) = floor(x + 0.5)
		result = static_cast<int>(floor(
			((multiplier * result) + offset) + 0.5
		));
		return result;
	}
	
	/**
		Calculates maximum value of the roll.
		
		\returns Highest value that \ref Dice::Roll may return for this dice.
	*/
	int Dice::Max() {
		return static_cast<int>((multiplier * (dices * faces)) + offset);
	}
	
	/**
		Calculates minimum value of the roll.
		
		\returns Lowest value that \ref Dice::Roll may return for this dice.
	*/
	int Dice::Min() {
		return static_cast<int>((multiplier * (dices * 1)) + offset);
	}
	
	/**
		\class Generator
			An interface to seed and use Mersenne Twister-based PNRG.
			
			\see Globals::generator
	*/
	
	/**
		Creates and optionally seeds generator.
		
		\see Generator::Seed
		\param[in] seed Seed to use.
	*/
	Generator::Generator(unsigned int seed) : generator(GeneratorImpl()), seed(0) {
		SetSeed(seed);
	}
	
	/**
		(Re-)seeds the generator.
		
		\param[in] seed New seed to use. If 0, then reuses seed from the global generator.
	*/
	void Generator::SetSeed(unsigned int seed) {
		if (seed == 0) {
			seed = Globals::generator.seed;
		}
		this->seed = seed;
		generator.seed(this->seed);
	}
	
	/**
		Returns the seed used in this generator.
		
		\returns The seed value.
	*/
	unsigned int Generator::GetSeed() const {
		return seed;
	}
	
	/**
		Generates a random integer from range [start, end] using uniform distribution.
		
		\param[in] start The start of the range.
		\param[in] end   The end of the range.
		\returns         A random number from specified range.
	*/
	int Generator::Generate(int start, int end) {
		return InternalGenerate(generator, boost::uniform_int<>(start, end));
	}
	
	/**
		Generates a random integer from range [0, end] using uniform distribution.
		
		\param[in] end The end of the range.
		\returns       A random number from specified range.
	*/
	int Generator::Generate(int end) {
		return Generate(0, end);
	}
	
	/**
		Generates a random float from range [0, 1] using uniform distribution.
		
		\returns A random number from range [0, 1].
	*/
	double Generator::Generate() {
		return InternalGenerate(generator, boost::uniform_01<>());
	}
	
	/**
		Generates a random boolean.
		
		\returns A random boolean.
	*/
	bool Generator::GenerateBool() {
		return !!Generate(0, 1);
	}
	
	/**
		Generates a random sign. XXX come up with a better name.
		
		\returns Either 1 or -1.
	*/
	short Generator::Sign() {
		return GenerateBool() ? 1 : -1;
	}

	/**
		Generates a random coordinate inside the rectangular area
		delimited by (origin, extent), using uniform distribution
		(origin+extent excluded).
		
		When given only one parameter, it is assumed that the origin is zero.
	*/
	Coordinate Generator::ChooseInExtent(const Coordinate& origin, const Coordinate& extent) {
		Coordinate res = origin;
		for (int d = 0; d < 2; ++d)
			res[d] += Generator::Generate(extent[d]-1);
		return res;
	}

	/** \copydoc Generator::ChooseInExtent */
	Coordinate Generator::ChooseInExtent(const Coordinate& extent) {
		return Generator::ChooseInExtent(zero, extent);
	}

	/**
		Generates a random coordinate inside a rectangle of radius
		R around the origin, that is, with each coordinate within +R
		or -R of the origin. May return negative coordinates.

		If no origin is given, assumes zero. This is typically used this way:
		   position += ChooseInRadius(r);
	*/
	Coordinate Generator::ChooseInRadius(const Coordinate& origin, int radius) {
		Coordinate res = origin;
		for (int d = 0; d < 2; ++d)
			res[d] += Generator::Generate(-radius, radius);
		return res;
	}
	Coordinate Generator::ChooseInRadius(int radius) {
		return Generator::ChooseInRadius(zero, radius);
	}

	/**
	   Generates a random coordinate inside a rectangle delimited by
	   its low and high corners, both included.
	 */
	Coordinate Generator::ChooseInRectangle(const Coordinate& low, const Coordinate& high) {
		Coordinate res;
		for (int d = 0; d < 2; ++d)
			res[d] = Generator::Generate(low[d], high[d]);
		return res;
	}

	/**
		Initialises the PRNG.
	*/
	void Init() {
		unsigned int seed = GetStandardSeed();
		LOG("Seeding global random generator with " << seed);
		Globals::generator.SetSeed(seed);
	}
	
	/** \copydoc Generator::Generate(int, int) */
	int Generate(int start, int end) {
		return Globals::generator.Generate(start, end);
	}
	
	/** \copydoc Generator::Generate(int) */
	int Generate(int end) {
		return Globals::generator.Generate(end);
	}
	
	/** \copydoc Generator::Generate() */
	double Generate() {
		return Globals::generator.Generate();
	}
	
	/** \copydoc Generator::GenerateBool */
	bool GenerateBool() {
		return Globals::generator.GenerateBool();
	}
	
	/** \copydoc Generator::Sign */
	short Sign() {
		return Globals::generator.Sign();
	}

	/** \copydoc Generator::ChooseInExtent */
	Coordinate ChooseInExtent(const Coordinate& zero, const Coordinate& extent) {
		return Globals::generator.ChooseInExtent(zero, extent);
	}
	Coordinate ChooseInExtent(const Coordinate& extent) {
		return Globals::generator.ChooseInExtent(extent);
	}
	/** \copydoc Generator::ChooseInRadius */
	Coordinate ChooseInRadius(const Coordinate& origin, int radius) {
		return Globals::generator.ChooseInRadius(origin, radius);
	}
	Coordinate ChooseInRadius(int radius) {
		return Globals::generator.ChooseInRadius(radius);
	}
	/** \copydoc Generator::ChooseInRectangle */
	Coordinate ChooseInRectangle(const Coordinate& low, const Coordinate& high) {
		return Globals::generator.ChooseInRectangle(low, high);
	}

	/**
		\fn T Sign(const T&)
			Multiplies the expression by a random sign. XXX come up with a better name.
			
			\see Generator::Sign
			\tparam    T    The expression type.
			\param[in] expr The expression value.
			\returns        The expression value multiplied by either 1 or -1.
	*/
	
	/**
		\fn unsigned ChooseIndex(const T&)
			Chooses a random index from a given STL container.
			
			\tparam T            The container type (must implement the <tt>Random Access Container</tt> concept).
			\param[in] container The container.
			\returns             The index of a random element.
	*/
	
	/**
		\fn unsigned ChooseElement(const T&)
			Chooses a random element from a given STL container.
			
			\tparam T            The container type (must implement the <tt>Random Access Container</tt> concept).
			\param[in] container The container.
			\returns             A random element.
	*/
}
