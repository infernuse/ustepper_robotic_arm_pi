#include <iostream>
#include <wiringPi.h>

using namespace std;

int main()
{
     if( wiringPiSetupGpio() == -1 )
     {
          cerr << "Failed to setup GPIO I/F" << endl;
          return -1;
     }

     pinMode(18, PWM_OUTPUT);
     pwmSetMode(PWM_MODE_MS);
     pwmSetClock(400);
     pwmSetRange(1024);

     while( true )
     {
          int value;
          cin >> value;
          if( value <= 0 )
          {
               break;
          }
          pwmWrite(18, value);
     }
     return 0;
}
