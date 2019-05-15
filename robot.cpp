#include <cstdio>
#include <chrono>
#include <cmath>
#include <functional>
#include <wiringPi.h>
#include "robot.h"

//------------------------------------------------------------------------------
//   コンストラクタ
//------------------------------------------------------------------------------
Robot::Robot()
     : m_terminated(false), m_homingState(0),
     m_homingThread(nullptr), m_motionThread(nullptr), m_servoThread(nullptr)
{
     m_stepper[MOTOR_BASE] = nullptr;
     m_stepper[MOTOR_SHOULDER] = nullptr;
     m_stepper[MOTOR_ELBOW] = nullptr;

     m_motionState[MOTOR_BASE] = 0;
     m_motionState[MOTOR_SHOULDER] = 0;
     m_motionState[MOTOR_ELBOW] = 0;

     // if( wiringPiSetupGpio() < 0 )
     // {
     //      cerr << "Failed to setup GPIO I/F." << endl;
     //      std::exit(-1);
     // }
     
     // initialize();
}

//------------------------------------------------------------------------------
//   デストラクタ
//------------------------------------------------------------------------------
Robot::~Robot()
{
     m_terminated = true;
     if( m_homingThread )
     {
          m_homingThread->join();
          delete m_homingThread;
     }
     if( m_motionThread )
     {
          m_motionThread->join();
          delete m_motionThread;
     }
     if( m_servoThread )
     {
          m_servoThread->join();
          delete m_servoThread;
     }
     delete m_stepper[MOTOR_BASE];
     delete m_stepper[MOTOR_SHOULDER];
     delete m_stepper[MOTOR_ELBOW];
}


//------------------------------------------------------------------------------
//   初期化
//   ※ 先に 
//        wiringPiSetupGpio() と
//        wiringPiSPISetup(0, 1000000) を実行してから呼び出すこと
//------------------------------------------------------------------------------
void Robot::initialize()
{
     pinMode(BASE_NLIM, INPUT);
     pinMode(BASE_PLIM, INPUT);
     pinMode(SHOULDER_PLIM, INPUT);
     pinMode(ELBOW_PLIM, INPUT);

     m_stepper[MOTOR_BASE    ] = new L6470(BASE_CS, BASE_BUSY, 
          std::function<uint8_t(int)>([this](int dir){ return getLimitState(MOTOR_BASE, dir); }));
     m_stepper[MOTOR_SHOULDER] = new L6470(SHOULDER_CS, SHOULDER_BUSY, 
          std::function<uint8_t(int)>([this](int dir){ return getLimitState(MOTOR_SHOULDER, dir); }));
     m_stepper[MOTOR_ELBOW   ] = new L6470(ELBOW_CS, ELBOW_BUSY, 
          std::function<uint8_t(int)>([this](int dir){ return getLimitState(MOTOR_ELBOW, dir); }));

     pinMode(SERVO_PIN, PWM_OUTPUT);
     pwmSetMode(PWM_MODE_MS);
     pwmSetClock(400);
     pwmSetRange(1024);
     m_gripperCurrentValue = m_gripperDestValue = SERVO_MAX_VALUE;
     pwmWrite(SERVO_PIN, m_gripperCurrentValue);

     m_homingThread = new std::thread([this](){ execHoming(); });
     m_motionThread = new std::thread([this](){ execMotion(); });
     m_servoThread  = new std::thread([this](){ execServo(); });
}

