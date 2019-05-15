#include "teaching_view.h"
#include <cstdint>
#include <cmath>
#include <sstream>
#include <stdio.h>

//------------------------------------------------------------------------------
TeachingView::TeachingView(UIWidget *parent, Robot *robot)
     : PaintBox(0, parent), m_robot(robot), m_selectedIndex(0)
{
     create(8, 8, 400-16, 392-16);

     Button *button;
     Label *label;
     Rect r;
     for( int n = 0 ; n < NUM_POINTS ; n++ )
     {
          m_teachingPoints.push_back(new TeachPoint);
          r.setRect(8, 8+29*n, 248, 30);
          m_itemRect.push_back(r.clone());
     }

     load();

     label = new Label(0, this);
     label->create(266, 8, 110, 20);
     label->setColor(DEFAULT_TEXT_COLOR, DEFAULT_FACE_COLOR);
     label->setBorder(false);
     label->setTextAlign(ALIGN_LEFT|ALIGN_MIDDLE);
     label->setValue("ジョグ送り量");
     m_pitchEdit = new Label(0, this, LARGE_FONT);
     m_pitchEdit->create(266, 36, 110, 32);
     m_pitchEdit->setColor(RGBToColor(0xD4,0xC3,0x6A), COLOR_BLACK);
     m_pitchEdit->setTextAlign(ALIGN_RIGHT|ALIGN_MIDDLE);
     m_pitchEdit->setMargin(12, 0);
     m_pitchEdit->setValue("10 mm");
     m_pitchEdit->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          NumEdit().open([this](UIWidget *, int32_t result, int32_t value){
               if( result )
               {
                    value = std::abs(value);
                    setJogPitch(value);
               }
          });
     });

     button = new Button(ID_Y_FWD, this, LARGE_FONT);
     button->create(296, 80, 50, 50);
     button->setCaption("\x12");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){ jog(sender); });

     button = new Button(ID_X_REV, this, LARGE_FONT);
     button->create(266, 140, 50, 50);
     button->setCaption("\x14");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){ jog(sender); });
     button = new Button(ID_X_FWD, this, LARGE_FONT);
     button->create(326, 140, 50, 50);
     button->setCaption("\x15");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){ jog(sender); });

     button = new Button(ID_Y_REV, this, LARGE_FONT);
     button->create(296, 200, 50, 50);
     button->setCaption("\x13");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){ jog(sender); });

     button = new Button(ID_Z_DN, this, LARGE_FONT);
     button->create(266, 280, 50, 50);
     button->setCaption("\x13");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){ jog(sender); });
     button = new Button(ID_Z_UP, this, LARGE_FONT);
     button->create(326, 280, 50, 50);
     button->setCaption("\x12");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){ jog(sender); });

     button = new Button(ID_SET, this);
     button->create(8, 310, 120, 32);
     button->setCaption("現在位置をセット");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          setTeachingPoint();
     });

     button = new Button(ID_MOVE, this);
     button->create(136, 310, 120, 32);
     button->setCaption("選択した点へ移動");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          m_destPos = *m_teachingPoints[m_selectedIndex];
          triggerEvent(EVENT_TEACHING_MOVE);
     });

     attachEvent(EVENT_PAINT, [this](UIWidget *, int32_t, int32_t){
          internalDraw();
     });
}

//------------------------------------------------------------------------------
TeachingView::~TeachingView()
{
     save();
     for( int n = 0 ; n < (int)m_teachingPoints.size() ; n++ )
     {
          delete m_teachingPoints[n];
     }
}

//------------------------------------------------------------------------------
void TeachingView::load()
{
     FILE *fp = fopen("./teach/teaching.dat", "r");
     if( !fp )
     {
          printf("[TeachingView] Unable to open teaching.dat, set defaults.\n");
          for( int n = 0 ; n < NUM_POINTS ; n++ )
          {
               m_teachingPoints[n]->x = 0;
               m_teachingPoints[n]->y = 350;
               m_teachingPoints[n]->z = 200;
               m_teachingPoints[n]->coordToMotor();
          }
          return;
     }
     int32_t buf[3];
     for( int n = 0 ; n < NUM_POINTS ; n++ )
     {
          if( fread(buf, 4, 3, fp) != 3 )
          {
               break;
          }
          m_teachingPoints[n]->base     = buf[0];
          m_teachingPoints[n]->shoulder = buf[1];
          m_teachingPoints[n]->elbow    = buf[2];
          m_teachingPoints[n]->motorToCoord();
     }
     fclose(fp);
     printf("[TeachingView] teaching data successfully loaded.\n");
}

