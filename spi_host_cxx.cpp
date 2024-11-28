/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if __cpp_exceptions

#include <cstdint>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "hal/spi_types.h"
#include "spi_host_cxx.hpp"
#include "spi_host_private_cxx.hpp"

using namespace std;

namespace idf {

SPIException::SPIException(esp_err_t error) : ESPException(error) { }

SPITransferException::SPITransferException(esp_err_t error) : SPIException(error) { }

SPIMaster::SPIMaster(SPINum host,
        SCLK sclk,
        MOSI mosi,
        MISO miso,
        QSPIWP qspiwp,
        QSPIHD qspihd,
        SPI_DMAConfig dma_config,
        SPITransferSize transfer_size)
    : spi_host(host)
{
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = int(mosi.get_value());
    bus_config.miso_io_num = int(miso.get_value());
    bus_config.sclk_io_num = int(sclk.get_value());
    bus_config.quadwp_io_num = int(qspiwp.get_value());
    bus_config.quadhd_io_num = int(qspihd.get_value());
    bus_config.max_transfer_sz = int(transfer_size.get_value());

    SPI_CHECK_THROW(spi_bus_initialize(spi_host.get_value<spi_host_device_t>(), &bus_config, dma_config.get_value()));
}

SPIMaster::~SPIMaster()
{
    spi_bus_free(spi_host.get_value<spi_host_device_t>());
}

SPIFuture::SPIFuture()
    : transaction(), is_valid(false)
{
}

SPIFuture::SPIFuture(shared_ptr<SPITransactionDescriptor> transaction)
    : transaction(std::move(transaction)), is_valid(true)
{
}

SPIFuture::SPIFuture(SPIFuture &&other) noexcept
    : transaction(std::move(other.transaction)), is_valid(true)
{
    other.is_valid = false;
}

SPIFuture &SPIFuture::operator=(SPIFuture &&other) noexcept
{
    if (this != &other) {
        transaction = std::move(other.transaction);
        is_valid = other.is_valid;
        other.is_valid = false;
    }
    return *this;
}

SPITransactionResult SPIFuture::get()
{
    if (!is_valid) {
        throw std::future_error(future_errc::no_state);
    }

    return transaction->get();
}

future_status SPIFuture::wait_for(chrono::milliseconds timeout)
{
    if (transaction->wait_for(timeout)) {
        return std::future_status::ready;
    } else {
        return std::future_status::timeout;
    }
}

void SPIFuture::wait()
{
    transaction->wait();
}

bool SPIFuture::valid() const noexcept
{
    return is_valid;
}

SPIDevice::SPIDevice(SPINum spi_host, CS cs, Frequency frequency, QueueSize q_size) : device_handle()
{
    device_handle = new SPIDeviceHandle(spi_host, cs, frequency, q_size);
}

SPIDevice::~SPIDevice()
{
    delete device_handle;
}

SPIFuture SPIDevice::transfer(const uint8_t *apTxData, uint8_t *apRxData, size_t aDataSize, std::function<void(void *)> pre_callback,
                              std::function<void(void *)> post_callback, void *user_data)
{
    current_transaction = make_shared<SPITransactionDescriptor>(apTxData,
                                                                apRxData,
                                                                aDataSize,
                                                                device_handle,
                                                                user_data,
                                                                std::move(pre_callback),
                                                                std::move(post_callback));
    current_transaction->start();
    return SPIFuture(current_transaction);
}

void SPIDevice::prepare(const uint8_t *apTxData, uint8_t *apRxData, size_t aDataSize, std::function<void(void *)> pre_callback, std::function<void(void *)> post_callback, void *user_data)
{
    current_transaction = make_shared<SPITransactionDescriptor>(apTxData,
                                                                apRxData,
                                                                aDataSize,
                                                                device_handle,
                                                                user_data,
                                                                std::move(pre_callback),
                                                                std::move(post_callback));
}

SPIFuture SPIDevice::transfer_prepared()
{
    current_transaction->start();
    return SPIFuture(current_transaction);
}

void SPIDevice::StartPolling()
{
    current_transaction->StartPolling();
}

SPITransactionDescriptor::SPITransactionDescriptor(const uint8_t *apTxData, uint8_t *apRxData, size_t aDataSize, SPIDeviceHandle *apHandle, void *apUserData,
                                                   trans_callback_t aPreCallback, trans_callback_t aPostCallback)
        : device_handle(apHandle),
          pre_callback(std::move(aPreCallback)),
          post_callback(std::move(aPostCallback)),
          user_data(apUserData)
{
    if (apRxData == nullptr && apTxData == nullptr) {
        throw SPITransferException(ESP_ERR_INVALID_ARG);
    }
    if (aDataSize == 0) {
        throw SPITransferException(ESP_ERR_INVALID_ARG);
    }
    if (apHandle == nullptr) {
        throw SPITransferException(ESP_ERR_INVALID_ARG);
    }

    transaction.flags = 0;
    transaction.rx_buffer = apRxData;
    transaction.length = aDataSize * 8;
    transaction.tx_buffer = apTxData;
    transaction.user = this;
}

SPITransactionDescriptor::~SPITransactionDescriptor()
{
    if (started) {
        assert(received_data);  // We need to make sure that trans_desc has been received, otherwise the
                                // driver may still write into it afterwards.
    }
}

void SPITransactionDescriptor::start()
{
    SPI_CHECK_THROW(device_handle->acquire_bus(portMAX_DELAY));
    SPI_CHECK_THROW(device_handle->queue_trans(&transaction, 0));
    received_data = false;
    started = true;
}

void SPITransactionDescriptor::StartPolling()
{
    SPI_CHECK_THROW(device_handle->start_polling(&transaction, 0));
    received_data = false;
    started = true;
}

void SPITransactionDescriptor::Acquire()
{
    SPI_CHECK_THROW(device_handle->acquire_bus(portMAX_DELAY));
}

void SPITransactionDescriptor::Release()
{
    device_handle->release_bus();
}


void SPITransactionDescriptor::wait()
{
    while (!wait_for(chrono::milliseconds(portMAX_DELAY))) { }
}

bool SPITransactionDescriptor::wait_for(const chrono::milliseconds &timeout_duration)
{
    if (received_data) {
        return true;
    }

    if (!started) {
        throw SPITransferException(ESP_ERR_INVALID_STATE);
    }

    spi_transaction_t *acquired_trans_desc;
    esp_err_t err = device_handle->get_trans_result(&acquired_trans_desc,
            (TickType_t) timeout_duration.count() / portTICK_PERIOD_MS);

    if (err == ESP_ERR_TIMEOUT) {
        return false;
    }

    if (err != ESP_OK) {
        throw SPITransferException(err);
    }

    if (acquired_trans_desc != &transaction) {
        throw SPITransferException(ESP_ERR_INVALID_STATE);
    }

    received_data = true;
    device_handle->release_bus();

    return true;
}

SPITransactionResult SPITransactionDescriptor::get()
{
    if (!received_data) {
        wait();
    }

    return {static_cast<uint8_t*>(transaction.rx_buffer), transaction.length / 8};
}

} // idf

#endif // __cpp_exceptions
