// SPDX-License-Identifier: MIT-0
// SPDX-FileCopyrightText:  2024 Istvan Pasztor
//
// Parametric CRC-8 / CRC-16 / CRC-32 / CRC-64
// Single-header C++ library (-std=c++14)
//
// Use the mini version of this library (parametric_crc_mini.h) if all you need
// is fast table based CRC calculation. This large version comes with features
// (e.g. tableless calculation, small tables,...) that most projects don't need.
//
// Usage:
//
// Find the CRC algorithm you need in the crc8, crc16, crc32 or crc64
// namespace (at the bottom of this file) and use it like this:
//
//    uint16_t crc_val = crc16::xmodem::calculate("123456789", 9);
//    printf("CRC-16/XMODEM check=0x%04x\n", crc_val);
// OR
//    crc16::xmodem crc_obj;
//    crc_obj.update("12345", 5);
//    crc_obj.update("6789", 4);
//    printf("CRC-16/XMODEM check=0x%04x\n", crc_obj.final());
//
// Modes of operation:
//
// - table-driven:           (fast, works without table source code generation)
//   - table_based           (256 constexpr table entries provided by the library)
//   - small_table_based     (32  constexpr table entries provided by the library)
//   - ext_table_based       (256 external table entries provided by the user)
//   - ext_small_table_based (32  external table entries provided by the user)
// - tableless (slowest, bit-by-bit processing, requires no memory for a table)
//
// Description of the modes:
//
// - The table_based mode is a comfortable high-performance default. The library
//   takes care of all the table-related business behind the curtains by using
//   static constexpr tables generated at compile so its API is as simple as
//   that of the tableless mode.
// - The "ext_table_based" mode relies on an external table that you have to
//   provide as an extra parameter to all update() and calculate() calls.
//   This gives you manual control over the lifecycle and/or the memory location
//   of the table. The library provides tools to create tables easily.
// - Use a "small" lookup table (aka. "nibble lookup table") or the table-less
//   mode if you have little or no memory for tables. A small table takes up
//   much less space (1/8th of a normal table) but turns a single lookup into
//   two lookups and a XOR (still faster than processing the data bit-by-bit and
//   in case of CRC-8 it eliminates the shift operations from the calculation).
/*

void example() {
	// I picked crc16::kermit and crc32::pkzip at random for the below examples:

	// Modes: table_based, small_table_based, tableless
	// The update() and calculate() methods don't have a table parameter.
	{
		// You can change the mode by switching out the type below to
		// crc16::kermit::small_table_based or crc16::kermit::tableless.
		// The rest of the code works without modifications.
		// The table_based mode is the default so crc16::kermit is the
		// same as crc16::kermit::table_based.
		using crc_t = crc16::kermit::table_based;

		printf("1: crc=0x%04x\n", crc_t::calculate("123456789", 9));

		crc_t crc_obj;
		crc_obj.update("12345", 5);
		crc_obj.update('6');
		crc_obj.update("789", 3);
		printf("2: crc=0x%04x\n", crc_obj.final());
	}

	// Modes: ext_table_based, ext_small_table_based
	// The update() and calculate() methods have a required table parameter.
	{
		// You can change the mode by using crc32::pkzip::ext_small_table_based.
		// The rest of the code works without modifications.
		using crc_t = crc32::pkzip::ext_table_based;

		// You control the lifespan and the location of the table.
		// It can be constexpr.
		crc_t::table_type table;

		// The default constructor is safe and generates/fills the table.
		// You can leave it uninitialized by passing the crc::UNINITIALIZED
		// constant to its constructor. In that scenario you have to initialize
		// the table manually with a generate() call before using it.
		//crc_t::table_type table(crc::UNINITIALIZED);
		//table.generate();

		printf("ext 1: crc=0x%08x\n", crc_t::calculate("123456789", 9, table));

		crc_t crc_obj;
		crc_obj.update("12345", 5, table);
		crc_obj.update('6', table);
		crc_obj.update("789", 3, table);
		printf("ext 2: crc=0x%08x\n", crc_obj.final());
	}

	// Using a custom polynomial:
	{
		// 0xa2eb was picked from the CRC Polynomial Zoo:
		// https://users.ece.cmu.edu/~koopman/crc/crc16.html
		using zoo_a2eb = crc::parametric<16, 0xa2eb, 0xffff, 0xffff, true>;
		printf("CRC-16/ZOO-A2EB-FF-LSB: check=0x%04x residue=0x%04x\n",
			zoo_a2eb::tableless::calculate("123456789", 9), zoo_a2eb::RESIDUE);
	}
}

void example_2_lower_level_api() {
	// The high-level object-oriented API of this library sits on top of a
	// lower level API located in the crc::core<BIT_WIDTH,REFLECTED> template.
	// BIT_WIDTH has to be 8, 16, 32 or 64. REFLECTED=true means reflected CRC
	// shift register and tables. It's only a few functions, take a look at the
	// actual implementation if you want to use it.

	// 16-bit unreflected CRC register
	using core = crc::core<16, false>;

	// Unlike the high level API the core works with uint8* only.
	const uint8_t* data = (const uint8_t*)"123456789";
	const uint8_t* data_end = data + strlen((const char*)data);

	static constexpr uint16_t CCITT_POLY = 0x1021;

	// Calculation with CRC-16/XMODEM parameters:
	{
		uint16_t crc_register = 0;
		core::tableless_update(CCITT_POLY, crc_register, data, data_end);
		printf("core 1: 0x%04x\n", crc_register);

		uint16_t table[256];
		core::generate_table(CCITT_POLY, table);

		crc_register = 0;
		core::table_based_update(crc_register, data, data_end, table);
		printf("core 2: 0x%04x\n", crc_register);
	}

	// Even if you are using the low level interface you can
	// still generate the tables with the high level API:
	{
		using TBL_CFG = crc::tbl_cfg<16, CCITT_POLY, false>;
		using table_type = crc::table<TBL_CFG>;
		using small_table_type = crc::small_table<TBL_CFG>;

		// Compared to a plain "static constexpr" table definition the
		// crc::static_table<> template helps in avoiding the instantiation
		// of multiple identical tables.
		const auto& table = crc::static_table<table_type>::instance;

		// alternative ways to generate a static table at compile time:

		//const auto& table = crc::static_table<small_table_type>::instance;
		//static constexpr table_type table = table_type();
		//static constexpr small_table_type table = small_table_type();
		// The table_instance() helper methods use the crc::static_table<>
		// template to avoid the instantiation of multiple identical tables:
		//const auto& table = crc16::xmodem::table_based::table_instance();
		//const auto& table = crc16::xmodem::small_table_based::table_instance();

		// generating a table at runtime on the stack or elsewhere in memory:

		//table_type table;
		//small_table_type table;
		//crc16::xmodem::table_type table;
		//crc16::xmodem::small_table_based::table_type table;

		// All of these tables are based on either crc::basic_table<> or
		// crc::basic_small_table<> so they have an operator[](uint8) that
		// accepts indexes in the range [0..255].
		// A crc::basic_table<> has an entries[256] array (which is public)
		// while a crc::basic_small_table<> has two arrays: first_row[16] and
		// first_column[16]. This is what the operator[](uint8) abstracts away
		// when any of these tables (or a plain C array[256]) is passed through
		// the templated table parameter of core::table_based_update().

		crc::uint16 crc_register = 0;
		core::table_based_update(crc_register, data, data_end, table);
		printf("core 3: 0x%04x\n", crc_register);
	}
}

*/

