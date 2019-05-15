#include "gripper_view.h"
#include <sstream>
#include <algorithm>

//------------------------------------------------------------------------------
GripperView::GripperView(UIWidget *parent) : Panel(0, parent)
{
     create(8, 8, 400-16, 392-16);
     setBorder(false, false, false, false);

     Label *label;
     Button *button;
     
     label = new Label(0, this);
     label->create(8, 8, 400-16-16, 24);
     label->setColor(DEFAULT_TEXT_COLOR, RGBToColor(0x70,0x13,0x34));
     label->setBorder(false);
     label->setMargin(16, 0);
     label->setValue("グリッパー操作");

     button = new Button(ID_DEC, this, LARGE_FONT);
     button->create(16, 40, 48, 48);
     button->setCaption("\x11");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          setValue(getValue()-4);
          triggerEvent(EVENT_GRIPPER_MOVE, getValue());
     });
     
     m_valueEdit = new Label(ID_VALUE, this, LARGE_FONT);
     m_valueEdit->create(72, 40, 100, 48);
     m_valueEdit->setColor(RGBToColor(0xD4,0xC3,0x6A), COLOR_BLACK);
     m_valueEdit->setTextAlign(ALIGN_CENTER|ALIGN_MIDDLE);     
     m_valueEdit->setValue("100 %");
     m_valueEdit->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          NumEdit().open([this](UIWidget *, int32_t result, int32_t value){
               if( result )
               {
                    value = 4*((value + 2)/4);
                    setValue(value);
                    triggerEvent(EVENT_GRIPPER_MOVE, getValue());
               }
          });
     });

     button = new Button(ID_INC, this, LARGE_FONT);
     button->create(180, 40, 48, 48);
     button->setCaption("\x10");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          setValue(getValue()+4);
          triggerEvent(EVENT_GRIPPER_MOVE, getValue());
     });
}

//------------------------------------------------------------------------------
int32_t GripperView::getValue()
{
     std::istringstream iss(m_valueEdit->getValue());
     int32_t value;
     iss >> value;
     value = std::max(0, value);
     value = std::min(value, 100);     
     return value;
}

//------------------------------------------------------------------------------
void GripperView::setValue(int32_t value)
{
     std::ostringstream oss;
     oss << value << " %";
     m_valueEdit->setValue(oss.str());
}

