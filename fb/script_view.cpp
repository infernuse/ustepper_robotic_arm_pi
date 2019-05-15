#include "script_view.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

//------------------------------------------------------------------------------
int ScriptView::fileFilter(const struct dirent *dir)
{
     const char *ext = ".lua"; 

     int len = strlen(dir->d_name);
     if( len < 4 )
     {
           return 0;
     }
     len -= 4;
     int extval = *((int *)&(dir->d_name[len]));
     if( extval == *((int *)ext) )
     {
          return 1;
     }
     return 0;
}

//------------------------------------------------------------------------------
ScriptView::ScriptView(UIWidget *parent, Robot *robot)
     : PaintBox(0, parent), m_selectedIndex(-1), m_messageUpdated(false)
{
     create(8, 8, 400-16, 392-16);
     attachEvent(EVENT_PAINT, [this](UIWidget *, int32_t, int32_t){
          internalDraw();
     });

     Button *button;

     button = new Button(ID_UPDATE, this);
     button->create(266, 8, 110, 32);
     button->setCaption("リストを更新");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          createFileList();
          refresh();
     });

     button = new Button(ID_RUN, this);
     button->create(266, 64, 110, 32);
     button->setCaption("実行");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          runScript();
     });

     button = new Button(ID_STOP, this);
     button->create(266, 104, 110, 32);
     button->setCaption("停止");
     button->attachEvent(EVENT_CLICKED, [this](UIWidget *, int32_t, int32_t){
          stopScript();
     });

     createFileList();

     Rect r = m_clientRect.clone();
     r.offset(8+1, 8+1).resize(246, 30);     
     for( int n = 0 ; n < MAX_FILES ; n++ )
     {
          m_itemRect.push_back(r.clone());
          r.offset(0, 30);
     }

     m_script = new Script(robot);
     m_script->onStart([this](Script *){
          m_mutex.lock();
          m_messages.clear();
          m_messages.push_back("started.");
          m_messageUpdated = true;
          m_mutex.unlock();
     });
     m_script->onEnd([this](Script *script){
          std::string msg = script->getErrorMessage();
          m_mutex.lock();
          if( msg.length() > 0 )
          {
               m_messages.push_back(msg);
          }
          m_messages.push_back("terminated.");
          m_messageUpdated = true;
          m_mutex.unlock();
     });
}

//------------------------------------------------------------------------------
ScriptView::~ScriptView()
{
     delete m_script;
}

//------------------------------------------------------------------------------
void ScriptView::update()
{
     if( !isVisible() || !isActive() )
     {
          return;
     }
     Button *runBtn = dynamic_cast<Button *>(getChildByID(ID_RUN));
     Button *stpBtn = dynamic_cast<Button *>(getChildByID(ID_STOP));
     if( m_script->isRunning() )
     {
          if( runBtn->isEnabled() )
          {
               runBtn->disable();
               runBtn->refresh();
          }
          if( !stpBtn->isEnabled() )
          {
               stpBtn->enable();
               stpBtn->refresh();
          }
     }
     else
     {
          if( m_selectedIndex >= 0 )
          {
               if( !runBtn->isEnabled() )
               {
                    runBtn->enable();
                    runBtn->refresh();
               }
          }
          else
          {
               if( runBtn->isEnabled() )
               {
                    runBtn->disable();
                    runBtn->refresh();
               }
          }
          if( stpBtn->isEnabled() )
          {
               stpBtn->disable();
               stpBtn->refresh();
          }
     }
     m_mutex.lock();
     if( m_messageUpdated )
     {
          Rect r = m_clientRect.clone();
          r.inflate(-8, 0).offset(0, 266).resizeHeight(16*6+4).inflate(-8, -8);
          fillRect(r, DEFAULT_FACE_COLOR);
          r.resizeHeight(20);          
          selectFont(SMALL_FONT);
          for( int n = 0 ; n < (int)m_messages.size() ; n++ )
          {
               drawText(r, m_messages[n].c_str(), ALIGN_LEFT|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR);
               r.offset(0, 20);
          }
          m_messageUpdated = false;
     }
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
void ScriptView::internalDraw()
{
     fillRect(m_clientRect, DEFAULT_FACE_COLOR);

     Rect r = m_clientRect.clone();
     r.inflate(-8, -8).resizeWidth(248).resizeHeight(30*MAX_FILES+2);
     drawRect(r, DEFAULT_BORDER_COLOR);
     for( int n = 0 ; n < (int)m_files.size() ; n++ )
     {
          drawFileItem(n);
     }

     r = m_clientRect.clone();
     r.inflate(-8, 0).offset(0, 266).resizeHeight(16*6+4);
     drawRect(r, DEFAULT_BORDER_COLOR);
     m_messageUpdated = true;
}

//------------------------------------------------------------------------------
void ScriptView::drawFileItem(int n)
{
     Rect r = m_itemRect[n];
     if( n == m_selectedIndex )
     {
          fillRect(r, 0x4382);
     }
     else
     {
          fillRect(r, DEFAULT_FACE_COLOR);
     }
     r.inflate(-8, 0);
     drawText(r, m_files[n].c_str(), ALIGN_LEFT|ALIGN_MIDDLE, DEFAULT_TEXT_COLOR);
}

//------------------------------------------------------------------------------
void ScriptView::onTouched(int16_t x, int16_t y)
{
     UIWidget::onTouched(x, y);

     Point pos(x, y);
     for( int n = 0 ; n < (int)m_files.size() ; n++ )
     {
          if( m_itemRect[n].include(pos) && n != m_selectedIndex )
          {
               int prev = m_selectedIndex;
               m_selectedIndex = n;
               drawFileItem(n);
               if( prev >= 0 )
               {
                    drawFileItem(prev);
               }
          }
     }
}
//------------------------------------------------------------------------------
void ScriptView::createFileList()
{
     m_files.clear();

     struct dirent **nameList;
     int num = scandir("./script", &nameList, fileFilter, alphasort);
     if( num < 0 )
     {
          return;
     }
     if( num > MAX_FILES )
     {
          num = MAX_FILES;
     }
     for( int n = 0 ; n < num ; n++ )
     {
          m_files.push_back(nameList[n]->d_name);
          free(nameList[n]);
     }
     free(nameList);
     m_selectedIndex = -1;
}

//------------------------------------------------------------------------------
void ScriptView::runScript()
{
     if( m_script->isRunning() || m_selectedIndex < 0 )
     {
          return;
     }
          
     std::string path = "./script/";
     path += m_files[m_selectedIndex]; 
     FILE *fp = fopen(path.c_str(), "r");
     if( !fp )
     {
          printf("[ScriptView] Unable to open %s\n", path.c_str());
          return;
     }
     std::string code;
     char buffer[256+1];
     int cb;
     while( (cb = fread(buffer, 1, 256, fp)) > 0 )
     {
          buffer[cb] = '\0';
          code += buffer;
     }
     fclose(fp);

     m_script->run(code);
}

//------------------------------------------------------------------------------
void ScriptView::stopScript()
{
     m_script->abort();
}

//------------------------------------------------------------------------------