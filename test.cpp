// SPDX-License-Identifier: MIT-0
// SPDX-FileCopyrightText:  2024 Istvan Pasztor
//
// This basic test compares the output of the library to the
// "check" and "residue" values provided in the CRC catalogue:
// https://reveng.sourceforge.io/crc-catalogue/all.htm

// Allowing the use of unsafe functions like strcpy() in Visual C++.
#define _CRT_SECURE_NO_WARNINGS

//#define PARAMETRIC_CRC_REVERSE_BITS_NIBBLE_LOOKUP_TABLE
//#define PARAMETRIC_CRC_NO_REVERSE_BITS_LOOKUP_TABLE
//#define PARAMETRIC_CRC_SIMPLE_TABLE_GENERATOR
#include "parametric_crc.h"

#include <stdio.h>
#include <stdint.h>    // uint64_t
#include <inttypes.h>  // PRIx64 macro
#include <string.h>

#define TEST_CONSTEXPR 1  // 0 makes it easier to debug the tests

#if TEST_CONSTEXPR
#  define DEBUG_PRINTF(fmt, ...)
#  define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#  define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#  define STATIC_ASSERT(cond, msg)
#endif

// This template function will be instantiated for each subtype (tableless,
// table_based, ext_table_based, etc...) of every tested CRC algorithm twice:
// with and without a reflected CRC shift register.
template <typename CRC>
int run_one(const char* name, uint64_t check_value, uint64_t residue_const) {
	static constexpr int NAME_W = 55;
	// two hex nibbles per byte
	static constexpr int CRC_W = sizeof(typename CRC::value_type) * 2;

	// CRC calculation

	CRC crc_obj;
	crc_obj.update("123456789", 9);
	auto crc_val = crc_obj.final();
	if (crc_val != (typename CRC::value_type)check_value) {
		printf("%-*s crc=%0*" PRIx64 " expected(crc)=%0*" PRIx64 " fail\n",
			NAME_W, name, CRC_W, (uint64_t)crc_val, CRC_W, (uint64_t)check_value);
		return 1;
	}

	// The residue constant that was calculated by the library at compile time

	if (CRC::RESIDUE != residue_const) {
		printf("%-*s CRC::RESIDUE=%0*" PRIx64 " expected(residue_const)=%0*" PRIx64 " fail\n",
			NAME_W, name, CRC_W, (uint64_t)CRC::RESIDUE, CRC_W, (uint64_t)residue_const);
		return 1;
	}

	// The residue() method should return the value of the residue constant
	// after feeding a valid codeword into a CRC calculator.

	// The line below works on Linux and Windows but on MacOS it generates an
	// abort after a failed stack check so I replaced it to an array + strcpy.
	//char codeword[9 + sizeof(typename CRC::value_type)] = "123456789";

	char codeword[9 + sizeof(typename CRC::value_type)];
	strcpy(codeword, "123456789");
	size_t size = 9;

	if (CRC::REF_IN != CRC::REF_OUT)
		crc_val = crc::reverse_bits(crc_val);

	if (CRC::REF_IN) {
		// LSB: append crc_val in little endian format
		for (size_t i=0; i<sizeof(typename CRC::value_type); i++)
			codeword[size++] = char(crc_val >> (i*8));
	}
	else {
		// MSB: append crc_val in big endian format
		for (int i=int(sizeof(typename CRC::value_type))-1; i>=0; i--)
			codeword[size++] = char(crc_val >> (i*8));
	}

	CRC crc_obj_2;
	crc_obj_2.update(codeword, size);
	auto rc = crc_obj_2.residue();
	if (rc != residue_const) {
		printf("%-*s residue=%0*" PRIx64 " expected(residue_const)=%0*" PRIx64 " fail\n",
			NAME_W, name, CRC_W, (uint64_t)rc, CRC_W, (uint64_t)residue_const);
		return 1;
	}

	printf("%-*s crc=%0*" PRIx64 " residue=%0*" PRIx64 " pass\n",
		NAME_W, name, CRC_W, (uint64_t)crc_val, CRC_W, (uint64_t)rc);
	return 0;
}

// This template pretends to be a CRC implementation that doesn't require
// a table parameter in its update() method but internally its a CRC
// implementation with an external table.
template <typename ext_table_based>
struct etb {
	using T = typename ext_table_based::value_type;
	using value_type = T;
	static constexpr T RESIDUE = ext_table_based::RESIDUE;

	static constexpr int WIDTH = ext_table_based::WIDTH;
	static constexpr T POLY = ext_table_based::POLY;
	static constexpr T INIT = ext_table_based::INIT;
	static constexpr T XOR_OUT = ext_table_based::XOR_OUT;
	static constexpr bool REF_IN = ext_table_based::REF_IN;
	static constexpr bool REF_OUT = ext_table_based::REF_OUT;

