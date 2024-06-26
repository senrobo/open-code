#include "movement.h"
#include "config.h"

#define FL_PWM_PIN 11
#define FL_IN1_PIN 10
#define FL_IN2_PIN 12
#define BL_PWM_PIN 8
#define BL_IN1_PIN 7
#define BL_IN2_PIN 9
#define FR_PWM_PIN 2
#define FR_IN1_PIN 1
#define FR_IN2_PIN 3
#define BR_PWM_PIN 5
#define BR_IN1_PIN 4
#define BR_IN2_PIN 6

#define FL_MULTIPLIER 1
#define FR_MULTIPLIER 1
#define BL_MULTIPLIER 1
#define BR_MULTIPLIER 1



#define SIN34 0.55919290F
#define SIN56 0.82903757F
#define COS34 0.82903757F
#define COS56 0.55919290F


Movement::Movement(){};

void Movement::initialize() {
    // initalize pins
    pinMode(FL_PWM_PIN, OUTPUT);
    pinMode(FL_IN1_PIN, OUTPUT);
    pinMode(FL_IN2_PIN, OUTPUT);
    pinMode(BL_PWM_PIN, OUTPUT);
    pinMode(BL_IN1_PIN, OUTPUT);
    pinMode(BL_IN2_PIN, OUTPUT);
    pinMode(FR_PWM_PIN, OUTPUT);
    pinMode(FR_IN1_PIN, OUTPUT);
    pinMode(FR_IN2_PIN, OUTPUT);
    pinMode(BR_PWM_PIN, OUTPUT);
    pinMode(BR_IN1_PIN, OUTPUT);
    pinMode(BR_IN2_PIN, OUTPUT);
    // changing PWM resolution
    analogWriteResolution(10);

    analogWriteFrequency(FL_PWM_PIN, 146484); 
    analogWriteFrequency(BL_PWM_PIN, 146484);
    analogWriteFrequency(FR_PWM_PIN, 146484);
    analogWriteFrequency(BR_PWM_PIN, 146484);
};

void Movement::updateParameters(double actualbearing, double actualdirection,
                                double actualvelocity) {
    _actualbearing = actualbearing;
    _actualdirection = actualdirection;
    _actualvelocity = actualvelocity;
};

void Movement::setconstantDirection(Direction::constant params) {
    _targetdirection = params.value;
};

void Movement::setmovetoPointDirection(Direction::movetoPoint params) {
    _targetdirection =
        (Vector::fromPoint(params.destination) - params.robotCoordinate).angle;
};

void Movement::setlinetrackDirection(Direction::linetrack params) {
    linetrackController.updateSetpoint(params.targetLineDepth);
    double correction = linetrackController.advance(params.lineDepth);
    // Serial.println(correction);
    if (params.trackLeftwards == true) {
        _targetdirection = correction * 90 + params.angleBisector - 90;
    } else {
        _targetdirection = (correction * -90) + params.angleBisector + 90;
    }
}

void Movement::setconstantVelocity(Velocity::constant params) {
    _targetvelocity = params.value;
};
void Movement::setstopatPointVelocity(Velocity::stopatPoint params) {
    auto correction = stopatPointController.advance(params.errordistance);

    _targetvelocity = abs(correction);
    _targetvelocity =
        constrain(_targetvelocity, params.minSpeed, params.maxSpeed);
};

void Movement::setconstantBearing(Bearing::constant params) {
    _targetbearing = params.targetValue;
    _actualbearing = params.actualBearing;
};
void Movement::setmoveBearingtoPoint(Bearing::moveBearingtoPoint params) {
    if (params.destination != _finalDestination) {
        _finalbearing = params.finalBearing;
        _initialbearing = _actualbearing;
        _initialrobotcoordinate = params.robotCoordinate;
    }
    _finalDestination = params.destination;
    double progress =
        (params.robotCoordinate - _initialrobotcoordinate).distance /
        (Vector::fromPoint(params.destination) - _initialrobotcoordinate)
            .distance;

    _targetbearing =
        _initialbearing + progress * (_finalbearing - _initialbearing);
};
void Movement::setBearingSettings(double minV, double maxV, double KP,
                                  double KD, double KI) {
    bearingController.updateLimits(minV, maxV, infinity());
    bearingController.updateGains(KP, KD, KI, 0.2);
};