//------------------------------------------------------------------------------
//   各軸のリミット信号の状態を返す
//   戻り値は Low(0)  が ON (リミットを叩いている)
//            High(1) が OFF (リミットには達していない)
//------------------------------------------------------------------------------
uint8_t Robot::getLimitState(int axis, int dir)
{
     uint8_t state = 1;
     switch( axis )
     {
          case MOTOR_BASE:
               if( dir == L6470::DIR_REVERSE )
               {
                    state = digitalRead(BASE_NLIM);
               }
               else
               {
                    state = digitalRead(BASE_PLIM);
               }
               break;
          case MOTOR_SHOULDER:
               if( dir == L6470::DIR_REVERSE )
               {
                    state = 1;     // マイナスリミットはL6470のSWへ接続されるため，ここでは監視できない．
               }
               else
               {
                    state = !digitalRead(SHOULDER_PLIM);    // 反転させる
               }
               break;
          case MOTOR_ELBOW:
               if( dir == L6470::DIR_REVERSE )
               {
                    state = 1;     // マイナスリミットはL6470のSWへ接続されるため，ここでは監視できない．
               }
               else
               {
                    state = !digitalRead(ELBOW_PLIM);  // 反転させる
               }
               break;
     }
     return state;
}

//------------------------------------------------------------------------------
//   軸が動作中かどうか調べる
//------------------------------------------------------------------------------
bool Robot::isInMotion(int axis)
{ 
     bool b;

     m_mutex.lock();
     if( axis < 0 || axis >= 3 )
     {
          b = (m_homingState > 0) || (m_motionState[MOTOR_BASE] > 0) || (m_motionState[MOTOR_ELBOW] > 0) || (m_motionState[MOTOR_SHOULDER] > 0);
     }
     else
     {
          b = (m_homingState > 0) || (m_motionState[axis] > 0); 
     }
     m_mutex.unlock();

     return b;
}

//------------------------------------------------------------------------------
bool Robot::isAlarmHappened(int axis)
{
     bool b = false;

     m_mutex.lock();
     for( int n = 0 ; n < 3 ; n++ )
     {
          if( n == axis || axis < 0 )
          {
               b = b || m_stepper[n]->isAlarmHappened();
          }
     }
     m_mutex.unlock();

     return b;
}

//------------------------------------------------------------------------------
bool Robot::isHalted(int axis)
{
     m_mutex.lock();
     bool b = m_stepper[axis]->isHalted();
     m_mutex.unlock();
     return b;
}

//------------------------------------------------------------------------------
bool Robot::isHomeCompleted(int axis)
{
     bool b[3];
     m_mutex.lock();
     for( int n = 0 ; n < 3 ; n++ )
     {
          b[n] = m_stepper[n]->isHomeCompleted() && !m_stepper[n]->isHalted();
     }
     m_mutex.unlock();

     if( 0 <= axis && axis < 3 )
     {
          return b[axis];
     }
     return b[0] && b[1] && b[2];
}

//------------------------------------------------------------------------------
uint8_t Robot::getAlarmFlag(int axis)
{
     m_mutex.lock();
     uint8_t f = m_stepper[axis]->getAlarmFlag();
     m_mutex.unlock();
     return f;
}

