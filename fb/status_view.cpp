#include <cstdio>
#include <sstream>
#include "status_view.h"


//==============================================================================
const char *RobotStatusView::ITEM_NAME[4] = {
     "アーム(X)",
     "アーム(Y)",
     "アーム(Z)",
     "グリッパー"
};

//------------------------------------------------------------------------------
RobotStatusView::RobotStatusView(UIWidget *parent) : PaintBox(0, parent)
{
     m_position[0] = "----.-";
     m_position[1] = "----.-";
     m_position[2] = "----.-";
     m_gripper = "0";
     create(416, 40, 376, 64);
     attachEvent(EVENT_PAINT, [this](UIWidget *, int32_t, int32_t){
          internalDraw();
     });
}

//------------------------------------------------------------------------------
void RobotStatusView::setPosition(int axis, double pos)
{
     char buffer[10];
     sprintf(buffer, "%6.1f", pos);
     if( m_position[axis] != buffer)
     {
          m_position[axis] = buffer;
          if( isActive() )
          {
               drawPosition(axis);
          }
     }
}

//------------------------------------------------------------------------------
void RobotStatusView::clearPosition(int axis)
{ 
     if( m_position[axis] != "----.-" )
     {
          m_position[axis] = "----.-";
          if( isActive() )
          {
               drawPosition(axis);
          }
     } 
}

//------------------------------------------------------------------------------
void RobotStatusView::setGripValue(int value)
{
     std::ostringstream oss;
     oss << value;
     if( oss.str() != m_gripper )
     {
          m_gripper = oss.str();
          if( isActive() )
          {
               drawGripValue();
          }
     }
}

//------------------------------------------------------------------------------
void RobotStatusView::internalDraw()
{
     selectFont(SMALL_FONT);
     drawRect(m_clientRect, DEFAULT_BORDER_COLOR);
     Rect r = m_clientRect.clone();
     r.inflate(-1, -1).resizeHeight(24);
     fillRect(r, RGBToColor(0x70,0x13,0x34));
     Point p(0, 25);
     drawFastHLine(p, m_clientRect.width, DEFAULT_BORDER_COLOR);
     for( int n = 0 ; n < 4 ; n++ )
     {
          p.setPoint(1+n*93, 1);
          r.setRect(p.x, p.y, 93, 24);
          drawText(r, ITEM_NAME[n], ALIGN_CENTER|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR);
          if( n < 3 )
          {
               p.offset(92, 0);
               drawFastVLine(p, m_clientRect.height-1, DEFAULT_BORDER_COLOR);
          }
     }

     for( int n = 0 ; n < 3 ; n++ )
     {
          drawPosition(n);
     }
     drawGripValue();
}

//------------------------------------------------------------------------------
void RobotStatusView::drawPosition(int axis)
{
     selectFont(LARGE_FONT);
     Rect r(1+94*axis, 26, 93, 37);
     r.inflate(-12, 0);
     drawText(r, m_position[axis].c_str(), ALIGN_RIGHT|ALIGN_MIDDLE, RGBToColor(0xD4,0xC3,0x6A), DEFAULT_FACE_COLOR);
}

//------------------------------------------------------------------------------
void RobotStatusView::drawGripValue()
{
     selectFont(LARGE_FONT);
     Rect r(375-92, 26, 92, 37);
     r.inflate(-12, 0);
     drawText(r, m_gripper.c_str(), ALIGN_RIGHT|ALIGN_MIDDLE, RGBToColor(0xD4,0xC3,0x6A), DEFAULT_FACE_COLOR);
}


//==============================================================================
const char *MotorStatus::FLAG_LABELS[NUM_FLAGS][2] = {
     {"disable", "enable"},
     {"false", "true"},
     {"false", "true"},
     {"off", "on"},
     {"off", "on"},
     {"off", "on"},
     {"-", "WARN"},
     {"-", "ERROR"},
     {"-", "ERROR"},
     {"-", "ERROR"},
     {"-", "ERROR"},
     {"-", "ERROR"}
};

const uint16_t MotorStatus::FLAG_COLORS[NUM_FLAGS] = {
     0x4382,
     0x4382,
     0x4382,
     0x4382,
     0x4382,
     0x4382,
     COLOR_RED,
     COLOR_RED,
     COLOR_RED,
     COLOR_RED,
     COLOR_RED,
     COLOR_RED
};

std::function<bool(Robot*, int)> MotorStatus::readFlagFn[NUM_FLAGS] = {
     [](Robot *robot, int axis){ return !robot->isHalted(axis); },
     [](Robot *robot, int axis){ return robot->isInMotion(axis); },
     [](Robot *robot, int axis){ return robot->isHomeCompleted(axis); },
     [](Robot *robot, int axis){ return robot->getLimitState(axis, L6470::DIR_REVERSE)? false : true; },
     [](Robot *robot, int axis){ return (robot->getMotorStatus(axis) & 0x04)? true : false; },
     [](Robot *robot, int axis){ return robot->getLimitState(axis, L6470::DIR_FORWARD)? false : true; },
     [](Robot *robot, int axis){ return (robot->getAlarmFlag(axis) & 0x01)? true : false; },
     [](Robot *robot, int axis){ return (robot->getAlarmFlag(axis) & 0x02)? true : false; },
     [](Robot *robot, int axis){ return (robot->getAlarmFlag(axis) & 0x04)? true : false; },
     [](Robot *robot, int axis){ return (robot->getAlarmFlag(axis) & 0x08)? true : false; },
     [](Robot *robot, int axis){ return (robot->getAlarmFlag(axis) & 0x10)? true : false; },
     [](Robot *robot, int axis){ return (robot->getAlarmFlag(axis) & 0x20)? true : false; }
}; 

