#include "script.h"
#include <lauxlib.h>
#include <lualib.h>
#include <wiringPi.h>
#include <cstdint>
#include <cstdio>
#include <regex>
#include <sstream>

//------------------------------------------------------------------------------
const char *Script::STARTUP_CODE =
     "\n"
     "print   = output_str\n"
     "debug   = nil\n"
     "require = nil\n"
     "package = nil\n"
     "os.exit = exit_script\n"
     "if type(main) == \"function\" then\n"
     "     main()\n"
     "else\n"
     "     error(\"main() が未定義です.\")\n"
     "end\n"
;
const char *Script::GLOBAL_NAME = "niwda_adwin";

//------------------------------------------------------------------------------
Script::Script(Robot *robot) : m_robot(robot), m_running(false),
     m_terminated(false), m_aborted(false)
{
     m_onStart = [](Script *){ 
          std::printf("[Script] started.\n"); 
     };
     m_onEnd  = [](Script *script){
          std::string msg = script->getErrorMessage();
          if( msg.length() > 0 )
          {
               std::printf("%s\n", msg.c_str());
          }
          std::printf("[Script] done.\n"); 
     };

     m_thread = new std::thread([this](){ execute(); });
}

//------------------------------------------------------------------------------
Script::~Script()
{
     m_terminated = true;
     m_aborted = true;
     m_thread->join();
     delete m_thread;
}

//------------------------------------------------------------------------------
void Script::execute()
{
     std::printf("[Script] thread started.\n");
     while( !m_terminated )
     {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          if( !m_running )
          {
               continue;
          }

          lua_State *pLua = luaL_newstate();
          luaL_openlibs(pLua);

          lua_pushlightuserdata(pLua, this);
          lua_setglobal(pLua, GLOBAL_NAME);

          lua_register(pLua, "moveto", &moveTo);
          lua_register(pLua, "go_home", &goHome);
          lua_register(pLua, "grip", &grip);
          lua_register(pLua, "delay", &delayScript);
          lua_register(pLua, "in_motion", &inMotion);
          lua_register(pLua, "alarm_hapenned", &alarmHappened);
          lua_register(pLua, "get_position", &getPosition);
          lua_register(pLua, "exit_script", &exitScript);
          lua_atpanic(pLua, &atPanic);
          lua_sethook(pLua, &hookProc, LUA_MASKCOUNT, 10);

          m_onStart(this);

          if( luaL_dostring(pLua, m_code.c_str()) )
          {
               atPanic(pLua);
          }
          else
          {
               // 正常終了
          }
          lua_close(pLua);
          m_running = false;
          m_aborted = false;
          m_onEnd(this);
          // if( m_errorMessage.length() > 0 )
          // {
          //      std::printf("%s\n", m_errorMessage.c_str());
          // }
     }
     std::printf("[Script] thread terminated.\n");
}

//------------------------------------------------------------------------------
void Script::run(std::string code)
{
     if( m_running )
     {
          return;
     }

     m_code = code + STARTUP_CODE;
     m_errorMessage = "";
     m_aborted = false;
     m_running = true;
}

//------------------------------------------------------------------------------
void Script::abort()
{
     m_aborted = true;
}

//------------------------------------------------------------------------------
int Script::atPanic(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     if( self->m_aborted || self->m_terminated )
     {
          return 0;
     }

     std::regex re("\\[.+\\]:(\\d+):\\s*(.+)");
     std::cmatch cm;
     std::string msg(lua_tostring(L, 1));
     std::regex_match(msg.c_str(), cm, re);
     if( cm.empty() )
     {
          self->m_errorMessage = msg;
     }
     else
     {
          std::ostringstream oss;
          oss << "ERROR [line " << cm[1] << "] " << cm[2];
          self->m_errorMessage = oss.str();
     }
     // self->m_errorMessage = std::string(lua_tostring(L, 1));

     return 0;
}

//------------------------------------------------------------------------------
void Script::hookProc(lua_State *L, lua_Debug *ar)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     std::this_thread::sleep_for(std::chrono::milliseconds(5));

     if( self->m_aborted || self->m_terminated )
     {
          luaL_error(L, "aborted.");
     }
}

//------------------------------------------------------------------------------
int Script::moveTo(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     double x = luaL_checknumber(L, 1);
     double y = luaL_checknumber(L, 2);
     double z = luaL_checknumber(L, 3);

     int32_t b, s, e;
     if( !Robot::coordToMotorPos(x, y, z, &b, &s, &e) )
     {
          return luaL_error(L, "moveto - Designated position is out of range");
     }
     if( !self->m_robot->startMotion3D(b, s, e) )
     {
          return luaL_error(L, "moveto - Unable to start motion");
     }
     std::this_thread::sleep_for(std::chrono::milliseconds(100));
     return 0;
}

//------------------------------------------------------------------------------
int Script::goHome(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     if( !self->m_robot->startMotion3D(0, 0, 0) )
     {
          return luaL_error(L, "go_home - Unable to start motion");
     }
     return 0;
}

//------------------------------------------------------------------------------
int Script::grip(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     int value = (int)luaL_checknumber(L, 1);
     if( value < 0 || 100 < value )
     {
          return luaL_error(L, "grip - Out of range (%d)", value);
     }

     self->m_robot->moveGripper((uint8_t)value);
     return 0;
}

//------------------------------------------------------------------------------
int Script::delayScript(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     uint32_t value = (uint32_t)luaL_checknumber(L, 1);
     if( value == 0 || 60000 < value )
     {
          return luaL_error(L, "delay - Out of range (%d)", value);
     }

     uint32_t timeout = millis() + value;
     while( (millis() < timeout) && !self->m_aborted )
     {
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
     }
     return 0;
}

//------------------------------------------------------------------------------
int Script::inMotion(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     int b = self->m_robot->isInMotion()? 1 : 0;

     lua_pushboolean(L, b);
     return 1;
}

//------------------------------------------------------------------------------
int Script::alarmHappened(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     int b = self->m_robot->isAlarmHappened()? 1 : 0;

     lua_pushboolean(L, b);
     return 1;
}

//------------------------------------------------------------------------------
int Script::getPosition(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     int32_t pos[3];
     double x, y, z;
     for( int n = 0 ; n < 3 ; n++ )
     {
          pos[n] = self->m_robot->getMotorPosition(n);
     }
     Robot::motorPosToCoord(pos[0], pos[1], pos[2], &x, &y, &z);
     
     // 戻り値を格納するためのテーブルを作成する
     lua_newtable(L);

     // 以下，テーブル（連想配列）の「キー」「値」の順にスタックに積み
     // それをテーブルへ格納する，という手順を繰り返す
     lua_pushstring(L, "x");
     lua_pushnumber(L, x);
     lua_settable(L, -3);

     lua_pushstring(L, "y");
     lua_pushnumber(L, y);
     lua_settable(L, -3);

     lua_pushstring(L, "z");
     lua_pushnumber(L, z);
     lua_settable(L, -3);
     
     return 1; // テーブルはスタックのトップにある
}

//------------------------------------------------------------------------------
int Script::exitScript(lua_State *L)
{
     lua_getglobal(L, GLOBAL_NAME);
     Script *self = (Script *)lua_touserdata(L, -1);
     lua_pop(L, 1);

     lua_sethook(L, &hookProc, LUA_MASKLINE, 0);
     self->m_aborted = true;
     return 0;
}
