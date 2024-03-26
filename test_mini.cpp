// SPDX-License-Identifier: MIT-0
// SPDX-FileCopyrightText:  2023 Istvan Pasztor
//
// This basic test compares the output of the library to the
// "check" and "residue" values provided in the CRC catalogue:
// https://reveng.sourceforge.io/crc-catalogue/all.htm

#include "parametric_crc_mini.h"
#include <stdio.h>
#include <stdint.h>    // uint64_t
#include <inttypes.h>  // PRIx64 macro

#define TEST_CONSTEXPR 1  // 0 makes it easier to debug the tests

#if TEST_CONSTEXPR
#  define DEBUG_PRINTF(fmt, ...)
#  define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#  define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#  define STATIC_ASSERT(cond, msg)
#endif

// conversion between little/big endian values
constexpr uint8_t reverse_bytes(uint8_t v) noexcept {
	return v;
}
constexpr uint16_t reverse_bytes(uint16_t v) noexcept {
	return (v >> 8) | (v << 8);
}
constexpr uint32_t reverse_bytes(uint32_t v) noexcept {
	return (uint32_t(reverse_bytes(uint16_t(v))) << 16) | reverse_bytes(uint16_t(v >> 16));
}
constexpr uint64_t reverse_bytes(uint64_t v) noexcept {
	return (uint64_t(reverse_bytes(uint32_t(v))) << 32) | reverse_bytes(uint32_t(v >> 32));
}

template <typename CRC>
constexpr bool test_one(typename CRC::value_type check_value, typename CRC::value_type residue_const) {

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

	// Checking the residue left by a valid codeword
	{
		auto cv_rb = CRC::REF_IN != CRC::REF_OUT ? crc::reverse_bits(check_value) : check_value;
		uint64_t cv64 = !CRC::REF_IN ? reverse_bytes(cv_rb) : cv_rb;
		uint8_t codeword[] {
			0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, // "123456789"
			uint8_t(cv64), uint8_t(cv64>>8), uint8_t(cv64>>16), uint8_t(cv64>>24),
			uint8_t(cv64>>32), uint8_t(cv64>>40), uint8_t(cv64>>48), uint8_t(cv64>>56),
		};
		CRC crc;
		crc.update(codeword, 9 + sizeof(typename CRC::value_type));
		auto v = crc.residue();
		if (v != residue_const) {
			DEBUG_PRINTF("residue(): output=%" PRIx64 " expected=%" PRIx64 "\n",
				(uint64_t)v, (uint64_t)residue_const);
			return false;
		}
	}
	return true;
}

int main() {
	int errors = 0;

	#define TEST_CRC(name, check_value, residue_const) \
		STATIC_ASSERT(test_one<name>(check_value, residue_const), #name " failed"); \
		if (test_one<name>(check_value, residue_const)) { \
			printf("%-25s pass\n", #name); \
		} else { \
			printf("%-25s fail\n", #name); \
			errors++; \
		}

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
