#ifndef   GRIPPER_VIEW_H
#define   GRIPPER_VIEW_H

#include "ui.h"

class GripperView : public Panel
{
     private:
          enum {
               ID_VALUE = 200,
               ID_DEC   = 201,
               ID_INC   = 202,
               ID_MOVE  = 203,
          };
          Label *m_valueEdit;

     public:
          enum { EVENT_GRIPPER_MOVE = 2001 };
          GripperView(UIWidget *parent);
          int32_t getValue();
          void setValue(int32_t value);
};

#endif
