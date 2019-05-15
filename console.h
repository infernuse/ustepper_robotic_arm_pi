#ifndef   CONSOLE_H
#define   CONSOLE_H

#include "robot.h"
#include "ui.h"
#include "arm_view.h"
#include "gripper_view.h"
#include "teaching_view.h"
#include "script_view.h"
#include "status_view.h"

//------------------------------------------------------------------------------
class RobotConsole
{
     private:
          Robot *m_robot;
          Desktop *m_desktop;
          TouchManager *m_touchManager;

          ArmView *m_armView;
          GripperView *m_gripperView;
          TeachingView *m_teachingView;
          ScriptView *m_scriptView;
          RobotStatusView *m_robotStatusView;
          MotorStatusView *m_motorStatusView;

          bool m_terminated;

          void initialize();
          void updateRobotStatus();
          void enableMotor(int axis, bool enable);
          void startHoming();
          void clearAlarm();
          void startMove(int32_t base, int32_t shoulder, int32_t elbow);
          void moveGripper(int32_t value);
          void stopAll();

     public:
          RobotConsole(Robot *robot);
          ~RobotConsole();
          bool execute();
          bool terminated(){ return m_terminated; }
};

#endif
