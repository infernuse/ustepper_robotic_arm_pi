#include "console.h"
#include <cstdio>
#include <cmath>

//------------------------------------------------------------------------------
RobotConsole::RobotConsole(Robot *robot)
     : m_robot(robot), m_terminated(false)
{
     // std::printf("RobotConsole started.\n");

     m_desktop = new Desktop();
     m_touchManager = new TouchManager();
     m_touchManager->pushEventListener(m_desktop);

     NumEdit().initialize(m_touchManager);
     MsgBox().initialize(m_touchManager);

     initialize();
}

//------------------------------------------------------------------------------
RobotConsole::~RobotConsole()
{
     delete m_touchManager;
     delete m_desktop;
}

//------------------------------------------------------------------------------
void RobotConsole::initialize()
{

     PaintBox *box = new PaintBox(0, m_desktop);
     box->create(0, 0, 800, 32);
     box->attachEvent(EVENT_PAINT, [](UIWidget *sender, int32_t, int32_t){
          Rect r = sender->getClientRect();
          sender->fillRect(r, UIWidget::RGBToColor(0x14, 0x06, 0x3A));
          Point p(0, r.height-1);
          sender->drawFastHLine(p, r.width, UIWidget::RGBToColor(0x89, 0x7B, 0xAF));
          r.inflate(-16, 4);
          sender->selectFont(LARGE_FONT);
          sender->drawText(r, "Robotic Arm Console", ALIGN_LEFT|ALIGN_MIDDLE, UIWidget::RGBToColor(0x89, 0x7B, 0xAF));
          sender->selectFont(SMALL_FONT);
          sender->drawText(r, "192.168.128.32", ALIGN_RIGHT|ALIGN_MIDDLE, UIWidget::RGBToColor(0x89, 0x7B, 0xAF));
     });

     Tabbar *tabbar = new Tabbar(1, m_desktop);
     tabbar->create(8, 40, 400, 40);
     tabbar->addTab(0, "アーム", 80);
     tabbar->addTab(1, "グリッパー", 80);
     tabbar->addTab(2, "教示", 80);
     tabbar->addTab(3, "スクリプト", 80);
     tabbar->attachEvent(EVENT_SELECT_CHANGED, [this](UIWidget *sender, int32_t index, int32_t){
          printf("tab changed : %d\n", (int)index);
          switch( index )
          {
               case 0:
                    m_gripperView->hide();
                    m_teachingView->hide();
                    m_scriptView->hide();
                    m_armView->show();
                    m_armView->refresh();
                    break;
               case 1:
                    m_armView->hide();
                    m_teachingView->hide();
                    m_scriptView->hide();
                    m_gripperView->show();
                    m_gripperView->refresh();
                    break;
               case 2:
                    m_armView->hide();
                    m_gripperView->hide();
                    m_scriptView->hide();
                    m_teachingView->show();
                    m_teachingView->refresh();
                    break;
               case 3:
                    m_armView->hide();
                    m_gripperView->hide();
                    m_teachingView->hide();
                    m_scriptView->show();
                    m_scriptView->refresh();
                    break;
          }
     });

     Panel *panel = new Panel(2, m_desktop);
     panel->create(8, 80, 400, 392);
     panel->setBorder(true, false, true, true);

     m_armView = new ArmView(panel);
     m_armView->attachEvent(ArmView::EVENT_ARM_ENABLE_MOTOR, [this](UIWidget *, int32_t axis, int32_t ena){
          enableMotor(axis, ena? true : false);
     });
     m_armView->attachEvent(ArmView::EVENT_ARM_START_HOMING, [this](UIWidget *, int32_t, int32_t){
          startHoming();
     });
     m_armView->attachEvent(ArmView::EVENT_ARM_CLEAR_ALARM, [this](UIWidget *, int32_t, int32_t){
          clearAlarm();
     });
     m_armView->attachEvent(ArmView::EVENT_ARM_GO_HOME, [this](UIWidget *, int32_t, int32_t){
          startMove(0, 0, 0);
     });
     m_armView->attachEvent(ArmView::EVENT_ARM_START_MOVE, [this](UIWidget *, int32_t, int32_t){
          double x, y, z;
          m_armView->getDestPos(x, y, z);
          int32_t b, s, e;
          if( !Robot::coordToMotorPos(x, y, z, &b, &s, &e) )
          {
               MsgBox().open(MBS_ERROR, "指定位置が可動範囲外です.", [this](UIWidget *, int32_t, int32_t){});
               return;
          }
          startMove(b, s, e);
     });

     m_gripperView = new GripperView(panel);
     m_gripperView->hide();
     m_gripperView->attachEvent(GripperView::EVENT_GRIPPER_MOVE, [this](UIWidget *, int32_t value, int32_t){
          moveGripper(value);
     });

     m_teachingView = new TeachingView(panel, m_robot);
     m_teachingView->hide();
     m_teachingView->attachEvent(TeachingView::EVENT_TEACHING_MOVE, [this](UIWidget *, int32_t, int32_t){
          double x, y, z;
          m_teachingView->getDestPos(x, y, z);
          int32_t b, s, e;
          if( !Robot::coordToMotorPos(x, y, z, &b, &s, &e) )
          {
               MsgBox().open(MBS_ERROR, "指定位置が可動範囲外です.", [this](UIWidget *, int32_t, int32_t){});
               return;
          }
          startMove(b, s, e);
     });

     m_scriptView = new ScriptView(panel, m_robot);
     m_scriptView->hide();

     m_robotStatusView = new RobotStatusView(m_desktop);
     m_motorStatusView = new MotorStatusView(m_desktop);

     Button *button = new Button(3, m_desktop, LARGE_FONT);
     button->create(692, 440, 100, 32);
     button->setCaption("終了");

     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          MsgBox().open(MBS_CONFIRM, "終了しても良いですか？", [this](UIWidget *, int32_t ok, int32_t){
               if( ok )
               {
                    m_terminated = true;
               }
          });
     });

     button = new Button(4, m_desktop, LARGE_FONT, BUTTONTYPE_DANGER);
     button->create(416, 440, 150, 32);
     button->setCaption("即時停止");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          stopAll();
     });

     m_desktop->show();
     m_desktop->refresh();

     m_touchManager->run();

     // std::printf("RobotConsole started.\n");
}

