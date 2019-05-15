#ifndef   ROBOT_H
#define   ROBOT_H

#include <thread>
#include <mutex>
#include <cstdint>
#include "L6470.h"

//------------------------------------------------------------------------------
class Robot
{
     public:
          enum
          {
               MOTOR_BASE     = 0,
               MOTOR_SHOULDER = 1,
               MOTOR_ELBOW    = 2
          };

     private:
          enum{ BASE_BUSY = 17 };  // ESP32 : 36 };             
          enum{ BASE_CS   = 26 };  // ESP32 : 14 };
          enum{ BASE_NLIM = 19 };  // ESP32 : 26 };
          enum{ BASE_PLIM = 13 };  // ESP32 : 25 };

          enum{ SHOULDER_BUSY = 22 };   // ESP32 : 39 };
          enum{ SHOULDER_CS   = 5 };    // ESP32 : 32 };
          enum{ SHOULDER_PLIM = 25 };   // ESP32 : 35 };

          enum{ ELBOW_BUSY = 24 }; // ESP32 : 34 };
          enum{ ELBOW_CS   = 6 };  // ESP32 : 33 };
          enum{ ELBOW_PLIM = 21 }; // ESP32 : 13 };

          enum{ SERVO_PIN = 18 };  // ESP32 : 27 };
          enum{
               SERVO_MIN_VALUE = 76,
               SERVO_MAX_VALUE = 101
          };

          L6470 *m_stepper[3];
          int    m_homingState;
          int    m_motionState[3];
          int    m_gripperCurrentValue;
          int    m_gripperDestValue;
          bool   m_terminated;

          std::thread *m_homingThread;
          std::thread *m_motionThread;
          std::thread *m_servoThread;
          std::mutex   m_mutex;

          void execHoming();
          void execMotion();
          void execServo();


     public:
          Robot();
          ~Robot();

          void initialize();
          bool startHoming();
          bool startMotion(int axis, int32_t destpos);
          bool startMotion3D(int32_t base, int32_t shoulder, int32_t elbow);
          bool isInMotion(int axis = -1);
          bool isAlarmHappened(int axis = -1);
          bool isHalted(int axis);
          bool isHomeCompleted(int axis = -1);
          bool canMove(){ return isHomeCompleted() && !isInMotion() && !isAlarmHappened(); }
          void enableMotor(int axis, bool ena);
          void clearAlarm(int axis = -1);
          void softStop(int axis = -1);
          void hardStop(int axis = -1);
          uint8_t getLimitState(int axis, int dir);
          uint8_t getAlarmFlag(int axis);

          void moveGripper(uint8_t value);
          uint8_t getGripperValue() const {
                return (uint8_t)(100*(m_gripperCurrentValue - SERVO_MIN_VALUE)/(SERVO_MAX_VALUE - SERVO_MIN_VALUE)); 
          }
          void stopGripper();

          uint16_t getMotorStatus(int axis);
          int32_t  getMotorPosition(int axis);
          int32_t  getMotorSpeed(int axis);
          uint32_t getMotorParam(int axis, uint8_t id);
          void     setMotorParam(int axis, uint8_t id, uint32_t value);

          static bool coordToMotorPos(double x, double y, double z, int32_t *base, int32_t *shoulder, int32_t *elbow);
          static void motorPosToCoord(int32_t base, int32_t shoulder, int32_t elbow, double *X, double *Y, double *Z);
};


#endif
