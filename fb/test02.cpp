#include <signal.h>
#include <stdio.h>
#include <sstream>

#include "ui.h"
#include "arm_view.h"
#include "status_view.h"

typedef void (*sighandler_t)(int);
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

bool g_break = false;
void handler(int)
{
     g_break = true;
}


//------------------------------------------------------------------------------
int main()
{
     trap_signal(SIGINT, handler);

     Desktop *desktop = new Desktop();
     TouchManager *touchManager = new TouchManager();
     touchManager->pushEventListener(desktop);

     NumEdit().initialize(touchManager);

     PaintBox *box = new PaintBox(0, desktop);
     box->create(0, 0, 800, 32);
     box->attachEvent(EVENT_PAINT, [](UIWidget *sender, int32_t, int32_t){
          Rect r = sender->getClientRect();
          sender->fillRect(r, UIWidget::RGBToColor(0x14, 0x06, 0x3A));
          Point p(0, r.height-1);
          sender->drawFastHLine(p, r.width, UIWidget::RGBToColor(0x89, 0x7B, 0xAF));
          r.inflate(-16, 4);
          sender->selectFont(LARGE_FONT);
          sender->drawText(r, "Robotic Arm Console", ALIGN_LEFT|ALIGN_MIDDLE, UIWidget::RGBToColor(0x89, 0x7B, 0xAF));
          sender->selectFont(SMALL_FONT);
          sender->drawText(r, "192.168.128.32", ALIGN_RIGHT|ALIGN_MIDDLE, UIWidget::RGBToColor(0x89, 0x7B, 0xAF));
     });

     Tabbar *tabbar = new Tabbar(1, desktop);
     tabbar->create(8, 40, 400, 40);
     tabbar->addTab(0, "アーム", 80);
     tabbar->addTab(1, "グリッパー", 80);
     tabbar->addTab(2, "教示", 80);
     tabbar->addTab(3, "スクリプト", 80);
     tabbar->attachEvent(EVENT_SELECT_CHANGED, [](UIWidget *sender, int32_t index, int32_t){
          printf("tab changed : %d\n", (int)index);
     });

     Panel *panel = new Panel(2, desktop);
     panel->create(8, 80, 400, 392);
     panel->setBorder(true, false, true, true);

     ArmView *armView = new ArmView(panel);
     RobotStatusView *robotStatusView = new RobotStatusView(desktop);
     MotorStatusView *motorStatusView = new MotorStatusView(desktop);

     //
     // ToggleButton *toggle = new ToggleButton(4, panel);
     // toggle->create(220, 50, 100, 40);
     // toggle->setCaption("励磁");
     // toggle->attachEvent(EVENT_CLICKED, [](UIWidget *sender, int32_t, int32_t){
     //      ToggleButton *toggle = dynamic_cast<ToggleButton *>(sender);
     //      bool b = toggle->getState();
     //      printf("toggle changed : %s\n", b? "ON" : "OFF");
     // });
     //
     // NumberEditor *editor = new NumberEditor();
     //
     // Label *label = new Label(5, panel, LARGE_FONT);
     // label->create(100, 120, 150, 40);
     // label->setTextAlign(ALIGN_RIGHT|ALIGN_MIDDLE);
     // label->setColor(COLOR_YELLOW, COLOR_BLACK);
     // label->setValue("-12345");
     // label->attachEvent(EVENT_CLICKED, [&](UIWidget *sender, int32_t, int32_t){
     //      editor->open([&](UIWidget *sender, int32_t result, int32_t value){
     //           if( result )
     //           {
     //                std::ostringstream oss;
     //                oss << value;
     //                label->setValue(oss.str());
     //           }
     //           touchManager->popEventListener();
     //      });
     //      touchManager->pushEventListener(editor);
     // });
     //

     Button *button = new Button(3, desktop);
     button->create(692, 440, 100, 32);
     button->setCaption("終了");

     MessageBox *messageBox = new MessageBox();

     bool terminated = false;
     button->attachEvent(EVENT_CLICKED, [&](UIWidget *, int32_t, int32_t){
          messageBox->open(MBS_CONFIRM, "終了しても良いですか？", [&](UIWidget *, int32_t ok, int32_t){
               touchManager->popEventListener();
               if( ok )
               {
                    terminated = true;
               }
          });
          touchManager->pushEventListener(messageBox);
     });

     button = new Button(4, desktop, SMALL_FONT, BUTTONTYPE_DANGER);
     button->create(416, 440, 150, 32);
     button->setCaption("即時停止");

     desktop->show();
     desktop->refresh();

     touchManager->run();
     while( !terminated && !g_break )
     {
          touchManager->dispatchEvent();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }

     delete touchManager;
     delete messageBox;
     // delete editor;
     delete desktop;
     return 0;
}
