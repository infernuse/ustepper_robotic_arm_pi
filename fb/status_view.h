#ifndef   STATUS_VIEW_H
#define   STATUS_VIEW_H

#include <string>
#include "ui.h"
#include "robot.h"

//------------------------------------------------------------------------------
class RobotStatusView : public PaintBox
{
     private:
          static const char *ITEM_NAME[4];
          std::string m_position[3];
          std::string m_gripper;

          void internalDraw();
          void drawPosition(int axis);
          void drawGripValue();

     public:
          RobotStatusView(UIWidget *parent);
          void setPosition(int axis, double position);
          void setGripValue(int value);
          void clearPosition(int axis);//{ m_position[axis] = "----.-"; }
};

//------------------------------------------------------------------------------
class MotorStatus
{
     public:
          enum {
               ENABLE         = 0,
               IN_MOTION      = 1,
               HOME_COMPLETED = 2,
               N_LIMIT        = 3,
               HOME           = 4,
               P_LIMIT        = 5,
               TH_WARN        = 6,
               UVLO           = 7,
               TH_SHDN        = 8,
               OVC            = 9,
               STALL_A        = 10,
               STALL_B        = 11,
               NUM_FLAGS      = 12
          };
     private:
          static const char    *FLAG_LABELS[NUM_FLAGS][2];
          static const uint16_t FLAG_COLORS[NUM_FLAGS];
          int32_t m_position;
          int32_t m_speed;
          bool m_flags[NUM_FLAGS];
          bool m_invalidated;
     public:
          static std::function<bool(Robot*, int)> readFlagFn[NUM_FLAGS]; 
          MotorStatus();
          void setInvalidate(bool b){ m_invalidated = b; }
          bool setPosition(int32_t value);
          bool setSpeed(int32_t value);
          bool setFlag(int n, bool f);
          std::string getPosition();
          std::string getSpeed();
          const char *getFlag(int n);
          uint16_t getFlagColor(int n);
};

//------------------------------------------------------------------------------
class MotorStatusView : public PaintBox
{
     private:
          static const char *ITEM_NAME[14];
          static const char *MOTOR_NAME[3];

          MotorStatus m_status[3];

          void internalDraw();
     public:
          MotorStatusView(UIWidget *parent);
          void update(Robot *robot);
};

#endif
