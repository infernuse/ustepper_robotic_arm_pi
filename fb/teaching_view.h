#ifndef   TEACHING_VIEW_H
#define   TEACHING_VIEW_H

#include "ui.h"
#include "robot.h"
#include <vector>
#include <cmath>

//------------------------------------------------------------------------------
class TeachPoint
{
     public:
          int32_t base;
          int32_t shoulder;
          int32_t elbow;
          double  x;
          double  y;
          double  z;
          TeachPoint() : base(0), shoulder(0), elbow(0), x(0), y(0), z(0){}
          const TeachPoint& operator = (const TeachPoint& r)
          {
               base = r.base;
               shoulder = r.shoulder;
               elbow = r.elbow;
               x = r.x;
               y = r.y;
               z = r.z;
               return *this;
          }
          void motorToCoord(){
               Robot::motorPosToCoord(base, shoulder, elbow, &x, &y, &z);
          }
          bool coordToMotor(){
               int32_t b, s, e;
               if( Robot::coordToMotorPos(x, y, z, &b, &s, &e) )
               {
                    base = b;
                    shoulder = s;
                    elbow = e;
                    return true;
               }
               return false;
          }
};

//------------------------------------------------------------------------------
class TeachingView : public PaintBox
{
     private:
          enum{ NUM_POINTS = 10 };
          enum {
               ID_SET  = 5000,
               ID_MOVE = 6000,
               ID_X_FWD = 7001,
               ID_X_REV = 7002,
               ID_Y_FWD = 8001,
               ID_Y_REV = 8002,
               ID_Z_UP = 9001,
               ID_Z_DN = 9002
          };
          std::vector<TeachPoint *> m_teachingPoints;
          std::vector<Rect> m_itemRect;
          int m_selectedIndex;
          Robot *m_robot;
          TeachPoint m_destPos;
          Label *m_pitchEdit;
          void load();
          void save();
          void internalDraw();
          void drawPointItem(int n);
          double getJogPitch();
          void setJogPitch(double value);
          void jog(UIWidget *sender);
          void setTeachingPoint();

     protected:
          void onTouched(int16_t x, int16_t y);

     public:
          enum {
               EVENT_TEACHING_MOVE = 301
          };
          TeachingView(UIWidget *parent, Robot *robot);
          ~TeachingView();
          void update(Robot *robot);
          void getDestPos(double& x, double& y, double& z)
          {
               x = std::round(m_destPos.x);
               y = std::round(m_destPos.y);
               z = std::round(m_destPos.z);
          }
};

#endif