	typename ext_table_based::table_type table;
	ext_table_based crc;

	void update(const void* data, size_t size) {
		crc.update(data, size, table);
	}
	T final() const {
		return crc.final();
	}
	T residue() const {
		return crc.residue();
	}
};

// Returns the number of errors.
template <typename CRC, bool REFLECTED_CRC_REGISTER>
int test_modes(const char* name, uint64_t check_value, uint64_t residue_const) {
	using crc_t = crc::parametric<CRC::WIDTH, CRC::POLY, CRC::INIT, CRC::XOR_OUT,
		CRC::REF_IN, CRC::REF_OUT, REFLECTED_CRC_REGISTER>;

	int errors = 0;
	char new_name[0x80];

	sprintf(new_name, "%s::%s", name, "tableless");
	errors += run_one<typename crc_t::tableless>(new_name, check_value, residue_const);

	sprintf(new_name, "%s::%s", name, "table_based");
	errors += run_one<typename crc_t::table_based>(new_name, check_value, residue_const);

	sprintf(new_name, "%s::%s", name, "small_table_based");
	errors += run_one<typename crc_t::small_table_based>(new_name, check_value, residue_const);

	sprintf(new_name, "%s::%s", name, "ext_table_based");
	errors += run_one<etb<typename crc_t::ext_table_based>>(new_name, check_value, residue_const);

	sprintf(new_name, "%s::%s", name, "ext_small_table_based");
	errors += run_one<etb<typename crc_t::ext_small_table_based>>(new_name, check_value, residue_const);

	return errors;
}

template <typename CRC>
constexpr bool test_constexpr(uint64_t check_value) {
	constexpr uint8_t CHECK_DATA[] = "123456789";

	// static CRC::calculate() method
	{
		constexpr uint8_t STR[] = "123456789";
		constexpr auto v = CRC::calculate(STR, 9);
		if (v != check_value) {
			DEBUG_PRINTF("calculate(): output=%" PRIx64 " expected=%" PRIx64 "\n",
				(uint64_t)v, (uint64_t)check_value);
			return false;
		}
	}

	// Multiple update() calls on a CRC instance
	{
		constexpr uint8_t STR1[] = "12345";
		constexpr uint8_t STR2[] = "6789";
		CRC crc;
		crc.update(STR1, 5);
		crc.update(STR2, 4);
		auto v = crc.final();
		if (v != check_value) {
			DEBUG_PRINTF("update() calls: output=%" PRIx64 " expected=%" PRIx64 "\n",
				(uint64_t)v, (uint64_t)check_value);
			return false;
		}
	}

	// Multiple update() calls with small external table
	{
		constexpr uint8_t STR1[] = "12345";
		constexpr uint8_t STR2[] = "6789";
		typename CRC::ext_small_table_based::table_type table;
		typename CRC::ext_small_table_based crc;
		crc.update(STR1, 5, table);
		crc.update(STR2, 4, table);
		auto v = crc.final();
		if (v != check_value) {
			DEBUG_PRINTF("small_table update() calls: output=%" PRIx64 " expected=%" PRIx64 "\n",
				(uint64_t)v, (uint64_t)check_value);
			return false;
		}
	}

	return true;
}

