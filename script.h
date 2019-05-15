#ifndef   SCRIPT_H
#define   SCRIPT_H

#include <thread>
#include <string>
#include <functional>
#include <lua.hpp>
#include "robot.h"

//------------------------------------------------------------------------------
class Script
{
     typedef std::function<void(Script *)>   EventHandler;
     private:
          static const char *STARTUP_CODE;
          static const char *GLOBAL_NAME;
          Robot *m_robot;
          bool m_running;
          bool m_terminated;
          bool m_aborted;
          std::thread *m_thread;
          std::string m_code;
          std::string m_errorMessage;
          EventHandler m_onStart;
          EventHandler m_onEnd;

          void execute();
          static int atPanic(lua_State *L);
          static void hookProc(lua_State *L, lua_Debug *ar);
          static int moveTo(lua_State *L);
          static int goHome(lua_State *L);
          static int grip(lua_State *L);
          static int delayScript(lua_State *L);
          static int inMotion(lua_State *L);
          static int alarmHappened(lua_State *L);
          static int getPosition(lua_State *L);
          static int exitScript(lua_State *L);

     public:
          Script(Robot *robot);
          ~Script();
          void onStart(EventHandler handler){
               m_onStart = handler;
          }
          void onEnd(EventHandler handler){
               m_onEnd = handler;
          }
          void run(std::string code);
          void abort();
          bool isRunning(){ return m_running; }
          std::string getErrorMessage(){ return m_errorMessage; }
};

#endif
