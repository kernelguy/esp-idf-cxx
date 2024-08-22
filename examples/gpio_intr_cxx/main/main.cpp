/* Blink C++ Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <thread>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "gpio_cxx.hpp"
#include "gpio_intr_cxx.hpp"

using namespace idf;
using namespace std;
using namespace std::placeholders;

#define GPIO_NUM_A 4
#define GPIO_NUM_B 5
#define GPIO_NUM_C 6

/*****************************************************
 * Lambda function used as callback on GPIO A interrupt
 *****************************************************/
static size_t counter_a = 0;
static auto lambda_cb_gpio_a = [](const GPIOInterrupt &gpio) {
    if (gpio.get_gpio_num() == GPIONum(GPIO_NUM_A))
    {
        counter_a++;
    }
};

static void task_intr_a(void* arg)
{
    // create interrupt on GPIO A using a lambda function as callback
    GPIOInterrupt gpio_intr_a(GPIONum(GPIO_NUM_A),
                              GPIOModeType::INPUT_OUTPUT_OPEN_DRAIN(), // Use open drain to let test generate own interrupts
                              GPIOPullMode::PULLUP(),
                              GPIODriveStrength::DEFAULT(),
                              GPIOInterruptType::POSITIVE_EDGE(),
                              lambda_cb_gpio_a);

    size_t cur_counter_a = 0;
    for(size_t i = 0 ; ; i++)
    {
        if (counter_a != cur_counter_a)
        {
            printf("interrupt occurred on GPIO A: %d\n", counter_a);
            cur_counter_a = counter_a;
        }

        this_thread::sleep_for(std::chrono::milliseconds(100));
        if (i & 1) {
            gpio_intr_a.set_floating();
        }
        else {
            gpio_intr_a.set_low();
        }
    }
}

/*****************************************************
 * Static function used a callback from a different
 * thread on GPIO B interrupt
 *****************************************************/
static size_t counter_b = 0;
static void static_cb_gpio_b(const GPIOInterrupt &gpio)
{
    if (gpio.get_gpio_num() == GPIONum(GPIO_NUM_B))
    {
        counter_b++;
    }
}

static void task_intr_b(void* arg)
{
    GPIOInterrupt gpio_intr_b(GPIONum(GPIO_NUM_B),
                              GPIOModeType::INPUT_OUTPUT_OPEN_DRAIN(), // Use open drain to let test generate own interrupts
                              GPIOPullMode::PULLUP(),
                              GPIODriveStrength::STRONGEST(),
                              GPIOInterruptType::POSITIVE_EDGE(),
                             static_cb_gpio_b);

    size_t cur_counter_b = 0;
    for(size_t i = 0 ; ; i++)
    {
        if (counter_b != cur_counter_b)
        {
            printf("interrupt occurred on GPIO B: %d\n", counter_b);
            cur_counter_b = counter_b;
        }

        this_thread::sleep_for(std::chrono::milliseconds(100));
        if (i & 1) {
            gpio_intr_b.set_floating();
        }
        else {
            gpio_intr_b.set_low();
        }
    }
}

/*****************************************************
 * Member method used a callback from a different
 * thread on GPIO C interrupt
 *****************************************************/
class TestIntrC {
public:
    size_t first_counter_c;

    TestIntrC(): first_counter_c(0)
    {}

    void first_callback(const GPIOInterrupt &gpio)
    {
        if (gpio.get_gpio_num() == GPIONum(GPIO_NUM_C))
        {
            first_counter_c++;
        }
    }
};

static void task_intr_c(void* arg)
{
    TestIntrC test_intr_c{};

    GPIOInterrupt gpio_intr_c(GPIONum(GPIO_NUM_C),
                              GPIOModeType::INPUT_OUTPUT_OPEN_DRAIN(), // Use open drain to let test generate own interrupts
                              GPIOPullMode::PULLUP(),
                              GPIODriveStrength::WEAK(),
                              GPIOInterruptType::POSITIVE_EDGE(),
                              nullptr);

    gpio_intr_c.set_callback(std::bind(&TestIntrC::first_callback, &test_intr_c, _1));

    size_t cur_counter_c = 0;
    for(size_t i = 0 ; ; i++)
    {
        if (test_intr_c.first_counter_c != cur_counter_c)
        {
            printf("Both callbacks triggered on GPIO interrupt C: %d\n", test_intr_c.first_counter_c);
            cur_counter_c = test_intr_c.first_counter_c;
        }

        this_thread::sleep_for(std::chrono::milliseconds(100));
        if (i & 1) {
            gpio_intr_c.set_floating();
        }
        else {
            gpio_intr_c.set_low();
        }
    }
}

/*****************************************************
 * Main function
 *****************************************************/
extern "C" void app_main(void)
{
    // install ISR service before creating the interrupt
    try {
        GPIOInterruptService::Get().Level1().Start();
    }
    catch (const GPIOException& e) {
        printf("[0x%x]: %s\n", e.error, e.what());
    }

    // create interrupt on GPIO A in a different thread using lambda
    // function as callback
    xTaskCreatePinnedToCore(task_intr_a, "task_intr_a", 4096, NULL, 0, NULL, 0);

    // create interrupt on GPIO B in a different thread using static
    // function as callback
    xTaskCreatePinnedToCore(task_intr_b, "task_intr_b", 4096, NULL, 0, NULL, 1);

    // create interrupt on GPIO C in a different thread using member
    // method (non static) as callback. Two methods are registered on the
    // interrupt and are both triggered.
    xTaskCreatePinnedToCore(task_intr_c, "task_intr_c", 4096, NULL, 0, NULL, 1);
}