int main() {
	int errors = 0;

	// This macro tests a CRC algorithm with both an unreflected and a reflected CRC register:
	#define TEST_CRC(name, check_value, residue_const) \
		errors += test_modes<name,false>("        " #name, check_value, residue_const); \
		errors += test_modes<name,true >("ref_reg " #name, check_value, residue_const); \
		STATIC_ASSERT(test_constexpr<name>(check_value), #name " test_constexpr"); \
		if (!test_constexpr<name>(check_value)) \
			{ errors++; printf("test_constexpr<%s>() returned false\n", #name); }

	TEST_CRC(crc8::rohc, 0xd0, 0x00);
	TEST_CRC(crc8::i_432_1, 0xa1, 0xac);
	TEST_CRC(crc8::smbus, 0xf4, 0x00);
	TEST_CRC(crc8::tech_3250, 0x97, 0x00);
	TEST_CRC(crc8::gsm_a, 0x37, 0x00);
	TEST_CRC(crc8::mifare_mad, 0x99, 0x00);
	TEST_CRC(crc8::i_code, 0x7e, 0x00);
	TEST_CRC(crc8::hitag, 0xb4, 0x00);
	TEST_CRC(crc8::sae_j1850, 0x4b, 0xc4);
	TEST_CRC(crc8::opensafety, 0x3e, 0x00);
	TEST_CRC(crc8::autosar, 0xdf, 0x42);
	TEST_CRC(crc8::maxim_dow, 0xa1, 0x00);
	TEST_CRC(crc8::nrsc_5, 0xf7, 0x00);
	TEST_CRC(crc8::darc, 0x15, 0x00);
	TEST_CRC(crc8::gsm_b, 0x94, 0x53);
	TEST_CRC(crc8::wcdma, 0x25, 0x00);
	TEST_CRC(crc8::lte, 0xea, 0x00);
	TEST_CRC(crc8::cdma2000, 0xda, 0x00);
	TEST_CRC(crc8::bluetooth, 0x26, 0x00);
	TEST_CRC(crc8::dvb_s2, 0xbc, 0x00);

	TEST_CRC(crc16::dect_x, 0x007f, 0x0000);
	TEST_CRC(crc16::dect_r, 0x007e, 0x0589);
	TEST_CRC(crc16::nrsc_5, 0xa066, 0x0000);
	TEST_CRC(crc16::dnp, 0xea82, 0x66c5);
	TEST_CRC(crc16::en_13757, 0xc2b7, 0xa366);
	TEST_CRC(crc16::kermit, 0x2189, 0x0000);
	TEST_CRC(crc16::tms37157, 0x26b1, 0x0000);
	TEST_CRC(crc16::riello, 0x63d0, 0x0000);
	TEST_CRC(crc16::a, 0xbf05, 0x0000);
	TEST_CRC(crc16::mcrf4xx, 0x6f91, 0x0000);
	TEST_CRC(crc16::ibm_sdlc, 0x906e, 0xf0b8);
	TEST_CRC(crc16::xmodem, 0x31c3, 0x0000);
	TEST_CRC(crc16::gsm, 0xce3c, 0x1d0f);
	TEST_CRC(crc16::spi_fujitsu, 0xe5cc, 0x0000);
	TEST_CRC(crc16::ibm_3740, 0x29b1, 0x0000);
	TEST_CRC(crc16::genibus, 0xd64e, 0x1d0f);
	TEST_CRC(crc16::profibus, 0xa819, 0xe394);
	TEST_CRC(crc16::opensafety_a, 0x5d38, 0x0000);
	TEST_CRC(crc16::m17, 0x772b, 0x0000);
	TEST_CRC(crc16::lj1200, 0xbdf4, 0x0000);
	TEST_CRC(crc16::opensafety_b, 0x20fe, 0x0000);
	TEST_CRC(crc16::arc, 0xbb3d, 0x0000);
	TEST_CRC(crc16::maxim_dow, 0x44c2, 0xb001);
	TEST_CRC(crc16::modbus, 0x4b37, 0x0000);
	TEST_CRC(crc16::usb, 0xb4c8, 0xb001);
	TEST_CRC(crc16::umts, 0xfee8, 0x0000);
	TEST_CRC(crc16::dds_110, 0x9ecf, 0x0000);
	TEST_CRC(crc16::cms, 0xaee7, 0x0000);
	TEST_CRC(crc16::t10_dif, 0xd0db, 0x0000);
	TEST_CRC(crc16::teledisk, 0x0fb3, 0x0000);
	TEST_CRC(crc16::cdma2000, 0x4c06, 0x0000);

	TEST_CRC(crc32::xfer, 0xbd0be338, 0x00000000);
	TEST_CRC(crc32::jamcrc, 0x340bc6d9, 0x00000000);
	TEST_CRC(crc32::iso_hdlc, 0xcbf43926, 0xdebb20e3);
	TEST_CRC(crc32::cksum, 0x765e7680, 0xc704dd7b);
	TEST_CRC(crc32::mpeg2, 0x0376e6e7, 0x00000000);
	TEST_CRC(crc32::bzip2, 0xfc891918, 0xc704dd7b);
	TEST_CRC(crc32::iscsi, 0xe3069283, 0xb798b438);
	TEST_CRC(crc32::mef, 0xd2c22f51, 0x00000000);
	TEST_CRC(crc32::cd_rom_edc, 0x6ec2edc4, 0x00000000);
	TEST_CRC(crc32::aixm, 0x3010bf7f, 0x00000000);
	TEST_CRC(crc32::base91_d, 0x87315576, 0x45270551);
	TEST_CRC(crc32::autosar, 0x1697d06a, 0x904cddbf);

	TEST_CRC(crc64::go_iso, 0xb90956c775a41001, 0x5300000000000000);
	TEST_CRC(crc64::ms, 0x75d4b74f024eceea, 0x0000000000000000);
	TEST_CRC(crc64::xz, 0x995dc9bbdf1939fa, 0x49958c9abd7d353f);
	TEST_CRC(crc64::ecma_182, 0x6c40df5f0b497347, 0x0000000000000000);
	TEST_CRC(crc64::we, 0x62ec59e3f1a4f00a, 0xfcacbebd5931a992);
	TEST_CRC(crc64::redis, 0xe9c6d914c4b8d9ca, 0x0000000000000000);

	#undef TEST_CRC

	if (errors) {
		printf("Number of failed tests: %d\n", errors);
		return 1;
	}
	printf("All tests passed.\n");
	return 0;
}
