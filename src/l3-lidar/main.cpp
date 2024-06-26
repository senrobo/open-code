#include <Arduino.h>
#include <HardwareSerial.h>
#include <TFLI2C.h>
#include <Wire.h>

#include "PacketSerial.h"
#include "SPI.h"
#include "util.h"

HardwareSerial MySerial0(0);

#define XSHUT1PIN 0
// #define DEBUGTOF

PacketSerial L3LIDARSerial;
TFLI2C front;
TFLI2C right;
TFLI2C back;
TFLI2C left;
TFLI2C tflI2C[4] = {front, right, back, left};

int16_t tfDist[4];

int tfAddress[4] = {0x35, 0x22, 0x33, 0x44}; // front, right, back, left

struct lidardata {
    int distance[4];
};
lidardata esp32lidardata;

typedef struct lidarTxPayload {
    lidardata esp32lidardata;
} lidarTxPayload;

void setup() {
    Serial.begin(115200);
    MySerial0.begin(115200, SERIAL_8N1, -1, -1);

    L3LIDARSerial.begin(&MySerial0);
    Wire.begin();
    for (int i = 0; i < 4; i++) { tflI2C[i].Save_Settings(tfAddress[i]); }

    // tflI2C[3].Set_I2C_Addr(0x44,0x22);
    // tflI2C[3].Soft_Reset(0x44);
    // tflI2C[3].Soft_Reset(0x44);
}

void loop() {
    // i2cscanner();
    for (int i = 0; i < 4; i++) {
        if (tflI2C[i].getData(tfDist[i], tfAddress[i])) // If read okay...
        {
#ifdef DEBUGTOF
            if (i < 3) {
                Serial.print(" Dist: ");
                Serial.print(tfDist[i]);
            }

            else if (i == 3) {
                Serial.print(" Dist: ");
                Serial.println(tfDist[i]);
            }
#endif
            esp32lidardata.distance[i] = tfDist[i];
        }

        else {
            esp32lidardata.distance[i] = 0;
            tflI2C[i].printStatus();
        }
    }
    delay(20);

    byte buf[sizeof(lidarTxPayload)];
    memcpy(buf, &esp32lidardata, sizeof(esp32lidardata));
    L3LIDARSerial.send(buf, sizeof(buf));
}
