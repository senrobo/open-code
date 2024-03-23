#ifndef MAIN_H
#define MAIN_H

#include "ArduinoEigen.h"
#include "config.h"
#include "movement.h"
#include "shared.h"
#include "util.h"


// global variables

extern Solenoid solenoid;
extern TruthValues truthValues;
extern Movement movement;
extern ProcessedValues processedValues;
extern SensorValues sensorValues;
extern Execution execution;
extern LightArray lightArray;
// subroutines
static double loopTimeinmicros() {
    timeControl.now = micros();
    timeControl.dt = timeControl.now - timeControl.last;
    timeControl.last = timeControl.now;
    return timeControl.dt / 1000000;
};

Vector localizeWithOffensiveGoal();
Vector localizeWithDefensiveGoal();
Vector localizeWithBothGoals();
double ballAngleOffset(double distance, double direction);
void receiveCameraTxData(const byte *buf, size_t size);
void attachBrushless();
double curveAroundBallMultiplier(double angle, double actual_distance, double start_distance);
void verifyingObjectExistance();
void processLidars();

//lightring
void selectMUXChannel(uint8_t channel);
int readMUXChannel(int index, int muxInput);
void getValues();
void findLine();

// functions
#endif