#ifndef PARAMETRIC_CRC_H
#define PARAMETRIC_CRC_H

#ifndef PARAMETRIC_CRC_NO_STDINT_H
#include <stdint.h>  // not needed if you define uint16_t, uint32_t and uint64_t
#endif

namespace crc {

	using size_t = decltype(sizeof(0));

	using int8   = signed char;
	using uint8  = unsigned char;
	using uint16 = uint16_t;
	using uint32 = uint32_t;
	using uint64 = uint64_t;

	static_assert(uint16(-1) > 0, "uint16 has to be an unsigned integral type");
	static_assert(uint32(-1) > 0, "uint32 has to be an unsigned integral type");
	static_assert(uint64(-1) > 0, "uint64 has to be an unsigned integral type");

	// sizeof(char) is guaranteed to be 1 so there is no need to check it.
	// However, the number of bits in a char (CHAR_BIT) isn't 8 on some
	// exotic systems - this library doesn't support those rare platforms.
	static_assert(sizeof(uint16) == 2, "sizeof(uint16) != 2");
	static_assert(sizeof(uint32) == 4, "sizeof(uint32) != 4");
	static_assert(sizeof(uint64) == 8, "sizeof(uint64) != 8");

	template<int WIDTH> struct _uint;
	template<> struct _uint <8> { typedef uint8  type; };
	template<> struct _uint<16> { typedef uint16 type; };
	template<> struct _uint<32> { typedef uint32 type; };
	template<> struct _uint<64> { typedef uint64 type; };
	template<int WIDTH> using uint = typename _uint<WIDTH>::type;

	template <typename TABLE_TYPE>
	struct static_table {
		static_table() = delete;
		static constexpr TABLE_TYPE instance = TABLE_TYPE();
	};

	template <typename TABLE_TYPE>
	constexpr TABLE_TYPE static_table<TABLE_TYPE>::instance;

	// Reverse an N-bit quantity in parallel in 5 * lg(N) operations:
	// https://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
	constexpr uint64 reverse_bits(uint64 v) noexcept {
		v = ((v >> 1) & 0x5555555555555555) | ((v & 0x5555555555555555) << 1);
		v = ((v >> 2) & 0x3333333333333333) | ((v & 0x3333333333333333) << 2);
		v = ((v >> 4) & 0x0F0F0F0F0F0F0F0F) | ((v & 0x0F0F0F0F0F0F0F0F) << 4);
		v = ((v >> 8) & 0x00FF00FF00FF00FF) | ((v & 0x00FF00FF00FF00FF) << 8);
		v = ((v >> 16) & 0x0000FFFF0000FFFF) | ((v & 0x0000FFFF0000FFFF) << 16);
		v = (v >> 32) | (v << 32);
		return v;
	}

	constexpr uint32 reverse_bits(uint32 v) noexcept {
		v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
		v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
		v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
		v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
		v = (v >> 16) | (v << 16);
		return v;
	}

	constexpr uint16 reverse_bits(uint16 v) noexcept {
		v = ((v >> 1) & 0x5555) | ((v & 0x5555) << 1);
		v = ((v >> 2) & 0x3333) | ((v & 0x3333) << 2);
		v = ((v >> 4) & 0x0F0F) | ((v & 0x0F0F) << 4);
		v = (v >> 8) | (v << 8);
		return v;
	}

// The performance of reverse_bits(uint8) matters only when you use a CRC
// algorithm with REF_IN != REF_REG parameters. In a scenario like that all
// bytes of the input data are passed through this function. Currently none
// of the builtin CRC algorithms are declared with REF_IN != REF_REG in the
// crc8, crc16, crc32 and crc64 namespaces but the user of the library can
// declare one with crc::parameteric<>.

#ifdef PARAMETRIC_CRC_NO_REVERSE_BITS_LOOKUP_TABLE

	constexpr uint8 reverse_bits(uint8 v) noexcept {
		v = ((v >> 1) & 0x55) | ((v & 0x55) << 1);
		v = ((v >> 2) & 0x33) | ((v & 0x33) << 2);
		v = (v >> 4) | (v << 4);
		return v;
	}

#elif defined(PARAMETRIC_CRC_REVERSE_BITS_NIBBLE_LOOKUP_TABLE)

	struct reverse_bits_lookup_table {
		uint8 entries[0x10];
		constexpr reverse_bits_lookup_table() noexcept : entries() {
			for (uint8 i=0; i<0x10; i++) {
				uint8 v = ((i >> 1) & 5) | ((i & 5) << 1);
				entries[i] = ((v >> 2) | (v << 2)) & 0xf;
			}
		}
	};

	constexpr uint8 reverse_bits(uint8 v) noexcept {
		constexpr auto& tbl = static_table<reverse_bits_lookup_table>::instance.entries;
		return tbl[v >> 4] | (tbl[v & 0xf] << 4);
	}

#else

	struct reverse_bits_lookup_table {
		uint8 entries[0x100];
		constexpr reverse_bits_lookup_table() noexcept : entries() {
			for (uint16 i=0; i<0x100; i++) {
				uint8 v = uint8(i);
				v = ((v >> 1) & 0x55) | ((v & 0x55) << 1);
				v = ((v >> 2) & 0x33) | ((v & 0x33) << 2);
				entries[i] = (v >> 4) | (v << 4);
			}
		}
	};

	constexpr uint8 reverse_bits(uint8 v) noexcept {
		return static_table<reverse_bits_lookup_table>::instance.entries[v];
	}

#endif // PARAMETRIC_CRC_NO_REVERSE_BITS_LOOKUP_TABLE

	template <typename T, bool CONDITION>
	struct conditional_reflect;
	template <typename T>
	struct conditional_reflect<T, false> { static constexpr T fn(T value) noexcept { return value; } };
	template <typename T>
	struct conditional_reflect<T, true> { static constexpr T fn(T value) noexcept { return reverse_bits(value); } };

/*
	constexpr uint64 reverse_bytes(uint64 v) noexcept {
		v = ((v >> 8) & 0x00ff00ff00ff00ff) | ((v & 0x00ff00ff00ff00ff) << 8);
		v = ((v >> 16) & 0x0000ffff0000ffff) | ((v & 0x0000ffff0000ffff) << 16);
		return (v >> 32) | (v << 32);
	}

	constexpr uint32 reverse_bytes(uint32 v) noexcept {
		v = ((v >> 8) & 0x00ff00ff) | ((v & 0x00ff00ff) << 8);
		return (v >> 16) | (v << 16);
	}

	constexpr uint16 reverse_bytes(uint16 v) noexcept {
		return (v >> 8) | (v << 8);
	}

	constexpr uint8 reverse_bytes(uint8 v) noexcept {
		return v;
	}

	template <typename T, bool CONDITION>
	struct conditional_reverse_bytes;
	template <typename T>
	struct conditional_reverse_bytes<T, false> { static constexpr T fn(T value) noexcept { return value; } };
	template <typename T>
	struct conditional_reverse_bytes<T, true> { static constexpr T fn(T value) noexcept { return reverse_bytes(value); } };
*/