//------------------------------------------------------------------------------
void TeachingView::save()
{
     FILE *fp = fopen("./teach/teaching.dat", "w");
     if( !fp )
     {
          printf("[TeachingView] Unable to open teaching.dat\n");
          return;
     }
     for( int n = 0 ; n < NUM_POINTS ; n++ )
     {
          fwrite(&(m_teachingPoints[n]->base), 4, 1, fp);
          fwrite(&(m_teachingPoints[n]->shoulder), 4, 1, fp);
          fwrite(&(m_teachingPoints[n]->elbow), 4, 1, fp);
     }
     fclose(fp);
     printf("[TeachingView] teaching data successfully saved.\n");
}

//------------------------------------------------------------------------------
void TeachingView::internalDraw()
{
     fillRect(m_clientRect, DEFAULT_FACE_COLOR);
     for( int n = 0 ; n < (int)m_itemRect.size() ; n++ )
     {
          drawPointItem(n);
     }
}

//------------------------------------------------------------------------------
void TeachingView::drawPointItem(int n)
{
     selectFont(SMALL_FONT);

     char buffer[50];
     Rect r = m_itemRect[n];

     drawRect(r, DEFAULT_BORDER_COLOR);
     r.inflate(-1, -1);

     if( n == m_selectedIndex )
     {
          fillRect(r, 0x4382);
     }
     else
     {
          fillRect(r, DEFAULT_FACE_COLOR);
     }

     r.inflate(-6, 0);
     sprintf(buffer, "[%2d](%6.1f, %6.1f, %6.1f)", n+1, m_teachingPoints[n]->x,  m_teachingPoints[n]->y, m_teachingPoints[n]->z);
     drawText(r, buffer, ALIGN_LEFT|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR);
}

//------------------------------------------------------------------------------
void TeachingView::onTouched(int16_t x, int16_t y)
{
     UIWidget::onTouched(x, y);

     Point pos(x, y);
     for( int n = 0 ; n < (int)m_itemRect.size() ; n++ )
     {
          if( m_itemRect[n].include(pos) && n != m_selectedIndex )
          {
               int prev = m_selectedIndex;
               m_selectedIndex = n;
               drawPointItem(n);
               drawPointItem(prev);
          }
     }
}

//------------------------------------------------------------------------------
void TeachingView::update(Robot *robot)
{
     if( !isVisible() || !isActive() )
     {
          return;
     }

     for( int n = 0 ; n < m_children.size() ; n++ )
     {
          if( robot->isInMotion() )
          {
               if( m_children[n]->isEnabled() )
               {
                    m_children[n]->disable();
                    m_children[n]->refresh();
               }
          }
          else
          {
               if( !m_children[n]->isEnabled() )
               {
                    m_children[n]->enable();
                    m_children[n]->refresh();
               }
          }
     }
}

//------------------------------------------------------------------------------
double TeachingView::getJogPitch()
{
     std::istringstream iss(m_pitchEdit->getValue());
     double value;
     iss >> value;
     value = std::abs(value);
     return value;
}

//------------------------------------------------------------------------------
void TeachingView::setJogPitch(double value)
{
     std::ostringstream oss;
     oss << (int)value << " mm";
     m_pitchEdit->setValue(oss.str());
}

//------------------------------------------------------------------------------
void TeachingView::jog(UIWidget *sender)
{
     m_destPos.base     = m_robot->getMotorPosition(0);
     m_destPos.shoulder = m_robot->getMotorPosition(1);
     m_destPos.elbow    = m_robot->getMotorPosition(2);
     m_destPos.motorToCoord();
     double pitch = getJogPitch();
     
     switch( sender->getID() )
     {
          case ID_X_FWD:
               m_destPos.x = std::round(m_destPos.x + pitch);
               break;
          case ID_X_REV:
               m_destPos.x = std::round(m_destPos.x - pitch);
               break;
          case ID_Y_FWD:
               m_destPos.y = std::round(m_destPos.y + pitch);
               break;
          case ID_Y_REV:
               m_destPos.y = std::round(m_destPos.y - pitch);
               break;
          case ID_Z_UP:
               m_destPos.z = std::round(m_destPos.z + pitch);
               break;
          case ID_Z_DN:
               m_destPos.z = std::round(m_destPos.z - pitch);
               break;
     }
     triggerEvent(EVENT_TEACHING_MOVE);
}

//------------------------------------------------------------------------------
void TeachingView::setTeachingPoint()
{
     m_destPos.base     = m_robot->getMotorPosition(0);
     m_destPos.shoulder = m_robot->getMotorPosition(1);
     m_destPos.elbow    = m_robot->getMotorPosition(2);
     m_destPos.motorToCoord();
     (*m_teachingPoints[m_selectedIndex]) = m_destPos;
     drawPointItem(m_selectedIndex);
}

//------------------------------------------------------------------------------