//------------------------------------------------------------------------------
void Robot::enableMotor(int axis, bool ena)
{
     m_mutex.lock();
     if( ena )
     {
          m_stepper[axis]->hardStop();
     }
     else
     {
          m_stepper[axis]->hardHIZ();
     }
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
void Robot::clearAlarm(int axis)
{
     m_mutex.lock();
     for( int n = 0 ; n < 3 ; n++ )
     {
          if( n == axis || axis < 0 )
          {
               m_stepper[n]->clearAlarm();
          }
     }
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
void Robot::softStop(int axis)
{
     m_mutex.lock();
     for( int n = 0 ; n < 3 ; n++ )
     {
          if( n == axis || axis < 0 )
          {
               m_stepper[n]->softStop();
          }
     }
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
void Robot::hardStop(int axis)
{
     m_mutex.lock();
     for( int n = 0 ; n < 3 ; n++ )
     {
          if( n == axis || axis < 0 )
          {
               m_stepper[n]->hardStop();
          }
     }
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
//   原点復帰を開始
//------------------------------------------------------------------------------
bool Robot::startHoming()
{
     if( isInMotion() )
     {
          return false;
     }

     m_mutex.lock();
     m_homingState = 1;
     m_mutex.unlock();

     return true;
}

//------------------------------------------------------------------------------
//   原点復帰処理スレッド
//------------------------------------------------------------------------------
void Robot::execHoming()
{
     uint32_t timeout;

     std::printf("[Robot] homing thread started.\n");

     while( !m_terminated )
     {
          m_mutex.lock();

          switch( m_homingState )
          {
               case 0:
                    break;
               case 1:
                    // いずれかの軸にアラームが発生している場合は実行不可
                    if( m_stepper[MOTOR_BASE]->isAlarmHappened() || m_stepper[MOTOR_SHOULDER]->isAlarmHappened() || m_stepper[MOTOR_ELBOW]->isAlarmHappened() )
                    {
                         m_homingState = 0;
                         break;
                    }
                    // BASE を励磁
                    m_stepper[MOTOR_BASE]->hardStop();

                    // SHOULDER を励磁
                    m_stepper[MOTOR_SHOULDER]->hardStop();

                    // ELBOW を原点復帰開始
                    m_stepper[MOTOR_ELBOW]->startHoming(L6470::DIR_REVERSE, 1024);
                    m_homingState++;
                    break;
               case 2:
                    if( m_stepper[MOTOR_ELBOW]->getHomingState() > 0 )
                    {
                         break;
                    }
                    if( !m_stepper[MOTOR_ELBOW]->isHomeCompleted() )
                    {
                         std::printf("Failed to home ELBOW axis (alarm = 0x%02X)\n", m_stepper[MOTOR_ELBOW]->getAlarmFlag());
                         m_homingState = 0;
                    }
                    timeout = millis() + 1000;
                    m_homingState++;
                    break;
               case 3:
                    if( millis() >= timeout )
                    {
                         // BASE と SHOULDER を原点復帰（これらは同時に行う）
                         m_stepper[MOTOR_BASE]->startHoming(L6470::DIR_FORWARD, 1024);
                         m_stepper[MOTOR_SHOULDER]->startHoming(L6470::DIR_FORWARD, 1024);
                         m_homingState++;
                    }
                    break;
               case 4:
                    if( (m_stepper[MOTOR_BASE]->getHomingState() > 0) || (m_stepper[MOTOR_SHOULDER]->getHomingState() > 0) )
                    {
                         break;
                    }
                    // いずれか一方の軸が原点復帰を完了できなかった場合、他方の軸も即時停止させる
                    if( (m_stepper[MOTOR_BASE]->getHomingState() <= 0) && !m_stepper[MOTOR_BASE]->isHomeCompleted() )
                    {
                         std::printf("Failed to home BASE axis (alarm = 0x%02X)\n", m_stepper[MOTOR_BASE]->getAlarmFlag());
                         if( m_stepper[MOTOR_SHOULDER]->isInMotion() )
                         {
                              m_stepper[MOTOR_SHOULDER]->hardStop();
                         }
                    }
                    if( (m_stepper[MOTOR_SHOULDER]->getHomingState() <= 0) && !m_stepper[MOTOR_SHOULDER]->isHomeCompleted() )
                    {
                         std::printf("Failed to home SHOULDER axis (alarm = 0x%02X)\n", m_stepper[MOTOR_SHOULDER]->getAlarmFlag());
                         if( m_stepper[MOTOR_BASE]->isInMotion() )
                         {
                              m_stepper[MOTOR_BASE]->hardStop();
                         }
                    }
                    m_homingState = 0;
                    break;
          }
          m_mutex.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
     }

     std::printf("[Robot] homing thread terminated.\n");
}

//------------------------------------------------------------------------------
//   軸の動作遷移監視スレッド
//------------------------------------------------------------------------------
void Robot::execMotion()
{
     std::printf("[Robot] motion thread started.\n");

     while( !m_terminated )
     {
          for( int axis = 0 ; axis < 3 ; axis++ )
          {
               std::this_thread::sleep_for(std::chrono::milliseconds(10));

               m_mutex.lock();
               
               m_stepper[axis]->execControl();

               switch( m_motionState[axis] )
               {
                    case 0:
                         break;
                    case 1:
                         if( m_stepper[axis]->isInMotion() )
                         {
                              break;
                         }
                         if( m_stepper[axis]->isAlarmHappened() )
                         {
                              std::printf("[AXIS-%d] Alarm : 0x%02X\n", axis, m_stepper[axis]->getAlarmFlag());
                              m_stepper[MOTOR_BASE]->hardHIZ();
                              m_stepper[MOTOR_SHOULDER]->hardHIZ();
                              m_stepper[MOTOR_ELBOW]->hardHIZ();
                         }
                         m_motionState[axis] = 0;
                         break;
               }
               m_mutex.unlock();
          }
     }

     std::printf("[Robot] motion thread terminated.\n");
}

//------------------------------------------------------------------------------
//   軸の駆動を開始する
//------------------------------------------------------------------------------
bool Robot::startMotion(int axis, int32_t destpos)
{
     if( axis < 0 || 3 <= axis )
     {
          return false;
     }
     
     if( m_stepper[axis]->isHalted() )
     {
          // 軸の励磁が切れている、または軸が駆動中、またはアラーム発生中
          std::printf("[AXIS-%d] Unable to start motion : halted.\n", axis);
          return false;
     }
     if( m_stepper[axis]->isAlarmHappened() )
     {
          // 軸の励磁が切れている、または軸が駆動中、またはアラーム発生中
          std::printf("[AXIS-%d] Unable to start motion : alarm (0x%02X)\n", axis, m_stepper[axis]->getAlarmFlag());
          return false;
     }

     if( isInMotion(axis) )
     {
          // 原点復帰中、または、指定された軸が駆動中
          std::printf("[AXIS-%d] Unable to start motion - motor busy.\n", axis);
          return false;
     }

     m_mutex.lock();
     uint8_t dir = (m_stepper[axis]->getAbsPos() > destpos)? L6470::DIR_REVERSE : L6470::DIR_FORWARD; 
     m_stepper[axis]->moveTo(dir, destpos);
     m_motionState[axis] = 1;
     m_mutex.unlock();

     return true;
}

//------------------------------------------------------------------------------
bool Robot::startMotion3D(int32_t base, int32_t shoulder, int32_t elbow)
{
     if( !canMove() )
     {
          return false;
     }

     int32_t destpos[3];
     uint32_t distance[3];
     uint32_t longest = 0;
     bool b[3] = {true, true, true};
     uint8_t dir[3];

     destpos[0] = base;
     destpos[1] = shoulder;
     destpos[2] = elbow;

     for( int axis = 0 ; axis < 3 ; axis++ )
     {
          int32_t abspos = getMotorPosition(axis);
          if( abspos == destpos[axis] )
          {
               b[axis] = false;    // この軸は動かす必要はない
               continue;
          }
          if( destpos[axis] > abspos )
          {
               dir[axis] = L6470::DIR_FORWARD;
          }
          else
          {
               dir[axis] = L6470::DIR_REVERSE;
          }
          distance[axis] = std::abs(destpos[axis] - abspos);
          if( distance[axis] > longest )
          {
               longest = distance[axis];
          }
     }
     if( longest == 0 )
     {
          return true;   // どの軸も動かす必要がない
     }
     // 各軸の速度を決定し，駆動開始
     m_mutex.lock();
     for( int axis = 0 ; axis < 3 ; axis++ )
     {
          if( b[axis] )
          {
               int32_t speed = 16 * distance[axis] / longest;
               m_stepper[axis]->setParam(L6470::PRM_MAX_SPEED, speed);
               m_stepper[axis]->moveTo(dir[axis], destpos[axis]);
               m_motionState[axis] = 1;
          }
     }
     m_mutex.unlock();
     return true;
}

//------------------------------------------------------------------------------
//   現在のモータのステータスを返す
//------------------------------------------------------------------------------
uint16_t Robot::getMotorStatus(int axis)
{
     m_mutex.lock();
     uint16_t status = m_stepper[axis]->getStatus();
     m_mutex.unlock();
     return status;
}

//------------------------------------------------------------------------------
//   現在のモータの軸位置(pulse)を返す
//------------------------------------------------------------------------------
int32_t Robot::getMotorPosition(int axis)
{
     m_mutex.lock();
     int32_t pos = m_stepper[axis]->getAbsPos();
     m_mutex.unlock();
     return pos;
}

//------------------------------------------------------------------------------
//   現在のモータの回転速度(pulse/sec)を返す
//------------------------------------------------------------------------------
int32_t Robot::getMotorSpeed(int axis)
{
     m_mutex.lock();
     int32_t spd = m_stepper[axis]->getSpeed();
     m_mutex.unlock();
     return spd;
}

//------------------------------------------------------------------------------
//   モータドライバのパラメータを取得
//------------------------------------------------------------------------------
uint32_t Robot::getMotorParam(int axis, uint8_t id)
{
     m_mutex.lock();
     uint32_t value = m_stepper[axis]->getParam(id);
     m_mutex.unlock();
     return value;     
}

//------------------------------------------------------------------------------
//   モータドライバのパラメータを設定
//------------------------------------------------------------------------------
void Robot::setMotorParam(int axis, uint8_t id, uint32_t value)
{
     m_mutex.lock();
     m_stepper[axis]->setParam(id, value);
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
//   グリッパー（サーボ）を動かす
//   value はグリッパーの開度をパーセントで指定する
//   0 は閉じた状態，100は全開の状態
//   ※グリッパー自体を動かしている最中や，軸が駆動中であっても使える
//------------------------------------------------------------------------------
void Robot::moveGripper(uint8_t value)
{
     m_mutex.lock();
     value = (value > 100)? 100 : value;
     m_gripperDestValue = SERVO_MIN_VALUE + (SERVO_MAX_VALUE - SERVO_MIN_VALUE)*value/100;
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
//   動作中のグリッパーを即時停止させる
//------------------------------------------------------------------------------
void Robot::stopGripper()
{
     m_mutex.lock();
     m_gripperDestValue = m_gripperCurrentValue;
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
//   グリッパー（サーボモータ）制御スレッド
//------------------------------------------------------------------------------
void Robot::execServo()
{
     std::printf("[Robot] servo thread started.\n");

     while( !m_terminated )
     {
          m_mutex.lock();
          if( m_gripperCurrentValue != m_gripperDestValue )
          {
               int delta = (m_gripperCurrentValue < m_gripperDestValue)? 1 : -1;
               m_gripperCurrentValue += delta;
               pwmWrite(SERVO_PIN, m_gripperCurrentValue);
          }
          m_mutex.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(25));
     }

     std::printf("[Robot] servo thread terminated.\n");
}

//------------------------------------------------------------------------------
//   エンドエフェクタ位置(mm単位)から，モータ軸位置(pulse単位)を算出する
//------------------------------------------------------------------------------
bool Robot::coordToMotorPos(double X, double Y, double Z, int32_t *base, int32_t *shoulder, int32_t *elbow)
{
     double a = 180;
     double b = 68;
     double c = 180;
     double d = 30;
     double e = 60;
     double f = 175;
     double r = std::sqrt(d*d + e*e);
     double m = 10;
     double n = 40;

     const double PI = 3.14159265359;
     
     X = X + m;

     double t = std::asin(X/std::sqrt(X*X+Y*Y));
     double omega = std::asin(m/std::sqrt(X*X+Y*Y)) - t;
     *base = (int32_t)(25600.0 * (omega / PI) * 45.0 / 21.0);

     double x = -(-X*std::sin(omega) + Y*std::cos(omega) - n - 135);     // 135(mm) は，「P点」とエンドエフェクタとの水平距離

     double y = Z - 85;

     double s = std::acos(x/std::sqrt(x*x+y*y));
     if( y < 0 )
     {
          s = 2*PI - s;
     }
     
     double alpha = s - std::acos((a*a+x*x+y*y-(d*d+f*f))/(2*a*std::sqrt(x*x+y*y))); 
     
     double theta = PI + alpha - std::acos((a*a+d*d+f*f-(x*x+y*y))/(2*a*std::sqrt(d*d+f*f)))
                    - std::acos((d*d-e*f)/(std::sqrt(d*d+f*f)*std::sqrt(d*d+e*e)));

     t = a*a+r*r+2*a*r*std::cos(alpha - theta);
     
     double u = std::acos((a*std::cos(alpha)+r*std::cos(theta))/std::sqrt(t));
     
     double beta = u - std::acos((t+b*b-c*c)/(2*b*std::sqrt(t)));

     *shoulder = (int32_t)(25600.0*((alpha - PI/3)/PI) * 45 / 11);
     *elbow = (int32_t)(25600.0*((beta-0.6632)/PI) * 45 / 11);

     if( std::abs(*base) > 21000 )
     {
          return false;
     }
     if( *shoulder < 0 || *elbow < 0 || *shoulder < *elbow || *shoulder > 51200 )
     {
          return false;
     }
     return true;
}

//------------------------------------------------------------------------------
//   モータの軸位置(pulse単位)から，エンドエフェクタ位置(mm単位)を算出する
//------------------------------------------------------------------------------
void Robot::motorPosToCoord(int32_t base, int32_t shoulder, int32_t elbow, double *X, double *Y, double *Z)
{
     double a = 180;
     double b = 68;
     double c = 180;
     double d = 30;
     double e = 60;
     double f = 175; 
     double h = 100;  //アームの支点の高さ
     double l1 =135;
     double l2 =15;

     const double PI = 3.14159265359;
     
     double alpha = 11*shoulder*2*PI/2304000 + PI/3;
     double beta  = 11*elbow*2*PI/2304000 + 38*PI/180;
     double phi   = 21*base*2*PI/2304000;
     
     double r = std::sqrt(d*d+e*e);
     double s = a*std::cos(alpha) - b*std::cos(beta);
     double t = a*std::sin(alpha)-b*std::sin(beta);

     double gamma;
     if( t > 0 )
     {
          gamma = std::acos(s/std::sqrt(s*s+t*t));
     }
     else
     {
          gamma = 2*PI - std::acos(s/std::sqrt(s*s+t*t));
     }
     double theta = gamma - std::acos((c*c-(a*a+b*b+d*d+e*e)+2*a*b*std::cos(alpha -beta))/(2*std::sqrt(d*d+e*e)*std::sqrt(s*s+t*t)));
     
     const double ROOT17 = std::sqrt(17);

     double x = -(-l1+a*std::cos(alpha)+((d*d-e*f)*std::cos(theta) - d*(e+f)*std::sin(theta))/r)*(4*std::cos(phi)-std::sin(phi))/ROOT17 + 10*ROOT17*std::cos(phi);
     double y = -(-l1+a*std::cos(alpha)+((d*d-e*f)*std::cos(theta) - d*(e+f)*std::sin(theta))/r)*(std::cos(phi)+4*std::sin(phi))/ROOT17 + 10*ROOT17*std::sin(phi);                //点Pからアームの位置の差を調整した値（-135、-15）
     double z = a*std::sin(alpha)+(d*(e+f)*std::cos(theta)+(d*d-e*f)*std::sin(theta))/r + h - l2;
     
     *X = x/ROOT17-4*y/ROOT17 -10;
     *Y = 4*x/ROOT17+y/ROOT17;
     *Z = z;
}