	template <typename T, bool REF, bool IS_UINT8=sizeof(T)==1>
	struct table_based_crc_updater;

	// sizeof(T)>1, unreflected CRC shift register
	template <typename T>
	struct table_based_crc_updater<T, false, false> {
		template <typename TABLE>
		static constexpr void update(T& crc, uint8 b, const TABLE& table) noexcept {
			crc = table[(crc >> (sizeof(crc)*8-8)) ^ b] ^ (crc << 8);
		}
	};

	// sizeof(T)>1, reflected CRC shift register
	template <typename T>
	struct table_based_crc_updater<T, true, false> {
		template <typename TABLE>
		static constexpr void update(T& crc, uint8 b, const TABLE& table) noexcept {
			crc = table[(crc & 0xff) ^ b] ^ (crc >> 8);
		}
	};

	// sizeof(T)==1, reflected or unreflected CRC shift register
	template <typename T, bool REF>
	struct table_based_crc_updater<T, REF, true> {
		template <typename TABLE>
		static constexpr void update(T& crc, uint8 b, const TABLE& table) noexcept {
			// The other two longer update methods would also work with
			// sizeof(T)==1 because expressions like (crc >> 8) translate
			// to zero but not without a warning form the compiler.
			crc = table[crc ^ b];
		}
	};

	template <int CRC_SHIFT_REGISTER_BIT_WIDTH, bool REFLECTED_CRC_SHIFT_REGISTER>
	class core;

	// unreflected CRC shift register
	template <int WIDTH>
	class core<WIDTH, false> {
		using T = uint<WIDTH>;
	public:
		using value_type = T;

		static constexpr T MSB_MASK = T(1) << (WIDTH - 1);

		// table-less bit-by-bit update
		// If num_bits<8 then you have to make sure that the unused (8-num_bits)
		// number of least significant bits in b are zeros.
		static constexpr void bbb_update(T poly, T& crc, uint8 b, uint8 num_bits=8) noexcept {
			crc ^= T(b) << (WIDTH - 8);
			for (uint8 i=0; i<num_bits; i++) {
				if (crc & MSB_MASK)
					crc = (crc << 1) ^ poly;
				else
					crc <<= 1;
			}
		}

		// bit-by-bit update with as many bits as we have in the T type
		static constexpr void bbb_update_wide(T poly, T& crc, T b) noexcept {
			crc ^= b;
			for (uint8 i=0; i<WIDTH; i++) {
				if (crc & MSB_MASK)
					crc = (crc << 1) ^ poly;
				else
					crc <<= 1;
			}
		}

		static constexpr void tableless_update(T poly, T& crc, const uint8* begin, const uint8* end) noexcept {
			for (auto p=begin; p<end; ++p)
				bbb_update(poly, crc, *p);
		}

		// Perform a CRC update using the specified lookup table.
		// TABLE is an object with support for array subscript and indexes in the
		// range [0..255]. It will be a normal C array or an object with compatible
		// operator[](index)->T, for example a table<> or small_table<> instance.
		template <typename TABLE>
		static constexpr void table_based_update(T& crc, const uint8* begin, const uint8* end, const TABLE& table) noexcept {
			for (auto p=begin; p<end; ++p)
				table_based_crc_updater<T, false>::update(crc, *p, table);
		}

		template <typename TABLE>
		static constexpr void table_based_update(T& crc, uint8 b, const TABLE& table) noexcept {
			table_based_crc_updater<T, false>::update(crc, b, table);
		}

		// calculates the value of a single table entry during table generation
		static constexpr T table_entry(T poly, uint8 index, uint8 skip_bits=0) noexcept {
			T entry = 0;
			bbb_update(poly, entry, index << skip_bits, 8 - skip_bits);
			return entry;
		}

#ifdef PARAMETRIC_CRC_SIMPLE_TABLE_GENERATOR

		static constexpr void generate_table(T poly, T entries[256]) noexcept {
			entries[0] = 0;

			uint8 i = 1;
			do entries[i] = table_entry(poly, i);
			while (i++ < 255);
/*
			// The equivalent version below didn't compile as a constexpr:

			for (uint8 i=1; i; i++)
				entries[i] = table_entry(poly, i);
*/
		}

#else

		static constexpr void generate_table(T poly, T entries[256]) noexcept {
			// generating the first row
			entries[0] = 0;
			// the upper nibble (4 bits) is zero so table_entry() can skip it
			for (uint8 i=1; i<0x10; i++)
				entries[i] = table_entry(poly, i, 4);

			// 15 more rows

			// table[i xor j] == table[i] xor table[j]
			// source: https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#Generating_the_tables
			//
			// In the code below k and i are the upper and lower nibbles of the
			// index so xor works like addition.

			uint8 k = 0;
			do {
				k += 0x10;
				entries[k] = table_entry(poly, k, 0);
				for (uint8 i=1; i<0x10; i++)
					entries[k^i] = entries[k] ^ entries[i];
			} while (k < 0xf0);
/*
			// The equivalent version below didn't compile as a constexpr:

			uint8 k = 0x10;
			do {
				entries[k] = table_entry(poly, k, 0);
				for (uint8 i=1; i<0x10; i++)
					entries[k^i] = entries[k] ^ entries[i];
			} while (k += 0x10);
*/
		}

#endif // PARAMETRIC_CRC_SIMPLE_TABLE_GENERATOR

		static constexpr void generate_small_table(T poly, T first_row[16], T first_column[16]) noexcept {
			first_row[0] = 0;
			// the upper nibble (4 bits) is zero so table_entry() can skip it
			for (uint8 i=1; i<0x10; i++)
				first_row[i] = table_entry(poly, i, 4);

			first_column[0] = 0;
			for (uint8 k=1; k<0x10; k++)
				first_column[k] = table_entry(poly, k<<4, 0);
		}
	};

	// reflected CRC shift register
	template <int WIDTH>
	class core<WIDTH, true> {
		using T = uint<WIDTH>;
	public:
		using value_type = T;

		static constexpr void bbb_update(T ref_poly, T& crc, uint8 b, uint8 num_bits=8) noexcept {
			crc ^= b;
			for (uint8 i=0; i<num_bits; i++) {
				if (crc & 1)
					crc = (crc >> 1) ^ ref_poly;
				else
					crc >>= 1;
			}
		}

		static constexpr void bbb_update_wide(T ref_poly, T& crc, T b) noexcept {
			crc ^= b;
			for (uint8 i=0; i<WIDTH; i++) {
				if (crc & 1)
					crc = (crc >> 1) ^ ref_poly;
				else
					crc >>= 1;
			}
		}

		static constexpr void tableless_update(T ref_poly, T& crc, const uint8* begin, const uint8* end) noexcept {
			for (auto p = begin; p<end; ++p)
				bbb_update(ref_poly, crc, *p);
		}

		template <typename TABLE>
		static constexpr void table_based_update(T& crc, const uint8* begin, const uint8* end, const TABLE& table) noexcept {
			for (auto p = begin; p<end; ++p)
				table_based_crc_updater<T, true>::update(crc, *p, table);
		}

