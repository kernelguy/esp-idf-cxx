/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if __cpp_exceptions

#include <array>
#include "driver/gpio.h"
#include "gpio_cxx.hpp"

namespace idf {

namespace {
#if CONFIG_IDF_TARGET_LINUX
constexpr std::array<uint32_t, 1> INVALID_GPIOS = {24};
#elif CONFIG_IDF_TARGET_ESP32
constexpr std::array<uint32_t, 1> INVALID_GPIOS = {24};
#elif CONFIG_IDF_TARGET_ESP32S2
constexpr std::array<uint32_t, 4> INVALID_GPIOS = {22, 23, 24, 25};
#elif CONFIG_IDF_TARGET_ESP32S3
constexpr std::array<uint32_t, 4> INVALID_GPIOS = {22, 23, 24, 25};
#elif CONFIG_IDF_TARGET_ESP32C3
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#elif CONFIG_IDF_TARGET_ESP32C2
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#elif CONFIG_IDF_TARGET_ESP32C6
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#elif CONFIG_IDF_TARGET_ESP32H2
constexpr std::array<uint32_t, 0> INVALID_GPIOS = {};
#else
#error "No GPIOs defined for the current target"
#endif

}

GPIOException::GPIOException(esp_err_t error) : ESPException(error) { }

esp_err_t check_gpio_pin_num(uint32_t pin_num) noexcept
{
    if (pin_num >= GPIO_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    for (auto num: INVALID_GPIOS)
    {
        if (pin_num == num) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    return ESP_OK;
}

esp_err_t check_gpio_drive_strength(uint32_t strength) noexcept
{
    if (strength >= GPIO_DRIVE_CAP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t check_gpio_mode(uint32_t mode) noexcept
{
    if ((mode & ~GPIO_MODE_INPUT_OUTPUT_OD) != 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

GPIOPullMode GPIOPullMode::FLOATING()
{
    return GPIOPullMode(GPIO_FLOATING);
}

GPIOPullMode GPIOPullMode::PULLUP()
{
    return GPIOPullMode(GPIO_PULLUP_ONLY);
}

GPIOPullMode GPIOPullMode::PULLDOWN()
{
    return GPIOPullMode(GPIO_PULLDOWN_ONLY);
}

GPIOInterruptType GPIOInterruptType::DISABLE()
{
    return GPIOInterruptType(GPIO_INTR_DISABLE);
}

GPIOInterruptType GPIOInterruptType::POSITIVE_EDGE()
{
    return GPIOInterruptType(GPIO_INTR_POSEDGE);
}

GPIOInterruptType GPIOInterruptType::NEGATIVE_EDGE()
{
    return GPIOInterruptType(GPIO_INTR_NEGEDGE);
}

GPIOInterruptType GPIOInterruptType::ANY_EDGE()
{
    return GPIOInterruptType(GPIO_INTR_ANYEDGE);
}

GPIOInterruptType GPIOInterruptType::LOW_LEVEL()
{
    return GPIOInterruptType(GPIO_INTR_LOW_LEVEL);
}

GPIOInterruptType GPIOInterruptType::HIGH_LEVEL()
{
    return GPIOInterruptType(GPIO_INTR_HIGH_LEVEL);
}

GPIODriveStrength GPIODriveStrength::DEFAULT()
{
    return MEDIUM();
}

GPIODriveStrength GPIODriveStrength::WEAK()
{
    return GPIODriveStrength(GPIO_DRIVE_CAP_0);
}

GPIODriveStrength GPIODriveStrength::LESS_WEAK()
{
    return GPIODriveStrength(GPIO_DRIVE_CAP_1);
}

GPIODriveStrength GPIODriveStrength::MEDIUM()
{
    return GPIODriveStrength(GPIO_DRIVE_CAP_2);
}

GPIODriveStrength GPIODriveStrength::STRONGEST()
{
    return GPIODriveStrength(GPIO_DRIVE_CAP_3);
}

GPIOModeType GPIOModeType::DISABLE()
{
    return GPIOModeType(GPIO_MODE_DISABLE);
}

GPIOModeType GPIOModeType::INPUT()
{
    return GPIOModeType(GPIO_MODE_INPUT);
}

GPIOModeType GPIOModeType::OUTPUT()
{
    return GPIOModeType(GPIO_MODE_OUTPUT);
}

GPIOModeType GPIOModeType::OUTPUT_OPEN_DRAIN()
{
    return GPIOModeType(GPIO_MODE_OUTPUT_OD);
}

GPIOModeType GPIOModeType::INPUT_OUTPUT_OPEN_DRAIN()
{
    return GPIOModeType(GPIO_MODE_INPUT_OUTPUT_OD);
}

GPIOModeType GPIOModeType::INPUT_OUTPUT()
{
    return GPIOModeType(GPIO_MODE_INPUT_OUTPUT);
}

GPIOBase::GPIOBase(GPIONum num, GPIOModeType mode)
    : gpio_num(num)
{
    gpio_config_t cfg = {
            .pin_bit_mask = gpio_num.get_value<uint64_t>(),
            .mode = mode.get_value<gpio_mode_t>(),
            // For safety reasons do not pull in any direction!!!
            .pull_up_en = gpio_pullup_t::GPIO_PULLUP_DISABLE,
            .pull_down_en = gpio_pulldown_t::GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
    };
    GPIO_CHECK_THROW(gpio_config(&cfg));
}

GPIOBase::GPIOBase(GPIONum num, GPIOModeType mode, GPIOPullMode pull, GPIODriveStrength strength)
    : GPIOBase(num, mode)
{
    set_pull_mode(pull);
    set_drive_strength(strength);
}

void GPIOBase::hold_en()
{
    GPIO_CHECK_THROW(gpio_hold_en(gpio_num.get_value<gpio_num_t>()));
}

void GPIOBase::hold_dis()
{
    GPIO_CHECK_THROW(gpio_hold_dis(gpio_num.get_value<gpio_num_t>()));
}

void GPIOBase::set(bool aValue) const
{
    GPIO_CHECK_THROW(gpio_set_level(gpio_num.get_value<gpio_num_t>(), aValue ? 1 : 0));
}

void GPIOBase::set_pull_mode(GPIOPullMode mode) const
{
    GPIO_CHECK_THROW(gpio_set_pull_mode(gpio_num.get_value<gpio_num_t>(),
                                        mode.get_value<gpio_pull_mode_t>()));
}

GPIOLevel GPIOBase::get_level() const noexcept
{
    if (bool(*this)) {
        return GPIOLevel::HIGH;
    } else {
        return GPIOLevel::LOW;
    }
}

GPIOBase::operator bool() const
{
    int level = gpio_get_level(gpio_num.get_value<gpio_num_t>());
    return (level != 0);
}

void GPIOBase::set_drive_strength(GPIODriveStrength strength)
{
    GPIO_CHECK_THROW(gpio_set_drive_capability(gpio_num.get_value<gpio_num_t>(),
                                               strength.get_value<gpio_drive_cap_t>()));
}

GPIODriveStrength GPIOBase::get_drive_strength()
{
    gpio_drive_cap_t strength;
    GPIO_CHECK_THROW(gpio_get_drive_capability(gpio_num.get_value<gpio_num_t>(), &strength));
    return GPIODriveStrength(static_cast<uint32_t>(strength));
}

GPIO_Output::GPIO_Output(GPIONum num)
    : GPIOBase(num, GPIOModeType::OUTPUT())
{
}

GPIOInput::GPIOInput(GPIONum num)
        : GPIOBase(num, GPIOModeType::INPUT())
{
}

void GPIOInput::wakeup_enable(GPIOWakeupIntrType interrupt_type) const
{
    GPIO_CHECK_THROW(gpio_wakeup_enable(gpio_num.get_value<gpio_num_t>(),
                                        interrupt_type.get_value<gpio_int_type_t>()));
}

void GPIOInput::wakeup_disable() const
{
    GPIO_CHECK_THROW(gpio_wakeup_disable(gpio_num.get_value<gpio_num_t>()));
}

GPIO_OpenDrain::GPIO_OpenDrain(GPIONum num)
    : GPIOInput(num, GPIOModeType::INPUT_OUTPUT_OPEN_DRAIN())
{
}

}

#endif
