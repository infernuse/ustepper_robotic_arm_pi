#include "robot.h"
#include "L6470.h"
#include "command_server.h"
// #include "script.h"
#include "console.h"
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <cstdio>

//------------------------------------------------------------------------------
typedef void (*sighandler_t)(int);
bool g_terminated = false;
sighandler_t trap_signal(int sig, sighandler_t handler)
{
     struct sigaction act, old;
     act.sa_handler = handler;
     sigemptyset(&act.sa_mask);
     act.sa_flags = SA_RESTART;
     if( sigaction(sig, &act, &old) < 0 )
     {
          return NULL;
     }
     return old.sa_handler;
}
void handler(int)
{
     g_terminated = true;
}

//------------------------------------------------------------------------------
int main()
{
     trap_signal(SIGINT, handler);

     wiringPiSetupGpio();
     wiringPiSPISetupMode(L6470::SPI_CHANNEL, 1000000, 3);  // L6470 は「モード３」であることに注意！

     Robot *robot = new Robot();
     robot->initialize();

     CommandManager *commandManager = new CommandManager(robot);
     // Script *script = new Script(robot);

     // std::printf("RobotScript\n");
     RobotConsole *console = new RobotConsole(robot);

     while( !g_terminated && console->execute() )
     {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }

     delete console;
     delete commandManager;
     // delete script;
     delete robot;

     if( !g_terminated )
     {
          system("shutdown -h now");
     }

     return 0;
}