		template <typename TABLE>
		static constexpr void table_based_update(T& crc, uint8 b, const TABLE& table) noexcept {
			table_based_crc_updater<T, true>::update(crc, b, table);
		}

		static constexpr T table_entry(T ref_poly, uint8 index, uint8 skip_bits=0) noexcept {
			T entry = 0;
			bbb_update(ref_poly, entry, index >> skip_bits, 8 - skip_bits);
			return entry;
		}

#ifdef PARAMETRIC_CRC_SIMPLE_TABLE_GENERATOR

		static constexpr void generate_table(T ref_poly, T entries[256]) noexcept {
			entries[0] = 0;

			uint8 i = 1;
			do entries[i] = table_entry(ref_poly, i);
			while (i++ < 255);
/*
			// The equivalent version below didn't compile as a constexpr:

			for (uint8 i=1; i; i++)
				entries[i] = table_entry(ref_poly, i);
*/
		}

#else

		static constexpr void generate_table(T ref_poly, T entries[256]) noexcept {
			// generating the first row
			entries[0] = 0;
			for (uint8 i=1; i<0x10; i++)
				entries[i] = table_entry(ref_poly, i, 0);

			// 15 more rows

			// table[i xor j] == table[i] xor table[j]
			// source: https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#Generating_the_tables
			//
			// In the code below k and i are the upper and lower nibbles of the
			// index so xor works like addition.

			uint8 k = 0;
			do {
				k += 0x10;
				// the lower nibble (4 bits) is zero so table_entry() can skip it
				entries[k] = table_entry(ref_poly, k, 4);
				for (uint8 i=1; i<0x10; i++)
					entries[k^i] = entries[k] ^ entries[i];
			} while (k < 0xf0);
/*
			// The equivalent version below didn't compile as a constexpr:

			uint8 k = 0x10;
			do {
				// the lower nibble (4 bits) is zero so table_entry() can skip it
				entries[k] = table_entry(ref_poly, k, 4);
				for (uint8 i=1; i<0x10; i++)
					entries[k^i] = entries[k] ^ entries[i];
			} while (k += 0x10);
*/
		}

#endif // PARAMETRIC_CRC_SIMPLE_TABLE_GENERATOR

		static constexpr void generate_small_table(T ref_poly, T first_row[16], T first_column[16]) noexcept {
			first_row[0] = 0;
			for (uint8 i=1; i<0x10; i++)
				first_row[i] = table_entry(ref_poly, i, 0);

			first_column[0] = 0;
			for (uint8 k=1; k<0x10; k++)
				// the lower nibble (4 bits) is zero so table_entry() can skip it
				first_column[k] = table_entry(ref_poly, k<<4, 4);
		}
	};

	enum uninitialized_type { UNINITIALIZED };

	template <typename T>
	struct basic_table {
		constexpr basic_table() noexcept : entries() {}
		// Unlike the default constexpr constructor this one doesn't call
		// the constructor of the entries array so there is no zero fill.
		basic_table(uninitialized_type) noexcept {}

		using value_type = T;

		T entries[256];

		constexpr T operator[](uint8 index) const noexcept {
			return entries[index];
		}
	};

	template <typename T>
	struct basic_small_table {
		constexpr basic_small_table() noexcept : first_row(), first_column() {}
		// Unlike the default constexpr constructor this one doesn't call the
		// constructor of the first_row and first_column arrays so there is no
		// zero fill.
		basic_small_table(uninitialized_type) noexcept {}

		using value_type = T;

		T first_row[16];
		T first_column[16];

		constexpr T operator[](uint8 index) const noexcept {
			return first_row[index & 0x0f] ^ first_column[index >> 4];
		}
	};

	// tbl_cfg is the config parameter for the table template classes below.
	//
	// The POLY template parameter is always in unreflected form by convention.
	// This is how the RevEng CRC catalog and the literature usually refer to
	// it regardless of the value of the the refin and refout parameters.
	template <
		int WIDTH_,         // bit width of the CRC shift register
		uint<WIDTH_> POLY_, // unreflected poly
		bool REF_REG_       // reflected CRC shift register and table entries
	>
	struct tbl_cfg {
		using T = uint<WIDTH_>;
		static constexpr int WIDTH = WIDTH_;
		static constexpr T POLY = POLY_;
		static constexpr bool REF_REG = REF_REG_;
		static constexpr T ACTUAL_POLY = conditional_reflect<T, REF_REG_>::fn(POLY_);
	};

	template <typename TBL_CFG>
	struct table : public basic_table<typename TBL_CFG::T> {
		// Pass the UNINITIALIZED constant to the constructor if you
		// want to skip the table generation and do it later manually.
		table(uninitialized_type) : basic_table<typename TBL_CFG::T>(UNINITIALIZED) {}

		constexpr table() noexcept {
			generate();
		}

		constexpr void generate() noexcept {
			core<TBL_CFG::WIDTH, TBL_CFG::REF_REG>::generate_table(
				TBL_CFG::ACTUAL_POLY, this->entries);
		}
	};

	template <typename TBL_CFG>
	struct small_table : public basic_small_table<typename TBL_CFG::T> {
		// Pass the UNINITIALIZED constant to the constructor if you
		// want to skip the table generation and do it later manually.
		small_table(uninitialized_type) : basic_small_table<typename TBL_CFG::T>(UNINITIALIZED) {}

		constexpr small_table() noexcept {
			generate();
		}

		constexpr void generate() noexcept {
			core<TBL_CFG::WIDTH, TBL_CFG::REF_REG>::generate_small_table(
				TBL_CFG::ACTUAL_POLY, this->first_row, this->first_column);
		}
	};

	// To be used as the 'UPDATER' template parameter of the 'impl' class.
	template <typename TBL_CFG>
	struct updater_tableless {
		using table_type = void;
	protected:
		static constexpr void update(
			typename TBL_CFG::T& crc, const uint8* begin, const uint8* end) noexcept {
			core<TBL_CFG::WIDTH, TBL_CFG::REF_REG>::tableless_update(
				TBL_CFG::ACTUAL_POLY, crc, begin, end);
		}
	};

	// To be used as the 'UPDATER' template parameter of the 'impl' class.
	template <typename TBL_CFG>
	struct updater_table_based {
		using table_type = table<TBL_CFG>;
		static constexpr const table_type& table_instance() noexcept {
			return static_table<table_type>::instance;
		}
	protected:
		static constexpr void update(
			typename TBL_CFG::T& crc, const uint8* begin, const uint8* end) noexcept {
			// Casting the table to the base type, this way we can get away with
			// less instantiations of the table_based_update() template function
			// even if a program uses a lot of different types of CRC algorithms
			// (with different TBL_CFG settings) at the same time.
			// The table_type is different for every unique TBL_CFG but the same
			// doesn't apply to basic_table<>. The whole TBL_CFG has already
			// been encoded into the contents of the table so all that core
			// needs is an operator[](uint8) to access the 256 table entries.
			core<TBL_CFG::WIDTH, TBL_CFG::REF_REG>::table_based_update(
				crc, begin, end, (const basic_table<typename TBL_CFG::T>&)static_table<table_type>::instance);
		}
	};

