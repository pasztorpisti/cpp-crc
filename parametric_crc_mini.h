// SPDX-License-Identifier: MIT-0
// SPDX-FileCopyrightText:  2024 Istvan Pasztor
//
// Parametric CRC-8 / CRC-16 / CRC-32 / CRC-64
// Single-header C++ library (-std=c++14)
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
// The tables of the used CRC algorithms are generated at compile time.

#ifndef PARAMETRIC_CRC_MINI_H
#define PARAMETRIC_CRC_MINI_H

#ifndef PARAMETRIC_CRC_NO_STDINT_H
#include <stdint.h>  // not needed if you define uint16_t, uint32_t and uint64_t
#endif

namespace crc {

	using uint8_t = unsigned char;
	using size_t = decltype(sizeof(0));

	template <typename T>
	struct static_table {
		static constexpr T instance = T();
	};
	template <typename T>
	constexpr T static_table<T>::instance;

	struct reverse_bits_lookup_table {
		uint8_t entries[0x100];
		constexpr reverse_bits_lookup_table() noexcept : entries() {
			for (uint16_t i=0; i<0x100; i++) {
				uint8_t v = uint8_t(i);
				v = ((v >> 1) & 0x55) | ((v & 0x55) << 1);
				v = ((v >> 2) & 0x33) | ((v & 0x33) << 2);
				entries[i] = (v >> 4) | (v << 4);
			}
		}
	};

	constexpr uint8_t reverse_bits(uint8_t v) noexcept {
		return static_table<reverse_bits_lookup_table>::instance.entries[v];
	}
	constexpr uint16_t reverse_bits(uint16_t v) noexcept {
		return (uint16_t(reverse_bits(uint8_t(v))) << 8) | reverse_bits(uint8_t(v >> 8));
	}
	constexpr uint32_t reverse_bits(uint32_t v) noexcept {
		return (uint32_t(reverse_bits(uint16_t(v))) << 16) | reverse_bits(uint16_t(v >> 16));
	}
	constexpr uint64_t reverse_bits(uint64_t v) noexcept {
		return (uint64_t(reverse_bits(uint32_t(v))) << 32) | reverse_bits(uint32_t(v >> 32));
	}

	template <typename T, T REF_POLY, bool REF>
	struct lookup_table {
		T entries[0x100];
		constexpr lookup_table() noexcept : entries() {
			for (uint16_t index=0; index<0x100; index++) {
				T entry = T(REF ? index : reverse_bits(uint8_t(index)));
				for (uint8_t i=0; i<8; i++)
					entry = (entry & 1) ? ((entry >> 1) ^ REF_POLY) : (entry >> 1);
				entries[index] = REF ? entry : reverse_bits(entry);
			}
		}
	};

	template <typename T, T POLY, T INIT, T XOR_OUT, bool REF_IN, bool REF_OUT=REF_IN>
	struct base;

	template <typename T, T POLY, T INIT, T XOR_OUT, bool REF_OUT>
	struct base<T, POLY, INIT, XOR_OUT, false, REF_OUT> { // REF_IN=false: unreflected CRC register
		static constexpr T calculate(const uint8_t* begin, const uint8_t* end, bool interim=false, T crc=INIT) noexcept {
			constexpr auto& tbl = static_table<lookup_table<T, reverse_bits(POLY), false>>::instance.entries;
			for (auto p=begin; p<end; p++) // (crc << 8) would emit compiler warnings with T=uint8_t
				crc = tbl[(crc >> (sizeof(T)*8-8)) ^ *p] ^ (crc << 4 << 4);
			return interim ? crc : ((REF_OUT ? reverse_bits(crc) : crc) ^ XOR_OUT);
		}
	};

	template <typename T, T POLY, T INIT, T XOR_OUT, bool REF_OUT>
	struct base<T, POLY, INIT, XOR_OUT, true, REF_OUT> { // REF_IN=true: reflected CRC register
		static constexpr T calculate(const uint8_t* begin, const uint8_t* end, bool interim=false, T crc=reverse_bits(INIT)) noexcept {
			constexpr auto& tbl = static_table<lookup_table<T, reverse_bits(POLY), true>>::instance.entries;
			for (auto p=begin; p<end; p++) // (crc >> 8) would emit compiler warnings with T=uint8_t
				crc = tbl[uint8_t(crc) ^ *p] ^ (crc >> 4 >> 4);
			return interim ? crc : ((REF_OUT ? crc : reverse_bits(crc)) ^ XOR_OUT);
		}
	};

	template<int WIDTH> struct _uint;
	template<> struct _uint <8> { typedef uint8_t  type; };
	template<> struct _uint<16> { typedef uint16_t type; };
	template<> struct _uint<32> { typedef uint32_t type; };
	template<> struct _uint<64> { typedef uint64_t type; };
	template<int WIDTH> using uint = typename _uint<WIDTH>::type;

	template <int W, uint<W> POLY, uint<W> INIT, uint<W> XOR_OUT, bool REF_IN_, bool REF_OUT_=REF_IN_>
	class parametric : public base<uint<W>, POLY, INIT, XOR_OUT, REF_IN_, REF_OUT_> {
		using T = uint<W>;
		T _crc;
	public:
		using base<T, POLY, INIT, XOR_OUT, REF_IN_, REF_OUT_>::calculate;
		static constexpr T calculate(const uint8_t* data, size_t size) noexcept  { return calculate(data, data+size); }
		static constexpr T calculate(const void* data, size_t size) noexcept     { return calculate((const uint8_t*)data, size); }

		constexpr parametric(T interim_remainder=REF_IN?reverse_bits(INIT):INIT) noexcept : _crc(interim_remainder) {}
		constexpr void update(const uint8_t* begin, const uint8_t* end) noexcept { _crc = calculate(begin, end, true, _crc); }
		constexpr void update(const uint8_t* data, size_t size) noexcept         { _crc = calculate(data, data+size, true, _crc); }
		constexpr void update(const void* data, size_t size) noexcept            { update((const uint8_t*)data, size); }
		constexpr T interim() const noexcept { return _crc; }
		constexpr T residue() const noexcept { return (REF_IN!=REF_OUT) ? reverse_bits(_crc) : _crc; }
		constexpr T final()   const noexcept { return residue() ^ XOR_OUT; }

		using value_type = T;
		static constexpr bool REF_IN = REF_IN_;   // determines the CRC bit/byte order
		static constexpr bool REF_OUT = REF_OUT_; // determines the CRC bit/byte order
		static constexpr auto& table() noexcept { return static_table<lookup_table<T, reverse_bits(POLY), REF_IN_>>::instance.entries; }
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

#endif // PARAMETRIC_CRC_MINI_H
