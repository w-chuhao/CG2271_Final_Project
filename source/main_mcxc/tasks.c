#include "tasks.h"
#include "app_types.h"
#include "adc.h"
#include "led.h"
#include "buttons.h"
#include "ssd1306.h"
#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <string.h>
#include <stdbool.h>

/* ================================================================== */
/* sensorTask — reads ADC every 300 ms, pushes to queue                */
/* ================================================================== */
void sensorTask(void *p) {
    (void)p;
    SensorPacket pkt;
    TickType_t lastWake = xTaskGetTickCount();
    memset(&pkt, 0, sizeof(pkt));

    while (1) {
        bool started = false;
        if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            started = g_systemStarted;
            xSemaphoreGive(g_statusMutex);
        }

        if (started) {
            pkt.lightRaw = ADC_ReadChannel(PHOTO_ADC_CH);
            ADC_ReadMicWindow(&pkt.micRaw, &pkt.micMin, &pkt.micMax, &pkt.micP2P);
            pkt.p2pFlag  = (pkt.micP2P > MIC_P2P_THRESHOLD) ? 1U : 0U;
            xQueueOverwrite(g_sensorQueue, &pkt);
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(300));
    }
}

/* ================================================================== */
/* buttonTask — handles SW2 (start/stop) and SW3 (ack alert)          */
/* ================================================================== */
void buttonTask(void *p) {
    (void)p;
    Buttons_Init(g_buttonSema);

    while (1) {
        if (xSemaphoreTake(g_buttonSema, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(50)) == pdTRUE) {

                if (g_sw2StartPending) {
                    g_sw2StartPending = false;
                    if (!g_systemStarted) {
                        g_systemStarted = true;
                        g_alertLatched  = false;
                        PRINTF("SW2: system STARTED\r\n");
                    } else {
                        g_systemStarted = false;
                        g_alertLatched  = false;
                        PRINTF("SW2: system STOPPED\r\n");
                    }
                }

                if (g_sw3AckPending) {
                    g_sw3AckPending = false;
                    if (g_systemStarted && g_alertLatched) {
                        g_alertLatched = false;
                        PRINTF("SW3: alert ACKNOWLEDGED\r\n");
                    } else {
                        PRINTF("SW3: no active alert\r\n");
                    }
                }

                xSemaphoreGive(g_statusMutex);
            }
        }
    }
}

/* ================================================================== */
/* alertTask — drives RGB LED and SSD1306 display                      */
/*                                                                      */
/* The display update is rate-limited to once per 500 ms to avoid      */
/* hammering the I2C bus on every queue receive timeout.               */
/* ================================================================== */
void alertTask(void *p) {
    (void)p;
    SensorPacket pkt;
    TickType_t lastDisplayUpdate = 0;

    while (1) {
        bool gotPacket = (xQueueReceive(g_sensorQueue, &pkt,
                                        pdMS_TO_TICKS(200)) == pdTRUE);

        if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(50)) == pdTRUE) {

            if (gotPacket) {
                g_latestPacket = pkt;
                if (pkt.p2pFlag) {
                    g_alertLatched = true;
                }
            }

            /* Update RGB LED immediately */
            if (!g_systemStarted) {
                g_alertLatched = false;
                LED_OffAll();
            } else if (g_alertLatched) {
                LED_SetRGB(true, false, false);   /* RED   */
            } else {
                LED_SetRGB(false, true, false);   /* GREEN */
            }

            /* Update SSD1306 at most every 500 ms — I2C is slow (1024
             * bytes at 100 kHz takes ~90 ms) so don't call on every loop */
            TickType_t now = xTaskGetTickCount();
            if ((now - lastDisplayUpdate) >= pdMS_TO_TICKS(500)) {
                lastDisplayUpdate = now;
                SSD1306_ShowAll(g_systemStarted,
                                g_alertLatched,
                                g_latestPacket.lightRaw,
                                g_latestPacket.micP2P);
            }

            xSemaphoreGive(g_statusMutex);
        }
    }
}

/* ================================================================== */
/* printTask — UART log every 1 s                                       */
/* ================================================================== */
void printTask(void *p) {
    (void)p;
    TickType_t lastWake = xTaskGetTickCount();
    SensorPacket pkt;
    bool started, alert;
    memset(&pkt, 0, sizeof(pkt));

    PRINTF("\r\n=== MCXC444 SENSOR MONITOR ===\r\n");
    PRINTF("SW2 (PTC3) : Start / Stop\r\n");
    PRINTF("SW3 (PTA4) : Acknowledge alert\r\n");
    PRINTF("Light      : PTB0 (ADC0_SE8)\r\n");
    PRINTF("Mic        : PTB1 (ADC0_SE9)\r\n");
    PRINTF("SSD1306    : I2C1  PTC1=SCL  PTC2=SDA  addr=0x3C\r\n");
    PRINTF("P2P thresh : >%u\r\n\r\n", MIC_P2P_THRESHOLD);

    while (1) {
        if (xSemaphoreTake(g_statusMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            pkt     = g_latestPacket;
            started = g_systemStarted;
            alert   = g_alertLatched;
            xSemaphoreGive(g_statusMutex);
        } else {
            started = false;
            alert   = false;
            memset(&pkt, 0, sizeof(pkt));
        }

        PRINTF("STARTED=%u | ALERT=%u | LIGHT=%u | MIC_P2P=%u"
               " | MIC_MIN=%u | MIC_MAX=%u | RAW=%u | FLAG=%u\r\n",
               started ? 1U : 0U,
               alert   ? 1U : 0U,
               pkt.lightRaw,
               pkt.micP2P,
               pkt.micMin,
               pkt.micMax,
               pkt.micRaw,
               pkt.p2pFlag);

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(1000));
    }
}
