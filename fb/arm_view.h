#ifndef   ARM_VIEW_H
#define   ARM_VIEW_H

#include "ui.h"
#include "robot.h"

class ArmView : public Panel
{
     private:
          enum {
               ID_ENABLE    = 100,
               ID_HOMING    = 200,
               ID_RESET     = 300,
               ID_GOHOME    = 400,
               ID_PTP_INPUT = 500,
               ID_MOVETO    = 600
          };
          void enableMotor(int axis, bool state);
          void startHoming();
          void clearAlarm();
          void goHome();
          void editPTPPosition(UIWidget *sender);
          void startMove();

     public:
          enum {
               EVENT_ARM_START_HOMING = 1001,
               EVENT_ARM_ENABLE_MOTOR = 1002,
               EVENT_ARM_CLEAR_ALARM  = 1003,
               EVENT_ARM_GO_HOME      = 1004,
               EVENT_ARM_START_MOVE   = 1006
          };
          ArmView(UIWidget *parent);
          void update(Robot *robot);
          void getDestPos(double& x, double& y, double& z);
};

#endif
