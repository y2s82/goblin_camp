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
#include <tut.h>
#include <tut_reporter.h>
#include <tut_macros.h>

#include <sstream>
#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>
#include <cstring>

#include "uSerialize.hpp"
using namespace Serializer;

// Unit tests for uSerialize
namespace tut {
	struct TestData {
		std::stringstream ss;
	};
	
	typedef test_group<TestData> TestGroup;
	TestGroup testGroup("libuSerialize tests");
	
	typedef TestGroup::object Test;
	
	#define TEST(Id, Na, Co) template <> template <> void Test::test<Id>() { set_test_name(Na); do Co while(0); }
	#define INT_EQUALS_TEST(Id, Ty, V) TEST(Id, #Ty, { WriteSInt<Ty>(ss, V); ensure_equals(ss.str().size(), sizeof(Ty)); ensure_equals(ReadSInt<Ty>(ss), V); })
	
	//
	// Integer tests
	//
	INT_EQUALS_TEST(1, boost::uint8_t,  42)
	INT_EQUALS_TEST(2, boost::uint16_t, 60210)
	INT_EQUALS_TEST(3, boost::uint32_t, 84240732)
	INT_EQUALS_TEST(4, boost::uint64_t, 9271048492103)
	INT_EQUALS_TEST(5, boost::int8_t,   -42)
	INT_EQUALS_TEST(6, boost::int16_t,  -23021)
	INT_EQUALS_TEST(7, boost::int32_t,  -203203402)
	INT_EQUALS_TEST(8, boost::int64_t,  -668920345920)
	
	//
	// String tests
	//
	TEST(9, "std::string", {
		std::string x = "foo bar baz";
		WriteString<char>(ss, x);
		ensure_equals(ss.str().size(), 16);
		std::string y = ReadString<char>(ss);
		ensure_equals(x, y);
	})
	
	TEST(10, "std::wstring", {
		std::wstring x = L"foo bar baz";
		WriteString<wchar_t>(ss, x);
		ensure_equals(ss.str().size(), 5 + (11 * sizeof(wchar_t)));
		std::wstring y = ReadString<wchar_t>(ss);
		ensure("equal", x == y);
	})
	
	TEST(11, "std::string encoded, std::wstring decoded", {
		WriteString<char>(ss, "foo bar baz");
		ensure_THROW(ReadString<wchar_t>(ss), std::runtime_error);
	})
	
	TEST(12, "std::wstring encoded, std::string decoded", {
		WriteString<wchar_t>(ss, L"foo bar baz");
		ensure_THROW(ReadString<char>(ss), std::runtime_error);
	})
}

// Runner
namespace tut {
	test_runner_singleton runner;
}

int main() {
	tut::reporter reporter;
	tut::runner.get().set_callback(&reporter);
	tut::runner.get().run_tests();
	return !reporter.all_ok();
}
