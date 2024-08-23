/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#if __cpp_exceptions

#include <cstdint>
#include <functional>

#include "gpio_cxx.hpp"

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


class GPIOInterruptService
{
public:
    static GPIOInterruptService& Get();
    static void ServiceCheck();

    ~GPIOInterruptService() { Stop(); }

    GPIOInterruptService(const GPIOInterruptService&) = delete;
    GPIOInterruptService(GPIOInterruptService&&) = delete;
    GPIOInterruptService& operator=(const GPIOInterruptService&) = delete;
    GPIOInterruptService& operator=(GPIOInterruptService&&) = delete;

    void Start(InterruptFlags aFlags);
    void Stop();

protected:
    bool mStarted = false;
    int mFlags = 0;

    GPIOInterruptService() = default;
};


class GPIOInterrupt : public GPIOBase
{
public:
    /**
     * @brief Callback footprint declaration for the GPIO interrupt.
     */
    typedef std::function<void(const GPIOInterrupt&)> interrupt_callback_t;

    using GPIOBase::GPIOBase;

    /**
     * @brief Constructor of GPIOInterrupt
     * 
     * @param num The GPIO on which to set the interrupt
     * @param mode The IO mode of the GPIO
     * @param pull The pull mode of the GPIO
     * @param strength The drive strength of the GPIO
     * @param type The interrupt type to set
     * @param cb The callback to be triggered on interrupt conditions
     */
    GPIOInterrupt(GPIONum num, GPIOModeType mode, GPIOPullMode pull, GPIODriveStrength strength, GPIOInterruptType type, interrupt_callback_t cb);

    ~GPIOInterrupt();

    using GPIOBase::set_pull_mode;
    using GPIOBase::set_floating;
    using GPIOBase::set_low;
    using GPIOBase::set_drive_strength;
    using GPIOBase::get_drive_strength;

    /**
     * @brief Set the interrupt type on the GPIO input
     * 
     * @note calling this function will overwrite the previously
     * set interrupt type if any.
     * 
     * @param type the interrupt type to be set
     */
    void set_type(GPIOInterruptType type);

    /**
     * @brief Add a user callback in the list of registered callbacks
     * 
     * @note If the user callback is empty when this method is
     * called, this method also register the driver_handler callback to
     * the GPIO driver
     * 
     * @param func_cb The user callback
     */
    void set_callback(interrupt_callback_t func_cb);

    /**
     * @brief Remove the registered callback
     * 
     * @note If no callback is found, this method as no effect.
     * The method also unregister the driver_handler callback
     * to the GPIO driver.
     */
    void remove_callback();

    /**
     * @brief Enable the interrupts on the GPIO
     */
    void enable() const;

    /**
     * @brief Disable interrupts on the GPIO
     */
    void disable() const;

private:
    interrupt_callback_t mCallback;

    /**
     * @brief Function registered and called from the GPIO driver
     * on interrupt with the pointer to the appropriate instance
     * of GPIOInterrupt passed as parameter.
     * 
     * @note This function casts the void* class_ptr to GPIOInterrupt* and
     * invokes the mCallback if assigned.
     * 
     * @param class_ptr The pointer to the instance of GPIOInterrupt
     */
    static void driver_handler(void* class_ptr);
};

} // namespace idf

#endif // __cpp_exceptions