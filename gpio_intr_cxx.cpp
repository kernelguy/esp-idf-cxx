/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "gpio_intr_cxx.hpp"
#include "driver/gpio.h"

namespace idf {

GPIOInterruptService& GPIOInterruptService::Get()
{
    static GPIOInterruptService instance;
    return instance;
}

void GPIOInterruptService::ServiceCheck()
{
    if (!GPIOInterruptService::Get().mStarted) {
        throw ESPException(ESP_ERR_NOT_FOUND);
    }
}

void GPIOInterruptService::Start(InterruptFlags aFlags)
{
    GPIO_CHECK_THROW(gpio_install_isr_service(aFlags.GetValue()));
    mStarted = true;
}

void GPIOInterruptService::Stop()
{
    gpio_uninstall_isr_service();
    mStarted = false;
}

GPIOInterrupt::GPIOInterrupt(GPIONum num, GPIOModeType mode, GPIOPullMode pull, GPIODriveStrength strength, GPIOInterruptType type, GPIOInterrupt::interrupt_callback_t cb)
    : GPIOBase(num, mode, pull, strength)
{
    set_type(type);
    set_callback(std::move(cb));
}

GPIOInterrupt::~GPIOInterrupt()
{
    remove_callback();
}

void GPIOInterrupt::set_type(const GPIOInterruptType type)
{
    GPIO_CHECK_THROW(gpio_set_intr_type(gpio_num.get_value<gpio_num_t>(),
                                        type.get_value<gpio_int_type_t>()));
}

void GPIOInterrupt::set_callback(interrupt_callback_t aCb)
{
    if (!mCallback)
    {
        GPIOInterruptService::ServiceCheck();
        GPIO_CHECK_THROW(gpio_isr_handler_add(gpio_num.get_value<gpio_num_t>(),
                                        (gpio_isr_t)(&GPIOInterrupt::driver_handler),
                                        this));
    }
    mCallback = std::move(aCb);
}

void GPIOInterrupt::remove_callback()
{
    if (mCallback) {
        mCallback = nullptr;
        GPIOInterruptService::ServiceCheck();
        GPIO_CHECK_THROW(gpio_isr_handler_remove(gpio_num.get_value<gpio_num_t>()));
    }
}

void GPIOInterrupt::enable() const
{
    GPIO_CHECK_THROW(gpio_intr_enable(gpio_num.get_value<gpio_num_t>()));
}

void GPIOInterrupt::disable() const
{
    GPIO_CHECK_THROW(gpio_intr_disable(gpio_num.get_value<gpio_num_t>()));
}

void GPIOInterrupt::driver_handler(void* class_ptr)
{
    auto p = reinterpret_cast<GPIOInterrupt*>(class_ptr);
    if (p && p->mCallback) {
        p->mCallback(*p);
    }
}

}  // namespace idf