	// To be used as the 'UPDATER' template parameter of the 'impl' class.
	template <typename TBL_CFG>
	struct updater_small_table_based {
		using table_type = small_table<TBL_CFG>;
		static constexpr const table_type& table_instance() noexcept {
			return static_table<table_type>::instance;
		}
	protected:
		static constexpr void update(
			typename TBL_CFG::T& crc, const uint8* begin, const uint8* end) noexcept {
			core<TBL_CFG::WIDTH, TBL_CFG::REF_REG>::table_based_update(
				crc, begin, end, (const basic_small_table<typename TBL_CFG::T>&)static_table<table_type>::instance);
		}
	};

	// To be used as the 'UPDATER' template parameter of the 'impl_ext' class.
	template <typename TBL_CFG>
	struct updater_ext_table_based {
		using table_type = table<TBL_CFG>;
	protected:
		static constexpr void update(typename TBL_CFG::T& crc,
			const uint8* begin, const uint8* end, const table_type& t) noexcept {
			core<TBL_CFG::WIDTH, TBL_CFG::REF_REG>::table_based_update(
				crc, begin, end, (const basic_table<typename TBL_CFG::T>&)t);
		}
	};

	// To be used as the 'UPDATER' template parameter of the 'impl_ext' class.
	template <typename TBL_CFG>
	struct updater_ext_small_table_based {
		using table_type = small_table<TBL_CFG>;
	protected:
		static constexpr void update(typename TBL_CFG::T& crc,
			const uint8* begin, const uint8* end, const table_type& t) noexcept {
			core<TBL_CFG::WIDTH, TBL_CFG::REF_REG>::table_based_update(
				crc, begin, end, (const basic_small_table<typename TBL_CFG::T>&)t);
		}
	};

	// The cfg template holds the parameters of a CRC algorithm.
	//
	// A CRC algorithm has two reflect parameters (REF_IN, REF_OUT) while the
	// implementation of a parametrized CRC library has an additional reflect
	// parameter that is usually hardcoded and isn't configurable:
	// the "reflectedness" of the CRC shift register. In this library you can
	// configure this through the REF_REG parameter. A CRC algorithm should
	// always work and produce the same output irrespective of the value of
	// REF_REG but different REF_REG settings come with different trade-offs.
	//
	// If REF_REG!=REF_IN then the bit order has to be reversed in the bytes
	// of the input data - this is why the default setting is REF_REG=REF_IN.
	//
	// CRC algorithms configured with the same POLY and REF_REG values use
	// identical lookup tables.
	//
	// If you decide to set up a CRC algorithm with parameters where
	// REF_REG!=REF_IN then the reverse_bits(uint8) function - that is
	// otherwise unused or almost unused - starts working overtime so you have
	// another choice to make: whether to reverse the bits of the input bytes in
	// code or through a lookup table. This trade-off between performance and
	// memory usage can be configured with the
	// PARAMETRIC_CRC_NO_REVERSE_BITS_LOOKUP_TABLE define.
	template <
		int WIDTH_,               // bit width of the CRC shift register
		uint<WIDTH_> POLY_,       // unreflected poly
		uint<WIDTH_> INIT_,       // unreflected init
		uint<WIDTH_> XOR_OUT_,
		bool REF_IN_,             // reflected input
		bool REF_OUT_ = REF_IN_,  // reflected output
		bool REF_REG_ = REF_IN_   // reflected CRC shift register and table entries
	>
	struct cfg {
		using T = uint<WIDTH_>;
		static constexpr int WIDTH = WIDTH_;
		static constexpr T POLY = POLY_;
		static constexpr T INIT = INIT_;
		static constexpr T XOR_OUT = XOR_OUT_;
		static constexpr bool REF_IN = REF_IN_;
		static constexpr bool REF_OUT = REF_OUT_;
		static constexpr bool REF_REG = REF_REG_;

		static constexpr T ACTUAL_INIT = conditional_reflect<T, REF_REG_>::fn(INIT_);
		static constexpr T ACTUAL_POLY = conditional_reflect<T, REF_REG_>::fn(POLY_);
		using TBL_CFG = tbl_cfg<WIDTH, POLY, REF_REG_>;
	};

	// Calculates the residue constant. This implementation is based on the
	// description provided by the RevEng project:
	// https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.legend.params
	//
	// Quote:
	//
	//   Residue:
	//   The contents of the register after initialising, reading an error-free
	//   codeword and optionally reflecting the register (if refout=true), but
	//   not applying the final XOR. This is mathematically equivalent to
	//   initialising the register with the xorout parameter, reflecting it as
	//   described (if refout=true), reading as many zero bits as there are
	//   cells in the register, and reflecting the result (if refin=true).
	//   The residue of a crossed-endian model is calculated assuming that
	//   the characters of the received CRC are specially reflected before
	//   submitting the codeword.
	template <typename CFG>
	class residue_const_calculator {
		using T = typename CFG::T;

		static constexpr T residue_const() noexcept {
			T residue = conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(CFG::XOR_OUT);
			core<CFG::WIDTH, CFG::REF_REG>::bbb_update_wide(CFG::ACTUAL_POLY, residue, 0);
			return conditional_reflect<T, CFG::REF_REG!=CFG::REF_IN>::fn(residue);

			// Before finding the above short formula on the RevEng website I
			// used the solution below that works as follows:
			// It forms the shortest possible codeword by calculating the CRC of
			// the empty string. It then calculates the CRC of that codeword
			// without the xorout step. It uses the tableless bit-by-bit (bbb)
			// CRC calculator. The two optimizations that I introduced have
			// moved the solution closer to the short formula above.

/*
#if 0
			// The CRC of the empty string:
			T codeword = conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(CFG::ACTUAL_INIT) ^ CFG::XOR_OUT;
			// Turning it into a valid codeword:
			codeword = conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(codeword);
#else
			// Optimized version:
			T codeword = CFG::ACTUAL_INIT ^ conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(CFG::XOR_OUT);
#endif

#if 0
			// Calculating the CRC of the codeword by treating it as an input
			// byte stream. Unfortunately this introduces the problem of
			// endianness when the CRC is wider than 8 bits. The bytes of the
			// CRC have to be appended to the dataword (which is the empty
			// string in our case) in the correct order to form a valid codeword.

			codeword = conditional_reverse_bytes<T, !CFG::REF_REG>::fn(codeword);

			T residue = CFG::ACTUAL_INIT;
			for (size_t i=0; i<sizeof(T); i++,codeword>>=8)
				core<CFG::WIDTH,CFG::REF_REG>::bbb_update(
					CFG::ACTUAL_POLY, residue, uint8(codeword));
			return conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(residue); // no xorout
#else
			// Optimized version:

			// The codeword happens to have the exact same bit length as our CRC
			// register so we can avoid looping over bytes and the problem of
			// endianness by using bbb_update_wide() instead of bbb_update():

			T residue = CFG::ACTUAL_INIT;
			core<CFG::WIDTH,CFG::REF_REG>::bbb_update_wide(CFG::ACTUAL_POLY, residue, codeword);
			return conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(residue); // no xorout
#endif
*/
		}

	public:
		static constexpr T RESIDUE = residue_const();
	};

