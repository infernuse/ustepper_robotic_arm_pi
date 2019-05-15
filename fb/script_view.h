#ifndef   SCRIPT_VIEW_H
#define   SCRIPT_VIEW_H

#include "ui.h"
#include "robot.h"
#include "script.h"
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <mutex>

//------------------------------------------------------------------------------
class ScriptView : public PaintBox
{
     private:
          enum{ MAX_FILES = 8 };
          enum{
               ID_UPDATE = 6001,
               ID_RUN = 6002,
               ID_STOP = 6003
          };
          Script *m_script;
          std::vector<Rect> m_itemRect;
          std::vector<std::string> m_files;
          std::vector<std::string> m_messages;
          std::mutex m_mutex;
          int m_selectedIndex;
          bool m_messageUpdated;
          void internalDraw();
          void drawFileItem(int n);
          void createFileList();
          void runScript();
          void stopScript();

          static int fileFilter(const struct dirent *dir);

     protected:
          void onTouched(int16_t x, int16_t y);

     public:
          enum {
               EVENT_SCRIPT_RUN = 401,
               EVENT_SCRIPT_STOP = 402
          };
          ScriptView(UIWidget *parent, Robot *robot);
          ~ScriptView();
          void update();
};

#endif