//------------------------------------------------------------------------------
bool RobotConsole::execute()
{
     if( m_terminated )
     {
          return false;
     }
     m_touchManager->dispatchEvent();
     m_armView->update(m_robot);
     m_teachingView->update(m_robot);
     m_scriptView->update();
     updateRobotStatus();
     m_motorStatusView->update(m_robot);
     return true;
}

//------------------------------------------------------------------------------
void RobotConsole::updateRobotStatus()
{
     if( m_robot->isHomeCompleted() )
     {
          int32_t b = m_robot->getMotorPosition(0);
          int32_t s = m_robot->getMotorPosition(1);
          int32_t e = m_robot->getMotorPosition(2);
          double x, y, z;
          Robot::motorPosToCoord(b, s, e, &x, &y, &z);
          m_robotStatusView->setPosition(0, x);
          m_robotStatusView->setPosition(1, y);
          m_robotStatusView->setPosition(2, z);
     }
     else
     {
          m_robotStatusView->clearPosition(0);
          m_robotStatusView->clearPosition(1);
          m_robotStatusView->clearPosition(2);
     }
     m_robotStatusView->setGripValue(m_robot->getGripperValue());
}

//------------------------------------------------------------------------------
void RobotConsole::enableMotor(int axis, bool enable)
{
     if( m_robot->isInMotion(axis) || m_robot->isAlarmHappened(axis) )
     {
          return;
     }
     m_robot->enableMotor(axis, enable);
}

//------------------------------------------------------------------------------
void RobotConsole::startHoming()
{
     if( !m_robot->startHoming() )
     {
          MsgBox().open(MBS_ERROR, "コマンドを実行できません.", [this](UIWidget *, int32_t, int32_t){});
     }
}

//------------------------------------------------------------------------------
void RobotConsole::clearAlarm()
{
     for( int axis = 0 ; axis < 3 ; axis++ )
     {
          if( !m_robot->isInMotion(axis) )
          {
               m_robot->clearAlarm(axis);
          }
     }
}

//------------------------------------------------------------------------------
void RobotConsole::startMove(int32_t base, int32_t shoulder, int32_t elbow)
{
     if( !m_robot->startMotion3D(base, shoulder, elbow) )
     {
          MsgBox().open(MBS_ERROR, "コマンドを実行できません.", [this](UIWidget *, int32_t, int32_t){});
     }

     // int32_t destpos[3];
     // uint32_t distance[3];
     // uint32_t longest = 0;
     // bool b[3] = {true, true, true};
     // uint8_t dir[3];

     // destpos[0] = base;
     // destpos[1] = shoulder;
     // destpos[2] = elbow;
     // for( int axis = 0 ; axis < 3 ; axis++ )
     // {
     //      if( m_robot->isInMotion(axis) || m_robot->isAlarmHappened(axis) )
     //      {
     //           MsgBox().open(MBS_ERROR, "コマンドを実行できません.", [this](UIWidget *, int32_t, int32_t){});
     //           return;
     //      }
     //      int32_t abspos = m_robot->getMotorPosition(axis);
     //      if( abspos == destpos[axis] )
     //      {
     //           b[axis] = false;    // この軸は動かす必要はない
     //      }
     //      if( destpos[axis] > abspos )
     //      {
     //           dir[axis] = L6470::DIR_FORWARD;
     //      }
     //      else
     //      {
     //           dir[axis] = L6470::DIR_REVERSE;
     //      }
     //      distance[axis] = std::abs(destpos[axis] - abspos);
     //      if( distance[axis] > longest )
     //      {
     //           longest = distance[axis];
     //      }
     // }
     // if( longest == 0 )
     // {
     //      return; // どの軸も動かす必要がない
     // }
     // // 各軸の速度を決定し，駆動開始
     // for( int axis = 0 ; axis < 3 ; axis++ )
     // {
     //      if( b[axis] )
     //      {
     //           int32_t speed = 16 * distance[axis] / longest;
     //           m_robot->setMotorParam(axis, L6470::PRM_MAX_SPEED, speed);
     //           m_robot->startMotion(axis, destpos[axis]);
     //      }
     // }
}

//------------------------------------------------------------------------------
void RobotConsole::moveGripper(int32_t value)
{
     m_robot->moveGripper((uint8_t)value);
}

//------------------------------------------------------------------------------
void RobotConsole::stopAll()
{
     for( int axis = 0 ; axis < 3 ; axis++ )
     {
          if( !m_robot->isHalted(axis) )
          {
               m_robot->hardStop(axis);
          }
     }
     m_robot->stopGripper();
}