	// In C++17 this could be solved with an "in-class explicit specialization"
	// by adding a "template <bool REVERSE> void update()" method to the impl
	// class with two specializations.
	template <typename UPDATER, typename T, bool REVERSE_THE_BITS_OF_THE_INPUT_BYTES>
	struct impl_input_reverser;

	template <typename UPDATER, typename T>
	struct impl_input_reverser<UPDATER,T,false> : public UPDATER {
		static constexpr void update(T& crc, const uint8* begin, const uint8* end) noexcept {
			UPDATER::update(crc, begin, end);
		}
	};

	template <typename UPDATER, typename T>
	struct impl_input_reverser<UPDATER,T,true> : public UPDATER {
		static constexpr void update(T& crc, const uint8* begin, const uint8* end) noexcept {
			// this happens only when REF_IN != REF_REG
			for (const uint8* p=begin; p<end; p++) {
				uint8 b = reverse_bits(*p);
				UPDATER::update(crc, &b, &b+1);
			}
		}
	};

	// CRC implementation that doesn't receive an extra table parameter in
	// its update method. The UPDATER template parameter is the plugin that
	// integrates the table type into the class. The tableless updater performs
	// the update without table lookup and defines its table_type as void.
	//
	// In theory this class could inherit from impl_ext or encapsulate a private
	// instance of it and just pass a static constexpr table instance
	// (identified by CFG::TBL_CFG) to its update() and calculate() methods but
	// we wouldn't benefit much from that approach especially in case of the
	// "tableless" mode. Not to mention that most users will probably use this
	// implementation so providing a leaner solution without that extra layer
	// below isn't a bad idea.
	template <typename CFG, typename UPDATER>
	class impl : public CFG, public UPDATER {
		using T = typename CFG::T;
		T _crc;

	public:
		using value_type = T;
		using table_type = typename UPDATER::table_type;

		static constexpr T RESIDUE = residue_const_calculator<CFG>::RESIDUE;

		constexpr impl(T interim_remainder=CFG::ACTUAL_INIT) noexcept : _crc(interim_remainder) {}

		// Returns the final CRC value. This value shouldn't be passed to the
		// constructor as an attempt to continue the CRC calculation. That would
		// work only with some CRC algorithms.
		constexpr T final() const noexcept {
			return residue() ^ CFG::XOR_OUT;
		}

		// Returns an interim remainder that can be passed to the constructor to
		// continue the CRC calculation later.
		constexpr T interim() const noexcept {
			return _crc;
		}

		constexpr T residue() const noexcept {
			return conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(_crc);
		}

		constexpr void update(uint8 b) noexcept {
			impl_input_reverser<UPDATER,T,CFG::REF_IN!=CFG::REF_REG>::update(_crc, &b, &b+1);
		}
		constexpr void update(int8 b) noexcept {
			update(uint8(b));
		}
		constexpr void update(char b) noexcept {
			update(uint8(b));
		}

		// If you want a constexpr update() with a buffer then use the (uint8*)
		// methods. The void pointer method uses reinterpret cast and so it is
		// unlikely to evaluate as constexpr. A smarter compiler or a future C++
		// standard could solve this issue at some point in the future.
		// The same applies to the calculate() methods.
		// This limitation doesn't seem to apply to the byte based update above
		// where casting between uint8/int8/char works for me without issues.
		constexpr void update(const uint8* begin, const uint8* end) noexcept {
			impl_input_reverser<UPDATER,T,CFG::REF_IN!=CFG::REF_REG>::update(_crc, begin, end);
		}
		constexpr void update(const uint8* data, size_t size) noexcept {
			update(data, data+size);
		}
		constexpr void update(const void* data, size_t size) noexcept {
			update((const uint8*)data, (const uint8*)data + size);
		}

		static constexpr T calculate(const uint8* begin, const uint8* end) noexcept {
			impl crc;
			crc.update(begin, end);
			return crc.final();
		}
		static constexpr T calculate(const uint8* data, size_t size) noexcept {
			return calculate(data, data+size);
		}
		static constexpr T calculate(const void* data, size_t size) noexcept {
			return calculate((const uint8*)data, (const uint8*)data + size);
		}
	};

	// In C++17 this could be solved with an "in-class explicit specialization"
	// by adding a "template <bool REVERSE> void update()" method to the impl_ext
	// class with two specializations.
	template <typename UPDATER, typename T, bool REVERSE_THE_BITS_OF_THE_INPUT_BYTES>
	struct impl_ext_input_reverser;

	template <typename UPDATER, typename T>
	struct impl_ext_input_reverser<UPDATER,T,false> : public UPDATER {
		static constexpr void update(T& crc, const uint8* begin, const uint8* end,
				const typename UPDATER::table_type& table) noexcept {
			UPDATER::update(crc, begin, end, table);
		}
	};

	template <typename UPDATER, typename T>
	struct impl_ext_input_reverser<UPDATER,T,true> : public UPDATER {
		static constexpr void update(T& crc, const uint8* begin, const uint8* end,
				const typename UPDATER::table_type& table) noexcept {
			// this happens only when REF_IN != REF_REG
			for (const uint8* p=begin; p<end; p++) {
				uint8 b = reverse_bits(*p);
				UPDATER::update(crc, &b, &b+1, table);
			}
		}
	};

	// CRC implementation with external table that is provided through an extra
	// parameter to the update() and calculate() methods. The UPDATER template
	// parameter is the plugin that integrates the table type into the class.
	template <typename CFG, typename UPDATER>
	class impl_ext : public CFG, public UPDATER {
		using T = typename CFG::T;
		T _crc;

	public:
		using value_type = T;
		using table_type = typename UPDATER::table_type;

		static constexpr T RESIDUE = residue_const_calculator<CFG>::RESIDUE;

		constexpr impl_ext(T interim_remainder=CFG::ACTUAL_INIT) noexcept : _crc(interim_remainder) {}

		// Returns the final CRC value. This value shouldn't be passed to the
		// constructor as an attempt to continue the CRC calculation. That would
		// work only with some CRC algorithms.
		constexpr T final() const noexcept {
			return residue() ^ CFG::XOR_OUT;
		}

		// Returns an interim remainder that can be passed to the constructor to
		// continue the CRC calculation later.
		constexpr T interim() const noexcept {
			return _crc;
		}

		constexpr T residue() const noexcept {
			return conditional_reflect<T, CFG::REF_REG!=CFG::REF_OUT>::fn(_crc);
		}

		constexpr void update(uint8 b, const table_type& table) noexcept {
			impl_ext_input_reverser<UPDATER,T,CFG::REF_IN!=CFG::REF_REG>::update(_crc, &b, &b+1, table);
		}
		constexpr void update(int8 b, const table_type& table) noexcept {
			update(uint8(b), table);
		}
		constexpr void update(char b, const table_type& table) noexcept {
			update(uint8(b), table);
		}

		constexpr void update(const uint8* begin, const uint8* end, const table_type& table) noexcept {
			impl_ext_input_reverser<UPDATER,T,CFG::REF_IN!=CFG::REF_REG>::update(_crc, begin, end, table);
		}
		constexpr void update(const uint8* data, size_t size, const table_type& table) noexcept {
			update(data, data+size, table);
		}
		constexpr void update(const void* data, size_t size, const table_type& table) noexcept {
			update((const uint8*)data, (const uint8*)data + size, table);
		}