void Movement::drive(Point robotPosition) {
    bearingController.updateSetpoint(_targetbearing);

    if (_targetbearing <= 90 && _targetbearing >= -90) {
        _movingbearing =
            bearingController.advance(clipAngleto180degrees(_actualbearing));
    } else if (_targetbearing > 90 && _targetbearing <= 135) {
        _movingbearing =
            bearingController.advance(clipAngleto180degrees(_actualbearing));
    } else if (_targetbearing < -90 && _targetbearing >= -135) {
        _movingbearing =
            bearingController.advance(clipAngleto180degrees(_actualbearing));
    } else {
        _movingbearing =
            bearingController.advance(clipAngleto360degrees(_actualbearing));
    }

    double x = sind(_targetdirection);
    double y = cosd(_targetdirection);

    
    #ifdef ATTACK_BOT_CODE

    if (robotPosition.x > X_AXIS_SLOWDOWN_START) {
        double deccel =
            constrain(X_AXIS_SLOWDOWN_SPEED -
                          ((robotPosition.x - X_AXIS_SLOWDOWN_START) /
                           (X_AXIS_SLOWDOWN_END - X_AXIS_SLOWDOWN_START) *
                           X_AXIS_SLOWDOWN_SPEED),
                      0, 1000);

        x = constrain(x, -700.0, deccel);
    } else if (robotPosition.x < -X_AXIS_SLOWDOWN_START) {

        double deccel =
            constrain(X_AXIS_SLOWDOWN_SPEED -
                          ((robotPosition.x + X_AXIS_SLOWDOWN_START) /
                           (X_AXIS_SLOWDOWN_END - X_AXIS_SLOWDOWN_START) *
                           X_AXIS_SLOWDOWN_SPEED),
                      -1000, 0);
        x = constrain(x, deccel, 600);
    }
    if (robotPosition.y > Y_AXIS_SLOWDOWN_START) {
        double deccel =
            constrain(Y_AXIS_SLOWDOWN_SPEED -
                          ((robotPosition.y - Y_AXIS_SLOWDOWN_START) /
                           (Y_AXIS_SLOWDOWN_END - Y_AXIS_SLOWDOWN_START) *
                           Y_AXIS_SLOWDOWN_SPEED),
                      0, 1000);
        y = constrain(y, -600.0, deccel);
    } else if (robotPosition.y < -Y_AXIS_SLOWDOWN_START) {
        double deccel =
            constrain(Y_AXIS_SLOWDOWN_SPEED -
                          ((robotPosition.y + Y_AXIS_SLOWDOWN_START) /
                           (Y_AXIS_SLOWDOWN_END - Y_AXIS_SLOWDOWN_START) *
                           Y_AXIS_SLOWDOWN_SPEED),
                      -1000, 0);
        y = constrain(y, deccel, 600);

    }
    #endif

    const auto transformspeed = [this](double velocityDirection,
                                       double angularComponent) {
        return (int)(_targetvelocity * velocityDirection + angularComponent);
    };

    double angularComponent = _movingbearing * 1.3;

    double FLSpeed =
        transformspeed(x * SIN34 + y * COS56, angularComponent) * FL_MULTIPLIER;
    double FRSpeed = transformspeed(x * -SIN34 + y * COS56, -angularComponent) *
                     FR_MULTIPLIER;
    double BRSpeed = transformspeed(x * SIN34 + y * COS56, -angularComponent) *
                     BR_MULTIPLIER;
    double BLSpeed = transformspeed(x * -SIN34 + y * COS56, angularComponent) *
                     BL_MULTIPLIER;


    // if (FLSpeed < 50 && FLSpeed > -50) { FLSpeed = 0; }
    // if (FRSpeed < 50 && FRSpeed > -50) { FRSpeed = 0; }
    // if (BLSpeed < 50 && BLSpeed > -50) { BLSpeed = 0; }
    // if (BRSpeed < 50 && BRSpeed > -50) { BRSpeed = 0; }


#ifdef ROBOT1
    digitalWriteFast(FL_IN1_PIN, FLSpeed > 0 ? LOW : HIGH);
    digitalWriteFast(FL_IN2_PIN, FLSpeed > 0 ? HIGH : LOW);

    digitalWriteFast(FR_IN1_PIN, FRSpeed > 0 ? LOW : HIGH);
    digitalWriteFast(FR_IN2_PIN, FRSpeed > 0 ? HIGH : LOW);

    digitalWriteFast(BR_IN1_PIN, BRSpeed > 0 ? LOW : HIGH);
    digitalWriteFast(BR_IN2_PIN, BRSpeed > 0 ? HIGH : LOW);

    digitalWriteFast(BL_IN1_PIN, BLSpeed > 0 ? LOW : HIGH);
    digitalWriteFast(BL_IN2_PIN, BLSpeed > 0 ? HIGH : LOW);

    analogWrite(FL_PWM_PIN, constrain(abs(FLSpeed), -600, 600));
    analogWrite(FR_PWM_PIN, constrain(abs(FRSpeed), -600, 600));
    analogWrite(BL_PWM_PIN, constrain(abs(BLSpeed), -600, 600));
    analogWrite(BR_PWM_PIN, constrain(abs(BRSpeed), -600, 600));
#endif
#ifdef ROBOT2
    digitalWriteFast(FL_IN1_PIN, FLSpeed > 0 ? HIGH : LOW);
    digitalWriteFast(FL_IN2_PIN, FLSpeed > 0 ? LOW : HIGH);

    digitalWriteFast(FR_IN1_PIN, FRSpeed > 0 ? HIGH : LOW);
    digitalWriteFast(FR_IN2_PIN, FRSpeed > 0 ? LOW : HIGH);

    digitalWriteFast(BR_IN1_PIN, BRSpeed > 0 ? LOW : HIGH);
    digitalWriteFast(BR_IN2_PIN, BRSpeed > 0 ? HIGH : LOW);

    digitalWriteFast(BL_IN1_PIN, BLSpeed > 0 ? HIGH : LOW);
    digitalWriteFast(BL_IN2_PIN, BLSpeed > 0 ? LOW : HIGH);

    analogWrite(FL_PWM_PIN, constrain(abs(FLSpeed), -700, 700));
    analogWrite(FR_PWM_PIN, constrain(abs(FRSpeed), -700, 700));
    analogWrite(BL_PWM_PIN, constrain(abs(BLSpeed), -700, 700));
    analogWrite(BR_PWM_PIN, constrain(abs(BRSpeed), -700, 700));
#endif
#ifdef DEBUG_MOVEMENT
    const auto printSerial = [](double value) {
        Serial.printf("%5d", (int)value);
    };

    Serial.print("FLSpeed: ");
    printSerial(FLSpeed);
    Serial.print(" | FRSpeed: ");
    printSerial(FRSpeed);
    Serial.print(" | BLSpeed: ");
    printSerial(BLSpeed);
    Serial.print(" | BRSpeed: ");
    printSerial(BRSpeed);
    Serial.print(" | TargetDirection: ");
    printSerial(_targetdirection);
    Serial.print(" | TargetVelocity: ");
    printSerial(_targetvelocity);
    Serial.print(" | YPosition: ");
    printSerial(robotPosition.y);
    Serial.print(" | Y: ");
    printSerial(Y_AXIS_SLOWDOWN_SPEED -
                ((robotPosition.y - Y_AXIS_SLOWDOWN_START) /
                 (Y_AXIS_SLOWDOWN_END - Y_AXIS_SLOWDOWN_START) *
                 Y_AXIS_SLOWDOWN_SPEED));
    Serial.println(" ");

#endif
};

double Movement::applySigmoid(double startSpeed, double endSpeed,
                              double progress, double constant) {
    const auto multiplier = 1 / (1 + powf(1200, constant * 2 * progress - 1));
    return startSpeed + (endSpeed - startSpeed) * multiplier;
};

std::vector<double> Movement::getmotorValues() {
    const auto x = sind(_targetdirection);
    const auto y = cosd(_targetdirection);

    const auto transformspeed = [this](double velocityDirection,
                                       double angularComponent) {
        return (int)(_targetvelocity * velocityDirection + angularComponent);
    };

    double angularComponent = _movingbearing * 1.3;

    double FLSpeed = transformspeed(x * SIN34 + y * COS56, angularComponent);
    double FRSpeed = transformspeed(x * -SIN34 + y * COS56, -angularComponent);
    double BRSpeed = transformspeed(x * SIN34 + y * COS56, -angularComponent);
    double BLSpeed = transformspeed(x * -SIN34 + y * COS56, angularComponent);

    return {FLSpeed, FRSpeed, BLSpeed, BRSpeed};
};
