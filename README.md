# uStepper Robotic Arm for Raspberry Pi 
## Description
This software is designed to control uStepper's Robotic Arm Rev3.  
All sourecs are written in C++. To build, you should also install 'wiringPi' and 'lua devtool' to your Pi.  
This uses L6470 stepper driver for control the three stepping motors and all control commands are available using this software.  
In addition, this software provides HMI (Human Machine Interface) by using Pi's genuine 800x480 touch panel display. The Graphical UI is implemented by direct manipulation of framebuffer and low level touch input (called 'input subsystem'), so Pi's Desktop environment is not required.  

## IK and FK of robitics
This software implements IK (Inverse Kinematics) and FK (Forward Kinematics) algorithm designed for uStepper Robotic Arm Rev3.
By using UI, you can specify the end-effector's designated position directly (X, Y, Z coordinates of real 3D space), and can see current effector's position at realtime.

## Lua based motion script
You can describe and run 'script' for sequential motion of robot (move, pick and place). Script has original build-in function for robotics.  

## Network I/F
Pi which running robotic arm program is provides TCP/IP remote control features. You can create any kind of TCP/IP socket client that opens specified port and connects to Pi. 

## Requirements
- Raspberry Pi (2/3/Zero)
- Touch display (All kinds of gadgets are available as long as it has 800x480 resolution) 