		static constexpr T calculate(const uint8* begin, const uint8* end, const table_type& table) noexcept {
			impl_ext crc;
			crc.update(begin, end, table);
			return crc.final();
		}
		static constexpr T calculate(const uint8* data, size_t size, const table_type& table) noexcept {
			return calculate(data, data+size, table);
		}
		static constexpr T calculate(const void* data, size_t size, const table_type& table) noexcept {
			return calculate((const uint8*)data, (const uint8*)data + size, table);
		}
	};

	template <
		int WIDTH_,             // bit width of the CRC shift register
		uint<WIDTH_> POLY_,     // unreflected poly
		uint<WIDTH_> INIT_,     // unreflected init
		uint<WIDTH_> XOR_OUT_,
		bool REF_IN_,           // reflected input
		bool REF_OUT_=REF_IN_,  // reflected output
		bool REF_REG_=REF_IN_   // reflected CRC shift register and table entries
	>
	class parametric : public impl< // the implementation of the "table_based" mode
			cfg<WIDTH_, POLY_, INIT_, XOR_OUT_, REF_IN_, REF_OUT_, REF_REG_>,
			updater_table_based<tbl_cfg<WIDTH_, POLY_, REF_REG_>>> {

		using CFG = cfg<WIDTH_, POLY_, INIT_, XOR_OUT_, REF_IN_, REF_OUT_, REF_REG_>;

	public:
		using value_type = typename CFG::T;

		// CRC classes:

		using table_based           = parametric;
		using small_table_based     = impl<CFG, updater_small_table_based<typename CFG::TBL_CFG>>;
		using ext_table_based       = impl_ext<CFG, updater_ext_table_based<typename CFG::TBL_CFG>>;
		using ext_small_table_based = impl_ext<CFG, updater_ext_small_table_based<typename CFG::TBL_CFG>>;
		using tableless             = impl<CFG, updater_tableless<typename CFG::TBL_CFG>>;
	};

} // namespace crc

// For more info on the below CRC algorithms visit
// https://reveng.sourceforge.io/crc-catalogue/all.htm

namespace crc8 {
	using rohc       = crc::parametric<8, 0x07, 0xff, 0x00, true >;  // CRC-8/ROHC
	using i_432_1    = crc::parametric<8, 0x07, 0x00, 0x55, false>;  // CRC-8/I-432-1    Alias: CRC-8/ITU
	using smbus      = crc::parametric<8, 0x07, 0x00, 0x00, false>;  // CRC-8/SMBUS      Alias: CRC-8
	using tech_3250  = crc::parametric<8, 0x1d, 0xff, 0x00, true >;  // CRC-8/TECH-3250  Alias: CRC-8/AES, CRC-8/EBU
	using gsm_a      = crc::parametric<8, 0x1d, 0x00, 0x00, false>;  // CRC-8/GSM-A
	using mifare_mad = crc::parametric<8, 0x1d, 0xc7, 0x00, false>;  // CRC-8/MIFARE-MAD
	using i_code     = crc::parametric<8, 0x1d, 0xfd, 0x00, false>;  // CRC-8/I-CODE
	using hitag      = crc::parametric<8, 0x1d, 0xff, 0x00, false>;  // CRC-8/HITAG
	using sae_j1850  = crc::parametric<8, 0x1d, 0xff, 0xff, false>;  // CRC-8/SAE-J1850
	using opensafety = crc::parametric<8, 0x2f, 0x00, 0x00, false>;  // CRC-8/OPENSAFETY
	using autosar    = crc::parametric<8, 0x2f, 0xff, 0xff, false>;  // CRC-8/AUTOSAR
	using maxim_dow  = crc::parametric<8, 0x31, 0x00, 0x00, true >;  // CRC-8/MAXIM-DOW  Alias: CRC-8/MAXIM, CRC-8/DOW-CRC
	using nrsc_5     = crc::parametric<8, 0x31, 0xff, 0x00, false>;  // CRC-8/NRSC-5
	using darc       = crc::parametric<8, 0x39, 0x00, 0x00, true >;  // CRC-8/DARC
	using gsm_b      = crc::parametric<8, 0x49, 0x00, 0xff, false>;  // CRC-8/GSM-B
	using wcdma      = crc::parametric<8, 0x9b, 0x00, 0x00, true >;  // CRC-8/WCDMA
	using lte        = crc::parametric<8, 0x9b, 0x00, 0x00, false>;  // CRC-8/LTE
	using cdma2000   = crc::parametric<8, 0x9b, 0xff, 0x00, false>;  // CRC-8/CDMA2000
	using bluetooth  = crc::parametric<8, 0xa7, 0x00, 0x00, true >;  // CRC-8/BLUETOOTH
	using dvb_s2     = crc::parametric<8, 0xd5, 0x00, 0x00, false>;  // CRC-8/DVB-S2
	using crc8       = smbus;
} // namespace crc8

