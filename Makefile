robotic_arm: robotic_arm.o robot.o command_server.o L6470.o script.o gfxpi.o ui.o arm_view.o gripper_view.o teaching_view.o script_view.o status_view.o console.o
	g++ -o robotic_arm robotic_arm.o robot.o command_server.o L6470.o script.o gfxpi.o ui.o arm_view.o gripper_view.o teaching_view.o script_view.o status_view.o console.o -lpthread -lwiringPi -llua5.1
robotic_arm.o: robotic_arm.cpp robot.h L6470.h command_server.h script.h console.h ui.h gfxpi.h arm_view.h gripper_view.h teaching_view.h script_view.h status_view.h 
	g++ -c -I/usr/include/lua5.1 robotic_arm.cpp
robot.o: robot.cpp robot.h L6470.h
	g++ -c robot.cpp
command_server.o: command_server.cpp command_server.h robot.h L6470.h
	g++ -c command_server.cpp
L6470.o: L6470.cpp L6470.h
	g++ -c L6470.cpp
script.o: script.cpp script.h robot.h L6470.h
	g++ -c -I/usr/include/lua5.1 script.cpp
gfxpi.o: gfxpi.cpp gfxpi.h
	g++ -c gfxpi.cpp
ui.o: ui.cpp ui.h gfxpi.h
	g++ -c ui.cpp
arm_view.o: arm_view.cpp arm_view.h ui.h gfxpi.h robot.h L6470.h
	g++ -c arm_view.cpp
gripper_view.o: gripper_view.cpp gripper_view.h ui.h gfxpi.h
	g++ -c gripper_view.cpp
teaching_view.o: teaching_view.cpp teaching_view.h ui.h gfxpi.h robot.h L6470.h
	g++ -c teaching_view.cpp
script_view.o: script_view.cpp script_view.h ui.h gfxpi.h script.h robot.h L6470.h
	g++ -c -I/usr/include/lua5.1 script_view.cpp
status_view.o: status_view.cpp status_view.h ui.h gfxpi.h robot.h L6470.h
	g++ -c status_view.cpp
console.o: console.cpp console.h robot.h L6470.h ui.h gfxpi.h script.h arm_view.h gripper_view.h teaching_view.h script_view.h status_view.h
	g++ -c -I/usr/include/lua5.1 console.cpp
clean:; rm -f *.o *~ robotic_arm