//------------------------------------------------------------------------------
MotorStatus::MotorStatus() : m_position(999999), m_speed(-1), m_invalidated(true)
{
     for( int n = 0 ; n < NUM_FLAGS ; n++ )
     {
          m_flags[n] = false;
     }
}

//------------------------------------------------------------------------------
bool MotorStatus::setPosition(int32_t value)
{
     if( m_position != value )
     {
          m_position = value;
          return true;
     }
     return m_invalidated;
}

//------------------------------------------------------------------------------
bool MotorStatus::setSpeed(int32_t value)
{
     if( m_speed != value )
     {
          m_speed = value;
          return true;
     }
     return m_invalidated;
}

//------------------------------------------------------------------------------
bool MotorStatus::setFlag(int n, bool f)
{
     if( m_flags[n] != f )
     {
          m_flags[n] = f;
          return true;
     }
     return m_invalidated;
}

//------------------------------------------------------------------------------
std::string MotorStatus::getPosition()
{
     std::ostringstream oss;
     oss << m_position;
     return oss.str(); 
}

//------------------------------------------------------------------------------
std::string MotorStatus::getSpeed()
{
     std::ostringstream oss;
     oss << m_speed;
     return oss.str();
}

//------------------------------------------------------------------------------
const char *MotorStatus::getFlag(int n)
{
     return FLAG_LABELS[n][m_flags[n]? 1 : 0];
}

//------------------------------------------------------------------------------
uint16_t MotorStatus::getFlagColor(int n)
{
     return m_flags[n]? FLAG_COLORS[n] : 0x18C3;
}

//==============================================================================
const char *MotorStatusView::ITEM_NAME[14] = {
     "Position (pulse)",
     "Speed (pulse/sec)",
     "Motor Enabled",
     "In-Motion",
     "Home Completed",
     "Limit SW (-)",
     "Home SW",
     "Limit SW (+)",
     "Thermal warning",
     "Under voltage",
     "Thermal shutdown",
     "Overcurrent",
     "Bridge-A stall",
     "Bridge-B stall",
};

const char *MotorStatusView::MOTOR_NAME[3] = {
     "Base", "Shoulder", "Elbow"
};

//------------------------------------------------------------------------------
MotorStatusView::MotorStatusView(UIWidget *parent) : PaintBox(0, parent)
{
     // 1+24+(1+20)*16+1 = 362;
     create(416, 112, 376, 320);
     attachEvent(EVENT_PAINT, [this](UIWidget *, int32_t, int32_t){
          internalDraw();
     });
}

//------------------------------------------------------------------------------
void MotorStatusView::internalDraw()
{
     selectFont(SMALL_FONT);
     drawRect(m_clientRect, DEFAULT_BORDER_COLOR);
     Rect r = m_clientRect.clone();
     r.inflate(-1, -1).resizeHeight(24);
     fillRect(r, RGBToColor(0x70,0x13,0x34));
     r.resizeWidth(146).resizeHeight(21*6).offset(0, 24+21*8);
     fillRect(r, RGBToColor(0x55,0x47,0x00));
     Point p(0, 25);
     for( int n = 0 ; n < 14 ; n++ )
     {
          drawFastHLine(p, m_clientRect.width, DEFAULT_BORDER_COLOR);
          r.setRect(p.x+1, p.y+1, 140, 20);
          drawText(r, ITEM_NAME[n], ALIGN_RIGHT|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR);
          p.offset(0, 21);
     }
     p.setPoint(m_clientRect.width-1, 0);
     for( int n = 2 ; n >= 0 ; --n )
     {
          p.offset(-76, 0);
          drawFastVLine(p, m_clientRect.height, DEFAULT_BORDER_COLOR);
          r.setRect(p.x+1, p.y+1, 75, 24);
          drawText(r, MOTOR_NAME[n], ALIGN_CENTER|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR);
     }
     m_status[0].setInvalidate(true);
     m_status[1].setInvalidate(true);
     m_status[2].setInvalidate(true);
}

//------------------------------------------------------------------------------
void MotorStatusView::update(Robot *robot)
{
     if( !isActive() )
     {
          return;
     }

     selectFont(SMALL_FONT);
     Rect rc;
     int16_t x = m_clientRect.width - 76*3;
     int16_t w = 75;
     int16_t h = 20;

     for( int axis = 0 ; axis < 3 ; axis++ )
     {
          rc.setRect(x+76*axis+4, 26, w-8, h);
          if( m_status[axis].setPosition(robot->getMotorPosition(axis)) )
          {
               drawText(rc, m_status[axis].getPosition().c_str(), ALIGN_RIGHT|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR, DEFAULT_FACE_COLOR);
          }
          
          rc.offset(0, 21);
          if( m_status[axis].setSpeed(robot->getMotorSpeed(axis)) )
          {
               drawText(rc, m_status[axis].getSpeed().c_str(), ALIGN_RIGHT|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR, DEFAULT_FACE_COLOR);
          }

          rc.inflate(4, 0);
          for( int flagIndex = 0 ; flagIndex < MotorStatus::NUM_FLAGS ; flagIndex++ )
          {
               rc.offset(0, 21);
               if( m_status[axis].setFlag(flagIndex, MotorStatus::readFlagFn[flagIndex](robot, axis)) )
               {
                    drawText(rc, m_status[axis].getFlag(flagIndex), ALIGN_CENTER|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR, m_status[axis].getFlagColor(flagIndex));
               }
          }
          m_status[axis].setInvalidate(false);
     }
}