namespace crc16 {
	using dect_x       = crc::parametric<16, 0x0589, 0x0000, 0x0000, false>;  // CRC-16/DECT-X    Alias: X-CRC-16
	using dect_r       = crc::parametric<16, 0x0589, 0x0000, 0x0001, false>;  // CRC-16/DECT-R    Alias: R-CRC-16
	using nrsc_5       = crc::parametric<16, 0x080b, 0xffff, 0x0000, true >;  // CRC-16/NRSC-5
	using dnp          = crc::parametric<16, 0x3d65, 0x0000, 0xffff, true >;  // CRC-16/DNP
	using en_13757     = crc::parametric<16, 0x3d65, 0x0000, 0xffff, false>;  // CRC-16/EN-13757
	using kermit       = crc::parametric<16, 0x1021, 0x0000, 0x0000, true >;  // CRC-16/KERMIT    Alias: CRC-16/BLUETOOTH, CRC-16/CCITT, CRC-16/CCITT-TRUE, CRC-16/V-41-LSB, CRC-CCITT
	using tms37157     = crc::parametric<16, 0x1021, 0x89ec, 0x0000, true >;  // CRC-16/TMS37157
	using riello       = crc::parametric<16, 0x1021, 0xb2aa, 0x0000, true >;  // CRC-16/RIELLO
	using a            = crc::parametric<16, 0x1021, 0xc6c6, 0x0000, true >;  // CRC-16/ISO-IEC-14443-3-A  Alias: CRC-A
	using mcrf4xx      = crc::parametric<16, 0x1021, 0xffff, 0x0000, true >;  // CRC-16/MCRF4XX
	using ibm_sdlc     = crc::parametric<16, 0x1021, 0xffff, 0xffff, true >;  // CRC-16/IBM-SDLC  Alias: CRC-16/ISO-HDLC, CRC-16/ISO-IEC-14443-3-B, CRC-16/X-25, CRC-B, X-25
	using xmodem       = crc::parametric<16, 0x1021, 0x0000, 0x0000, false>;  // CRC-16/XMODEM    Alias: CRC-16/ACORN, CRC-16/LTE, CRC-16/V-41-MSB, XMODEM, ZMODEM
	using gsm          = crc::parametric<16, 0x1021, 0x0000, 0xffff, false>;  // CRC-16/GSM
	using spi_fujitsu  = crc::parametric<16, 0x1021, 0x1d0f, 0x0000, false>;  // CRC-16/SPI-FUJITSU Alias: CRC-16/AUG-CCITT
	using ibm_3740     = crc::parametric<16, 0x1021, 0xffff, 0x0000, false>;  // CRC-16/IBM-3740  Alias: CRC-16/AUTOSAR, CRC-16/CCITT-FALSE
	using genibus      = crc::parametric<16, 0x1021, 0xffff, 0xffff, false>;  // CRC-16/GENIBUS   Alias: CRC-16/DARC, CRC-16/EPC, CRC-16/EPC-C1G2, CRC-16/I-CODE
	using profibus     = crc::parametric<16, 0x1dcf, 0xffff, 0xffff, false>;  // CRC-16/PROFIBUS  Alias: CRC-16/IEC-61158-2
	using opensafety_a = crc::parametric<16, 0x5935, 0x0000, 0x0000, false>;  // CRC-16/OPENSAFETY-A
	using m17          = crc::parametric<16, 0x5935, 0xffff, 0x0000, false>;  // CRC-16/M17
	using lj1200       = crc::parametric<16, 0x6f63, 0x0000, 0x0000, false>;  // CRC-16/LJ1200
	using opensafety_b = crc::parametric<16, 0x755b, 0x0000, 0x0000, false>;  // CRC-16/OPENSAFETY-B
	using arc          = crc::parametric<16, 0x8005, 0x0000, 0x0000, true >;  // CRC-16/ARC       Alias: ARC, CRC-16, CRC-16/LHA, CRC-IBM
	using maxim_dow    = crc::parametric<16, 0x8005, 0x0000, 0xffff, true >;  // CRC-16/MAXIM-DOW Alias: CRC-16/MAXIM
	using modbus       = crc::parametric<16, 0x8005, 0xffff, 0x0000, true >;  // CRC-16/MODBUS
	using usb          = crc::parametric<16, 0x8005, 0xffff, 0xffff, true >;  // CRC-16/USB
	using umts         = crc::parametric<16, 0x8005, 0x0000, 0x0000, false>;  // CRC-16/UMTS      Alias: CRC-16/BUYPASS, CRC-16/VERIFONE
	using dds_110      = crc::parametric<16, 0x8005, 0x800d, 0x0000, false>;  // CRC-16/DDS-110
	using cms          = crc::parametric<16, 0x8005, 0xffff, 0x0000, false>;  // CRC-16/CMS
	using t10_dif      = crc::parametric<16, 0x8bb7, 0x0000, 0x0000, false>;  // CRC-16/T10-DIF
	using teledisk     = crc::parametric<16, 0xa097, 0x0000, 0x0000, false>;  // CRC-16/TELEDISK
	using cdma2000     = crc::parametric<16, 0xc867, 0xffff, 0x0000, false>;  // CRC-16/CDMA2000
	using crc16        = arc;
	using bluetooth    = kermit;
	using ccitt        = kermit;
	using v41_lsb      = kermit;
	using v41_msb      = xmodem;
	using zmodem       = xmodem;
	using aug_ccitt    = spi_fujitsu;
	using ccitt_false  = ibm_3740; // commonly misidentified as CCITT
	using autosar      = ibm_3740;
	using darc         = genibus;
	using b            = ibm_sdlc;
	using x25          = ibm_sdlc;
} // namespace crc16

namespace crc32 {
	using xfer       = crc::parametric<32, 0x000000af, 0x00000000, 0x00000000, false>;  // CRC-32/XFER
	using jamcrc     = crc::parametric<32, 0x04c11db7, 0xffffffff, 0x00000000, true >;  // CRC-32/JAMCRC
	using iso_hdlc   = crc::parametric<32, 0x04c11db7, 0xffffffff, 0xffffffff, true >;  // CRC-32/ISO-HDLC  Alias: CRC-32, CRC-32/ADCCP, CRC-32/V-42, CRC-32/XZ, PKZIP
	using cksum      = crc::parametric<32, 0x04c11db7, 0x00000000, 0xffffffff, false>;  // CRC-32/CKSUM     Alias: CRC-32/POSIX
	using mpeg2      = crc::parametric<32, 0x04c11db7, 0xffffffff, 0x00000000, false>;  // CRC-32/MPEG-2
	using bzip2      = crc::parametric<32, 0x04c11db7, 0xffffffff, 0xffffffff, false>;  // CRC-32/BZIP2     Alias: CRC-32/AAL5, CRC-32/DECT-B, B-CRC-32
	using iscsi      = crc::parametric<32, 0x1edc6f41, 0xffffffff, 0xffffffff, true >;  // CRC-32/ISCSI     Alias: CRC-32/BASE91-C, CRC-32/CASTAGNOLI, CRC-32/INTERLAKEN, CRC-32C
	using mef        = crc::parametric<32, 0x741b8cd7, 0xffffffff, 0x00000000, true >;  // CRC-32/MEF
	using cd_rom_edc = crc::parametric<32, 0x8001801b, 0x00000000, 0x00000000, true >;  // CRC-32/CD-ROM-EDC
	using aixm       = crc::parametric<32, 0x814141ab, 0x00000000, 0x00000000, false>;  // CRC-32/AIXM      Alias: CRC-32Q
	using base91_d   = crc::parametric<32, 0xa833982b, 0xffffffff, 0xffffffff, true >;  // CRC-32/BASE91-D  Alias: CRC-32D
	using autosar    = crc::parametric<32, 0xf4acfb13, 0xffffffff, 0xffffffff, true >;  // CRC-32/AUTOSAR
	using crc32      = iso_hdlc;
	using pkzip      = iso_hdlc;
	using v42        = iso_hdlc;
	using xz         = iso_hdlc;
	using posix      = cksum;
	using castagnoli = iscsi;
	using c          = iscsi;
	using d          = base91_d;
	using q          = aixm;
} // namespace crc32

namespace crc64 {
	using go_iso   = crc::parametric<64, 0x000000000000001b, 0xffffffffffffffff, 0xffffffffffffffff, true >;  // CRC-64/GO-ISO
	using ms       = crc::parametric<64, 0x259c84cba6426349, 0xffffffffffffffff, 0x0000000000000000, true >;  // CRC-64/MS
	using xz       = crc::parametric<64, 0x42f0e1eba9ea3693, 0xffffffffffffffff, 0xffffffffffffffff, true >;  // CRC-64/XZ        Alias: CRC-64/GO-ECMA
	using ecma_182 = crc::parametric<64, 0x42f0e1eba9ea3693, 0x0000000000000000, 0x0000000000000000, false>;  // CRC-64/ECMA-182  Alias: CRC-64
	using we       = crc::parametric<64, 0x42f0e1eba9ea3693, 0xffffffffffffffff, 0xffffffffffffffff, false>;  // CRC-64/WE
	using redis    = crc::parametric<64, 0xad93d23594c935a9, 0x0000000000000000, 0x0000000000000000, true >;  // CRC-64/REDIS
	using crc64    = ecma_182;
} // namespace crc64

#endif // PARAMETRIC_CRC_H
