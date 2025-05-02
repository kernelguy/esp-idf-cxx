/**
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.
*
* \copyright   Copyright 2025 RSP Systems A/S. All rights reserved.
* \license     Mozilla Public License 2.0
* \author      steffen
*/
#ifndef SIMPLE_SPI_RW_EXAMPLE_INCLUDE_INTERRUPT_FLAGS_H
#define SIMPLE_SPI_RW_EXAMPLE_INCLUDE_INTERRUPT_FLAGS_H

#include <cstdint>

namespace idf {

class InterruptFlags
{
public:
    constexpr InterruptFlags() = default;

    constexpr InterruptFlags& Level1()      { mFlags |= (1<<1); return *this; }
    constexpr InterruptFlags& Level2()      { mFlags |= (1<<2); return *this; }
    constexpr InterruptFlags& Level3()      { mFlags |= (1<<3); return *this; }
    constexpr InterruptFlags& Level4()      { mFlags |= (1<<4); return *this; }
    constexpr InterruptFlags& Level5()      { mFlags |= (1<<5); return *this; }
    constexpr InterruptFlags& Level6()      { mFlags |= (1<<6); return *this; }
    constexpr InterruptFlags& NonMaskable() { mFlags |= (1<<7); return *this; }
    constexpr InterruptFlags& Shared()      { mFlags |= (1<<8); return *this; }
    constexpr InterruptFlags& Edge()        { mFlags |= (1<<9); return *this; }
    constexpr InterruptFlags& InternalRAM() { mFlags |= (1<<10); return *this; }
    constexpr InterruptFlags& Disabled()    { mFlags |= (1<<11); return *this; }

    [[nodiscard]] constexpr uint32_t GetValue() const { return mFlags; }
protected:
    uint32_t mFlags = 0;
};

} // namespace idf

#endif //SIMPLE_SPI_RW_EXAMPLE_INCLUDE_INTERRUPT_FLAGS_H
