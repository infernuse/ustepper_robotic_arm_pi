#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <iostream>
#include <cstdint>

bool pollTouchEvent(int fd, input_event& ev)
{
     fd_set mask;
     struct timeval timeout;
     FD_ZERO(&mask);
     FD_SET(fd, &mask);
     timeout.tv_sec = 0;
     timeout.tv_usec = 20000;
     int ret = select(fd+1, &mask, NULL, NULL, &timeout);
     if( ret > 0 && FD_ISSET(fd, &mask) )
     {
          read(fd, &ev, sizeof(ev));
          return true;
     }
     return false;
}

int main()
{
     int fd = open("/dev/input/event0", O_RDONLY);
     if( fd < 0 )
     {
          printf("Cannot open /dev/input/event0\n");
          return -1;
     }

     input_event ev;
     int16_t x = 0, y = 0;
     bool terminated = false;

     while( !terminated )
     {
          if( !pollTouchEvent(fd, ev) )
          {
               continue;
          }
          switch( ev.type )
          {
               case EV_KEY:   // (1)タッチパネルの「押された」「離された」を検出
                    if( ev.code == BTN_TOUCH )    // 330
                    {
                         if( ev.value )
                         { 
                              std::cout << "Touched" << std::endl;
                              x = y = 0; 
                         }
                         else
                         {
                              std::cout << "Released" << std::endl;
                              if( x > 750 && y > 430 )
                              {
                                   terminated = true;
                              }
                         }
                    }
                    break;
               case EV_ABS:   // 3
                    if( ev.code == ABS_X )
                    {
                         std::cout << "  X-Pos : " << ev.value << std::endl;
                         x = ev.value;
                    }
                    if( ev.code == ABS_Y )
                    {
                         std::cout << "  Y-Pos : " << ev.value << std::endl;
                         y = ev.value;
                    }
          }
     }

     close(fd);
     return 0;
}
