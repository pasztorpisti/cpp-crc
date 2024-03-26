Parametric CRC-8 / CRC-16 / CRC-32 / CRC-64
===========================================
Single-header C++ library (-std=c++14)

Usage:

```cpp
#include "parametric_crc_mini.h"

void example() {
    // Find the CRC algorithm you need in the crc8, crc16, crc32 or crc64
    // namespace (at the bottom of the header file) and use it like this:

    uint16_t crc_xmodem = crc16::xmodem::calculate("123456789", 9);
    printf("CRC-16/XMODEM check=0x%04x\n", crc_xmodem);

    crc16::xmodem crc_obj;
    crc_obj.update("12345", 5);
    crc_obj.update("6789", 4);
    printf("CRC-16/XMODEM check=0x%04x\n", crc_obj.final());

    // Custom polynomial:
    // 0xa2eb was picked from the CRC Polynomial Zoo:
    // https://users.ece.cmu.edu/~koopman/crc/crc16.html

    using zoo_a2eb = crc::parametric<16, 0xa2eb, 0xffff, 0xffff, true>;
    uint16_t crc_a2eb = zoo_a2eb::calculate("123456789", 9);
    printf("CRC-16/ZOO-A2EB-FF-LSB: check=0x%04x\n", crc_a2eb);
}
```

The "mini" version of the library provides only (constexpr) table-based CRC
calculation which is what the average application needs. The "large" version
is more like a CRC playground with features that no one asked for.
