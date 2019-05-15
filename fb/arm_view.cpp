#include "arm_view.h"
#include <sstream>

//------------------------------------------------------------------------------
ArmView::ArmView(UIWidget *parent) : Panel(0, parent)
{
     create(8, 8, 400-16, 392-16);
     setBorder(false, false, false, false);

     Label *label;
     ToggleButton *toggle;
     Button *button;
     const char *TOGGLE_CAPTION[3] = {"Base", "Shoulder", "Elbow"};
     const char *PTP_LABEL[3] = {"X-Position (mm) :", "Y-Position (mm) :", "Z-Position (mm) :"};

     label = new Label(0, this);
     label->create(8, 8, 400-16-16, 24);
     label->setColor(DEFAULT_TEXT_COLOR, RGBToColor(0x70,0x13,0x34));
     label->setBorder(false);
     label->setMargin(16, 0);
     label->setValue("モータ励磁");

     for( int n = 0 ; n < 3 ; n++ )
     {
          toggle = new ToggleButton(ID_ENABLE+n, this);
          toggle->create(16+n*110, 40, 100, 32);
          toggle->setCaption(TOGGLE_CAPTION[n]);
          toggle->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){
               int axis = sender->getID() - ID_ENABLE;
               bool state = dynamic_cast<ToggleButton *>(sender)->getState();
               enableMotor(axis, state);
          });
     }

     label = new Label(0, this);
     label->create(8, 106, 400-16-16, 24);
     label->setColor(DEFAULT_TEXT_COLOR, RGBToColor(0x70,0x13,0x34));
     label->setBorder(false);
     label->setMargin(16, 0);
     label->setValue("イニシャライズ");

     button = new Button(ID_HOMING, this);
     button->create(16, 136, 100, 32);
     button->setCaption("原点復帰");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){
          startHoming();
     });
     button = new Button(ID_RESET, this);
     button->create(16+110, 136, 100, 32);
     button->setCaption("アラーム解除");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){
          clearAlarm();
     });

     button = new Button(ID_GOHOME, this);
     button->create(16+220, 136, 100, 32);
     button->setCaption("原点へ移動");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){
          goHome();
     });

     label = new Label(0, this);
     label->create(8, 204, 400-16-16, 24);
     label->setColor(DEFAULT_TEXT_COLOR, RGBToColor(0x70,0x13,0x34));
     label->setBorder(false);
     label->setMargin(16, 0);
     label->setValue("PTP動作");

     for( int n = 0 ; n < 3 ; n++ )
     {
          label = new Label(0, this);
          label->create(8, 234+48*n, 150, 36);
          label->setColor(DEFAULT_TEXT_COLOR, DEFAULT_CONTAINER_COLOR);
          label->setTextAlign(ALIGN_RIGHT|ALIGN_MIDDLE);
          label->setBorder(false);
          label->setValue(PTP_LABEL[n]);

          label = new Label(ID_PTP_INPUT+n, this, LARGE_FONT);
          label->create(160, 234+48*n, 80, 36);
          label->setColor(RGBToColor(0xD4,0xC3,0x6A), COLOR_BLACK);
          label->setTextAlign(ALIGN_RIGHT|ALIGN_MIDDLE);
          label->setMargin(8, 0);
          label->setValue("0");
          label->attachEvent(EVENT_CLICKED, [this](UIWidget *sender, int32_t, int32_t){
               editPTPPosition(sender);
          });
     }

     button = new Button(ID_MOVETO, this);
     button->create(256, 334, 120, 32);
     button->setCaption("指定位置へ移動");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          startMove();
     });
}

//------------------------------------------------------------------------------
void ArmView::update(Robot *robot)
{
     if( !isActive() || !isVisible() )
     {
          return;
     }
     for( int axis = 0 ; axis < 3 ; axis++ )
     {
          bool b = !robot->isHalted(axis);
          ToggleButton *toggle = dynamic_cast<ToggleButton *>(getChildByID(ID_ENABLE+axis));
          if( toggle )
          {
               toggle->setState(b);
          }
     }
     for( int n = 0 ; n < (int)m_children.size() ; n++ )
     {
          UIWidget *w = m_children[n];
          Button *button = dynamic_cast<Button *>(w);
          ToggleButton *toggle = dynamic_cast<ToggleButton *>(w); 
          if( !button && !toggle )
          {
               continue;
          }
          if( robot->isInMotion() )
          {
               if( w->isEnabled() )
               {
                    w->disable();
                    w->refresh();
               }
          }
          else
          {
               if( !w->isEnabled() )
               {
                    w->enable();
                    w->refresh();
               }
          }
     }
}

//------------------------------------------------------------------------------
void ArmView::enableMotor(int axis, bool state)
{
     triggerEvent(EVENT_ARM_ENABLE_MOTOR, axis, state? 1 : 0);
}

//------------------------------------------------------------------------------
void ArmView::startHoming()
{
     triggerEvent(EVENT_ARM_START_HOMING);
}

//------------------------------------------------------------------------------
void ArmView::clearAlarm()
{
     triggerEvent(EVENT_ARM_CLEAR_ALARM);
}

//------------------------------------------------------------------------------
void ArmView::goHome()
{
     triggerEvent(EVENT_ARM_GO_HOME);
}

//------------------------------------------------------------------------------
void ArmView::editPTPPosition(UIWidget *sender)
{
     NumEdit().open([this, sender](UIWidget *, int32_t result, int32_t value){
          if( result )
          {
               std::ostringstream oss;
               oss << value;
               dynamic_cast<Label *>(sender)->setValue(oss.str());
          }
     });
}

//------------------------------------------------------------------------------
void ArmView::startMove()
{
     triggerEvent(EVENT_ARM_START_MOVE);
}

//------------------------------------------------------------------------------
void ArmView::getDestPos(double& x, double& y, double& z)
{
     double pos[3] = {0, 0, 0};
     for( int n = 0 ; n < 3 ; n++ )
     {
          Label *label = dynamic_cast<Label *>(getChildByID(ID_PTP_INPUT+n));
          std::istringstream iss(label->getValue());
          iss >> pos[n];
     }
     x = pos[0];
     y = pos[1];
     z = pos[2];
     std::printf("(x,y,z) = (%f,%f,%f)\n", x, y, z);
}
