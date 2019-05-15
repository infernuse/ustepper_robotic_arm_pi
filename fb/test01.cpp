#include "ui.h"
#include <unistd.h>

int main()
{
     Desktop *desktop = new Desktop();
     TouchManager *touchManager = new TouchManager();
     touchManager->addEventListener(desktop);

     Button *button = new Button(1, desktop);
     button->create(100, 100, 100, 50);
     button->setCaption("Click me");

     bool terminated = false;

     button->attachEvent(EVENT_BTN_CLICKED, [](uint32_t p1, uint32_t p2){
          terminated = true;
     });

     desktop->show();

     while( !terminated )
     {
          touchManager->dispatchEvent();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }

     delete touchManager;
     delete desktop;
     return 0;
}

