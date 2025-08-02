/* The example of ESP-IDF
 *
 * This sample code is in the public domain.
 */

// AT+MODE=TEST
// AT+TEST=RFCFG, 866, SF7, 125, 12, 15, 14, ON, OFF, OFF
// AT+TEST=RXLRPKT

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "sx126x.h"

static const char *TAG = "MAIN";
#define CONFIG_SENDER 1

#if CONFIG_SENDER
#define CONFIG_RECEIVER 0
#else
#define CONFIG_RECEIVER 1
#endif // CONFIG_SENDER

#define CONFIG_USE_TCXO 1

#if CONFIG_SENDER
void task_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[255]; // Maximum Payload size of SX1261/62/68 is 255
	while(1) {
		TickType_t nowTick = xTaskGetTickCount();
		int txLen = sprintf((char *)buf, "Hello World %"PRIu32, nowTick);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent...", txLen);

		// Wait for transmission to complete
		if (LoRaSend(buf, txLen, SX126x_TXMODE_SYNC) == false) {
			ESP_LOGE(pcTaskGetName(NULL),"LoRaSend fail");
		}

		// Do not wait for the transmission to be completed
		//LoRaSend(buf, txLen, SX126x_TXMODE_ASYNC );

		int lost = GetPacketLost();
		if (lost != 0) {
			ESP_LOGW(pcTaskGetName(NULL), "%d packets lost", lost);
		}

		vTaskDelay(pdMS_TO_TICKS(1000));
	} // end while

	// never reach here
	vTaskDelete( NULL );
}
#endif // CONFIG_SENDER

#if CONFIG_RECEIVER
void task_rx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[255]; // Maximum Payload size of SX1261/62/68 is 255
	while(1) {
		uint8_t rxLen = LoRaReceive(buf, sizeof(buf));
		if ( rxLen > 0 ) { 
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", rxLen, rxLen, buf);

			int8_t rssi, snr;
			GetPacketStatus(&rssi, &snr);
			ESP_LOGI(pcTaskGetName(NULL), "rssi=%d[dBm] snr=%d[dB]", rssi, snr);
		}
		vTaskDelay(1); // Avoid WatchDog alerts
	} // end while

	// never reach here
	vTaskDelete( NULL );
}
#endif // CONFIG_RECEIVER

// #define feather 0
// #if feather
// int CONFIG_MISO_GPIO=37;
// int CONFIG_MOSI_GPIO=35;
// int CONFIG_SCLK_GPIO=36;
// int CONFIG_NSS_GPIO=5;
// int CONFIG_RST_GPIO=11;
// int CONFIG_BUSY_GPIO=6;
// int CONFIG_TXEN_GPIO=10;  //feather 
// int CONFIG_RXEN_GPIO=9;
// #endif

// int CONFIG_MISO_GPIO=8;
// int CONFIG_MOSI_GPIO=9;
// int CONFIG_SCLK_GPIO=7;
// int CONFIG_NSS_GPIO=41;
// int CONFIG_RST_GPIO=42;
// int CONFIG_BUSY_GPIO=40;
// int CONFIG_TXEN_GPIO=-1;
// int CONFIG_RXEN_GPIO=38;

// #define xsiao 1
// #if xsiao
// int CONFIG_MISO_GPIO=8;
// int CONFIG_MOSI_GPIO=9;
// int CONFIG_SCLK_GPIO=7;  //xsiao
// int CONFIG_NSS_GPIO=41;
// int CONFIG_RST_GPIO=42;
// int CONFIG_BUSY_GPIO=40;
// // //
// int CONFIG_TXEN_GPIO=-1;
// int CONFIG_RXEN_GPIO=-1;
// #endif


void app_main()
{
	//LoRaDebugPrint(true);

	// Initialize LoRa
	lora_gpio_t pins = {
		.CONFIG_MISO_GPIO=8,
		.CONFIG_MOSI_GPIO=9,
		.CONFIG_SCLK_GPIO=7,  //xsiao
		.CONFIG_NSS_GPIO=41,
		.CONFIG_RST_GPIO=42,
		.CONFIG_BUSY_GPIO=40,
		
		.CONFIG_TXEN_GPIO=-1,
		.CONFIG_RXEN_GPIO=-1
	};
	LoRaInit(&pins);

	lora_begin_params_t params = {
		.frequencyInHz = 866000000,
		.txPowerInDbm = 22,
		.tcxoVoltage = 0,
		.useRegulatorLDO = false
	};
	#if CONFIG_USE_TCXO
		ESP_LOGW(TAG, "Enable TCXO");
		params.tcxoVoltage = 3.1; // use TCXO
		params.useRegulatorLDO = true; // use DCDC + LDO
	#endif
	if (LoRaBegin(&params) != 0) {
		ESP_LOGE(TAG, "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}

	lora_config_params_t config = {
		.spreadingFactor = 7,
		.bandwidth = 4,
		.codingRate = 1,
		.preambleLength = 8,
		.payloadLen = 0,
		.crcOn = true,
		.invertIrq = false
	};
	LoRaConfig(&config);


#if CONFIG_SENDER
	xTaskCreate(&task_tx, "TX", 1024*4, NULL, 5, NULL);
#endif
#if CONFIG_RECEIVER
	xTaskCreate(&task_rx, "RX", 1024*4, NULL, 5, NULL);
#endif
}
