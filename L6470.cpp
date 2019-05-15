#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <cmath>
#include "L6470.h"

const uint8_t L6470::PARAM_ADDR[32] = {
     0x01,     // PRM_ABS_POS
     0x02,     // PRM_EL_POS
     0x03,     // PRM_MARK
     0x04,     // PRM_SPEED
     0x05,     // PRM_ACC
     0x06,     // PRM_DEC
     0x07,     // PRM_MAX_SPEED
     0x08,     // PRM_MIN_SPEED
     0x09,     // PRM_KVAL_HOLD
     0x0A,     // PRM_KVAL_RUN
     0x0B,     // PRM_KVAL_ACC
     0x0C,     // PRM_KVAL_DEC
     0x0D,     // PRM_INT_SPEED
     0x0E,     // PRM_ST_SLP
     0x0F,     // PRM_FN_SLP_ACC
     0x10,     // PRM_FN_SLP_DEC
     0x11,     // PRM_K_THERM
     0x12,     // PRM_ADC_OUT
     0x13,     // PRM_OCD_TH
     0x14,     // PRM_STALL_TH
     0x15,     // PRM_FS_SPD
     0x16,     // PRM_STEP_MODE
     0x17,     // PRM_ALARM_EN
     0x18,     // PRM_CONFIG
     0x19,     // PRM_STATUS
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
};

const uint8_t L6470::PARAM_SIZE[32] = {
     3,   // PRM_ABS_POS      Current position                   (22 bit)
     2,   // PRM_EL_POS       Electrical position                ( 9 bit)
     3,   // PRM_MARK         Mark position                      (22 bit)
     3,   // PRM_SPEED        Current speed                      (20 bit)
     2,   // PRM_ACC          Acceleration                       (12 bit)
     2,   // PRM_DEC          Deceleration                       (12 bit)
     2,   // PRM_MAX_SPEED    Maximum speed                      (10 bit)
     2,   // PRM_MIN_SPEED    Minimum speed                      (13 bit)
     1,   // PRM_KVAL_HOLD    Holding Kval                       ( 8 bit)
     1,   // PRM_KVAL_RUN     Constant speed Kval                ( 8 bit)
     1,   // PRM_KVAL_ACC     Acceleration starting Kval         ( 8 bit)
     1,   // PRM_KVAL_DEC     Deceleration starting Kval         ( 8 bit)
     2,   // PRM_INT_SPEED    Intersect speed                    (14 bit)
     1,   // PRM_ST_SLP       Start slope                        ( 8 bit)
     1,   // PRM_FN_SLP_ACC   Acceleration final slope           ( 8 bit)
     1,   // PRM_FN_SLP_DEC   Deceleration final slope           ( 8 bit)
     1,   // PRM_K_THERM      Thermal compensation factor        ( 4 bit)
     1,   // PRM_ADC_OUT      ADC Output                         ( 5 bit)
     1,   // PRM_OCD_TH       OCD threshold                      ( 4 bit)
     1,   // PRM_STALL_TH     STALL threshold                    ( 7 bit)
     2,   // PRM_FS_SPD       Full-step speed                    (10 bit)
     1,   // PRM_STEP_MODE    Step mode                          ( 8 bit)
     1,   // PRM_ALARM_EN     Alarm enable                       ( 8 bit)
     2,   // PRM_CONFIG       IC configuration                   (16 bit)
     2,   // PRM_STATUS       Status                             (16 bit)
     0,
     0,
     0,
     0,
     0,
     0,
     0,
};

const uint8_t L6470::PARAM_ATTR[32] = {
     L6470::ATTR_R_WS,   // PRM_ABS_POS
     L6470::ATTR_R_WS,   // PRM_EL_POS
     L6470::ATTR_R_WR,   // PRM_MARK
     L6470::ATTR_R,      // PRM_SPEED
     L6470::ATTR_R_WS,   // PRM_ACC
     L6470::ATTR_R_WS,   // PRM_DEC
     L6470::ATTR_R_WR,   // PRM_MAX_SPEED
     L6470::ATTR_R_WS,   // PRM_MIN_SPEED
     L6470::ATTR_R_WR,   // PRM_KVAL_HOLD
     L6470::ATTR_R_WR,   // PRM_KVAL_RUN
     L6470::ATTR_R_WR,   // PRM_KVAL_ACC
     L6470::ATTR_R_WR,   // PRM_KVAL_DEC
     L6470::ATTR_R_WH,   // PRM_INT_SPEED
     L6470::ATTR_R_WH,   // PRM_ST_SLP
     L6470::ATTR_R_WH,   // PRM_FN_SLP_ACC
     L6470::ATTR_R_WH,   // PRM_FN_SLP_DEC
     L6470::ATTR_R_WR,   // PRM_K_THERM
     L6470::ATTR_R,      // PRM_ADC_OUT
     L6470::ATTR_R_WR,   // PRM_OCD_TH
     L6470::ATTR_R_WR,   // PRM_STALL_TH
     L6470::ATTR_R_WR,   // PRM_FS_SPD
     L6470::ATTR_R_WH,   // PRM_STEP_MODE
     L6470::ATTR_R_WS,   // PRM_ALARM_EN
     L6470::ATTR_R_WH,   // PRM_CONFIG
     L6470::ATTR_R,      // PRM_STATUS
     L6470::ATTR_R,
     L6470::ATTR_R,
     L6470::ATTR_R,
     L6470::ATTR_R,
     L6470::ATTR_R,
     L6470::ATTR_R,
     L6470::ATTR_R,
};

#define   OPPOSITE_DIR(dir)   (1-(dir))

//------------------------------------------------------------------------------
//   コンストラクタ
//   ss:   SSピン番号
//   busy: /BUSY ピン番号
//   proc: リミット信号の状態を問い合わせる関数
//------------------------------------------------------------------------------
L6470::L6470(uint8_t ss, uint8_t busy, std::function<uint8_t(int)> proc) :
     m_SS(ss), m_BUSY(busy),
     m_limitProc(proc), m_homeCompleted(false),
     m_status(0), m_alarmFlag(0), m_switchEvent(false), m_homingState(0),
     m_homingDir(DIR_REVERSE), m_homingSpeed(10000), m_savedMaxSpeed(16)
{
     pinMode(m_SS, OUTPUT);
     pinMode(m_BUSY, INPUT);
     initialize();
}

//------------------------------------------------------------------------------
//   デストラクタ
//------------------------------------------------------------------------------
L6470::~L6470()
{
     if( !isHalted() )
     {
          hardStop();
     }
}

//------------------------------------------------------------------------------
//   １バイトを送信
//
//   send  : BUSY状態の場合は、BUSYが解除されるのを待ってから送信
//   sendF : BUSY状態に関わらず即座に送信
//
//   L6470 は１バイト毎に SS を LOW/HIGH しないといけないことに注意．
//------------------------------------------------------------------------------
void L6470::send(uint8_t data)
{
     while( isBusy() );

     delayMicroseconds(5);
     digitalWrite(m_SS, LOW);
     delayMicroseconds(5);
     wiringPiSPIDataRW(SPI_CHANNEL, &data, 1);
     delayMicroseconds(5);
     digitalWrite(m_SS, HIGH);
     delayMicroseconds(5);
}

//------------------------------------------------------------------------------
void L6470::sendF(uint8_t data)
{
     delayMicroseconds(5);
     digitalWrite(m_SS, LOW);
     delayMicroseconds(5);
     wiringPiSPIDataRW(SPI_CHANNEL, &data, 1);
     digitalWrite(m_SS, HIGH);
     delayMicroseconds(5);
}

//------------------------------------------------------------------------------
//   指定アドレスへのパラメータ付き送信
//------------------------------------------------------------------------------
void L6470::transfer(uint8_t addr, uint8_t bytes, uint32_t val)
{
     send(addr);
     if( bytes >= 3 ){ send((uint8_t)((val >> 16) & 0xFF)); }
     if( bytes >= 2 ){ send((uint8_t)((val >>  8) & 0xFF)); }
     if( bytes >= 1 ){ send((uint8_t)(val         & 0xFF)); }
}

//------------------------------------------------------------------------------
//   パラメータ値を取得
//------------------------------------------------------------------------------
uint32_t L6470::getParam(uint8_t id)
{
     uint32_t val = 0;
     uint8_t data;

     if( (id >= 32) || (PARAM_ADDR[id] == 0) ){ return 0; }

     sendF(0x20 | PARAM_ADDR[id]);

     for( uint8_t i = 0 ; i < PARAM_SIZE[id] ; i++ )
     {
          val = val << 8;
          delayMicroseconds(5);
          digitalWrite(m_SS, LOW);
          data = 0x00;
          delayMicroseconds(5);
          wiringPiSPIDataRW(SPI_CHANNEL, &data, 1);
          delayMicroseconds(5);
          val = val | data;
          digitalWrite(m_SS, HIGH);
          delayMicroseconds(5);
     }

     return val;
}

//------------------------------------------------------------------------------
//   パラメータ値を設定
//------------------------------------------------------------------------------
void L6470::setParam(uint8_t id, uint32_t val)
{
     if( (id >= 32) || (PARAM_ADDR[id] == 0) )
     { 
          return; 
     }
     transfer(PARAM_ADDR[id], PARAM_SIZE[id], val);
}

//------------------------------------------------------------------------------
//   初期化
//------------------------------------------------------------------------------
void L6470::initialize()
{
     digitalWrite(m_SS, HIGH);

     // デバイスリセットを実行
     sendF(CMD_NOP);
     sendF(CMD_NOP);
     sendF(CMD_NOP);
     sendF(CMD_NOP);
     sendF(CMD_RESET_DEVICE);

     // パラメータを初期化
     hardHIZ();
     setParam(L6470::PRM_ACC, 0x12);         // [R, WS] 加速度 default 0x08A (12bit) (14.55*val+14.55[step/s^2]) 0x14
     setParam(L6470::PRM_DEC, 0x12);         // [R, WS] 減速度 default 0x08A (12bit) (14.55*val+14.55[step/s^2]) 0x14
     setParam(L6470::PRM_MAX_SPEED, m_savedMaxSpeed);  // [R, WR] 最大速度 default 0x041 (10bit) (15.25*val+15.25[step/s]) 0x0D
     setParam(L6470::PRM_MIN_SPEED, 0x00);   // [R, WS] 最小速度 default 0x000 (1+12bit) (0.238*val[step/s])
     setParam(L6470::PRM_FS_SPD, 0x3FF);     // [R, WR] マイクロステップからフルステップへの切替点速度 default 0x027 (10bit) (15.25*val+7.63[step/s])
     setParam(L6470::PRM_KVAL_HOLD, 0x18);   // [R, WR] 停止時励磁電圧 default 0x29 (8bit) (Vs[V]*val/256)
     setParam(L6470::PRM_KVAL_RUN, 0x24);    // [R, WR] 定速回転時励磁電圧 default 0x29 (8bit) (Vs[V]*val/256)
     setParam(L6470::PRM_KVAL_ACC, 0x29);    // [R, WR] 加速時励磁電圧 default 0x29 (8bit) (Vs[V]*val/256)
     setParam(L6470::PRM_KVAL_DEC, 0x29);    // [R, WR] 減速時励磁電圧 default 0x29 (8bit) (Vs[V]*val/256)
     setParam(L6470::PRM_STEP_MODE, 0x07);   // ステップモードdefault 0x07 (1+3+1+3bit)
     setParam(L6470::PRM_CONFIG, 0x2E98);    // ICコンフィグレーション (Bit4のSW_MODEを1に、その他はデフォルト)
                                             // 重要：
                                             //   PRM_CONFIG の SW_MODE は必ず1にすること。
                                             //   デフォルトの0だと原点を遮光しただけで HardStop が実行される。
                                             //   (移動中であれば即時停止。励磁を切っている状態で信号が入力すると励磁してしまう)
     // setParam(L6470::PRM_STALL_TH, 0x7F);
     // setParam(L6470::PRM_K_THERM, 0x0F);
     // 電源投入直後は WRONG_CMD などのアラームビットが立っている可能性が
     // あるので、ここでクリアしておく
     internalGetStatus();

     execControl();
     clearAlarm();
     hardHIZ();
}

//------------------------------------------------------------------------------
//   制御処理
//   ステータスの監視や、原点復帰シーケンス処理のため、本メソッドを
//   メインループ内で定期的に実行すること。
//------------------------------------------------------------------------------
void L6470::execControl()
{
     // ステータス値を更新
     m_status = getParam(PRM_STATUS);
     internalGetStatus();

     m_inMotion = ((m_status & 0x0060) != 0)? true : false;
     m_halted = ((m_status & 0x0001) != 0)? true : false;

     // 現在位置情報を更新
     internalUpdatePosition();

     // 現在速度情報を更新
     internalUpdateSpeed();

     // アラームの発生をチェック(UVLO,TH_WRN,TH_SD,OCD,STEP_LOSS_A,STEP_LOSS_B)
     // m_statusからアラーム関連ビットを読み取ってm_alarmFlagにセットする
     // m_alarmFlag |= getAlarm_UVLO(m_status);
     // m_alarmFlag |= getAlarm_UVLO_ADC(m_status);
     // m_alarmFlag |= getAlarm_TH_WARN(m_status);
     m_alarmFlag |= getAlarm_TH_SD(m_status);
     m_alarmFlag |= getAlarm_OCD(m_status);
     //m_alarmFlag |= getAlarm_STEP_LOSS_A(m_status);
     //m_alarmFlag |= getAlarm_STEP_LOSS_B(m_status);

     if( m_homeCompleted == true && m_alarmFlag != 0 )
     {
          m_homeCompleted = false;
     }

     // 外部スイッチのイベント発火をチェック
     if( m_status & 0x0008 )
     {
          m_switchEvent = true;
     }

     // エンドリミットのチェック
     if( isInMotion() )
     {
          int dir = getCurrentDirection();
          if( !m_limitProc(dir) )
          {
               // Serial.print("End-Limit overtravelled (");
               // Serial.print(dir);
               // Serial.println(")");
               sendF(CMD_SOFT_STOP);
          }
     }

     // 原点復帰シーケンスを実行
     if( m_homingState > 0 )
     {
          execHoming();
     }
}

//------------------------------------------------------------------------------
//   原点復帰シーケンス処理
//
//   原点復帰の手順：
//
//   ■ m_homingDir = DIR_REVERSE の場合
//
//   OT(-)       ORG
//    +-----------+--------
//
//    |<---------------* (1)
//    *-----------|-->   (2)
//             <--|--*   (3)
//             *->|      (4)
//
//   (1) マイナスリミットへ向かって回転。リミットを検出したら停止。
//       この時点で、外部スイッチ入力はON(Low)になっているはず。
//   (2) 反転し、原点サーチを開始(L6470/L6480のReleaseSWコマンドを使う)
//       外部スイッチ入力がOFF(High)になったらABS_POSをリセット後、停止。
//   (3) 原点を超えた反対側の位置(ABS_POS = -1000 pulse)へ絶対移動。
//   (4) 原点(ABS_POS = 0) へ移動。
//
//   ■ m_homingDir = DIR_FORWARD の場合
//
//               ORG         OT(+)
//      ----------+-----------+
//
//        *------------------>|   (1)
//             <--|-----------*   (2)
//             *--|-->            (3)
//                |<-*            (4)
//
//   (1) プラスリミットへ向かって回転。リミットを検出したら停止。
//   (2) 反転し、原点サーチを開始(L6470/L6480のGoUntilコマンドを使う)
//       外部スイッチ入力の立下り検出でABS_POSリセット後、停止。
//   (3) 原点を超えた反対側の位置(ABS_POS = +1000 pulse)へ絶対移動。
//   (4) 原点(ABS_POS = 0) へ移動。
//
//   ※各動作間には１秒のウェイトが入る。
//   ※バックラッシュ対策として、直接原点をサーチせず、一旦リミット端へ移動後に
//     反転してサーチしている点に注意。
//
//------------------------------------------------------------------------------
void L6470::execHoming()
{
     if( isAlarmHappened() || isHalted() )
     {
          if( m_homingState > 7 )
          {
               setParam(PRM_MAX_SPEED, m_savedMaxSpeed);    // MAX_SPEEDの設定を元に戻す
          }
          m_homingState = -1;
          // Serial.println("homing aborted");
          return;
     }

     if( m_homingState <= 0 || isInMotion() )
     {
          return;
     }

     switch( m_homingState )
     {
          case 1:
               // (1) DIR_FORWARD の場合は，プラスエンドリミットへ向かって移動
               if( m_homingDir == DIR_FORWARD )
               {
                    run(m_homingDir, m_homingSpeed);
               }
               else
               {
                    if( m_status & 0x04 )    // 既に原点を叩いている場合
                    {
                         releaseSW(ACT_RESET, DIR_FORWARD); // 一旦原点を抜ける
                         m_homingState = 101;
                    }
                    else
                    {
                         goUntil(ACT_RESET, m_homingDir, m_homingSpeed);
                    }
               }
               break;
          case 2:   // (1)の動作完了
          case 102:
          case 5:   // (2)の動作完了
          case 105:
          case 8:   // (3)の動作完了
               if( (m_homingState == 5) && (getLimitFlag(OPPOSITE_DIR(m_homingDir)) == 0) )
               {
                    // 原点を検出できずに反対側のリミットまで行って停止した。
                    // エラー終了とする。
                    m_homingState = -1;
                    // Serial.println("homing aborted (LIMIT)");
                    return;
               }
               // 時間待ちを開始
               m_homingTimer = millis() + 1000;   // 1秒間のウエイト開始
               break;

          case 3:   // (1)-(2)間の時間待ち
          case 103:
          case 6:   // (2)-(3)間の時間待ち
          case 106:
          case 9:   // (3)-(4)間の時間待ち
               // 時間待ち中
               if( millis() < m_homingTimer )
               {
                    return;
               }
               break;

          case 104:
               // 原点をサーチ
               goUntil(ACT_RESET, DIR_REVERSE, m_homingSpeed);
               break;
          case 4:
               // (2) 反転して、原点サーチ動作を開始
               // m_homingDir == DIR_REVERSE の場合、マイナスリミットで停止した状態では
               // すでに原点もONになっているので、releaseSW() で抜ける必要がある
               if( m_homingDir == DIR_REVERSE )
               {
                    releaseSW(ACT_RESET, OPPOSITE_DIR(m_homingDir));
               }
               else
               {
                    goUntil(ACT_RESET, OPPOSITE_DIR(m_homingDir), m_homingSpeed);
               }
               break;
          case 107:
               m_homingState = 7;
               // breakしない
          case 7:
               // (3) 原点をはさんで反対側へ1000パルスだけ「戻る」
               // 動作(3)(4)は MAX_SPEED レジスタの速度で動作するが、そのままの値では速すぎるので
               // 設定を変更する
               // ※ m_savedMaxSpeed と m_homingSpeed は単位が異なるので混同しないよう注意
               setParam(PRM_MAX_SPEED, m_savedMaxSpeed/10);
               relativeMove(m_homingDir, std::abs(getAbsPos())+1000);
               break;

          case 10:
               // (4) 原点へ移動
               goHome();
               break;

          case 11:
               // (4) が完了した
               m_homeCompleted = true;
               m_homingState = 0;
               setParam(PRM_MAX_SPEED, m_savedMaxSpeed);    // MAX_SPEEDの設定を元に戻す
               return;

          case HOMING_ABORT:
               // softStop, hardStop による中断 (m_homeCompleted は false のまま)
               m_homingState = -1;
               setParam(PRM_MAX_SPEED, m_savedMaxSpeed);    // MAX_SPEEDの設定を元に戻す
               return;
     }

     m_homingState++;
}

//------------------------------------------------------------------------------
//   ステータス取得 (publicメソッド)
//
//   戻り値 STATUS レジスタの値(16bit)
//
//   m_status の値をそのまま返す
//------------------------------------------------------------------------------
uint16_t L6470::getStatus()
{
     return m_status;
}

//------------------------------------------------------------------------------
//   ステータス取得 (privateメソッド)
//
//   戻り値 STATUS レジスタの値(16bit)
//
//   GetStatus コマンドを使って取得する
//   この結果は、L6470内部でラッチされているフラグをクリアするので、
//   外部へは公開せず、内部処理でのみ利用可能とする。
//------------------------------------------------------------------------------
uint16_t L6470::internalGetStatus()
{
     uint16_t val = 0;
     uint8_t data;

     sendF(CMD_GET_STATUS);
     for( uint8_t i = 0 ; i < 2 ; i++ )
     {
          val = val << 8;
          data = 0x00;
          delayMicroseconds(5);
          digitalWrite(m_SS, LOW);
          delayMicroseconds(5);
          wiringPiSPIDataRW(SPI_CHANNEL, &data, 1);
          val = val | data;
          delayMicroseconds(5);
          digitalWrite(m_SS, HIGH);
          delayMicroseconds(5);
     }
     return val;
}

//------------------------------------------------------------------------------
//   直前に実行要求したコマンドにエラーが発生していないか取得する
//------------------------------------------------------------------------------
bool L6470::isCommandError()
{
     // m_status や CMD_GET_STATUS ではなく、直接STATUSを読む必要がある
     uint16_t stat = getParam(PRM_STATUS);
     if( (stat & 0x0180) != 0 )
     {
          return true;
     }
     return false;
}

//------------------------------------------------------------------------------
//   現在のモータの回転方向を返す
//
//   戻り値: DIR_FORWARD or DIR_REVERSE
//------------------------------------------------------------------------------
uint8_t L6470::getCurrentDirection()
{
     return (m_status >> 4) & 0x01;
}

//------------------------------------------------------------------------------
//   "BUSY" か? (ICが特定の処理を受け付けない状態)
//
//   ※「モータが回転中かどうか」とは異なる。
//      run や goUntil での駆動中などは BUSY にはならないので注意。
//------------------------------------------------------------------------------
bool L6470::isBusy()
{
     return digitalRead(m_BUSY) == 0;
}

//------------------------------------------------------------------------------
//   モータが回転中かどうか？
//------------------------------------------------------------------------------
bool L6470::isInMotion()
{
     return m_inMotion;  // (getParam(PRM_STATUS) & 0x0060) != 0;
}

//------------------------------------------------------------------------------
//   モータの励磁が切れているかどうか？
//------------------------------------------------------------------------------
bool L6470::isHalted()
{
     return m_halted;    //(getParam(PRM_STATUS) & 0x0001) != 0;
}

//------------------------------------------------------------------------------
//   外部スイッチのONイベントが発火しているか？
//------------------------------------------------------------------------------
bool L6470::isExtSwitchTriggered()
{
     if( m_switchEvent )
     {
          m_switchEvent = false;
          return true;
     }
     return false;
}

//------------------------------------------------------------------------------
//   アラームが発生しているか？
//------------------------------------------------------------------------------
bool L6470::isAlarmHappened()
{
     return (m_alarmFlag & 0x3F) != 0;
}

//------------------------------------------------------------------------------
//   アラームフラグの値を取得する
//
//   戻り値(アラームが発生していなければ全てゼロ)
//   bit0: UVLO        (電圧低下ロックアウト)          (L6470/L6480)
//   bit1: UVLO_ADC    (ADC入力電圧低下ロックアウト)   (L6480のみ)
//   bit2: TH_WARN     (サーマル警報)                  (L6470/L6480)
//   bit3: TH_SD       (過昇温シャットダウン)          (L6470/L6480)
//   bit4: OCD         (過電流検出)
//   bit5: STEP_LOSS_A (A相ストール検出)
//   bit6: STEP_LOSS_B (B相ストール検出)
//------------------------------------------------------------------------------
uint8_t L6470::getAlarmFlag()
{
     return m_alarmFlag;
}

//------------------------------------------------------------------------------
//   アラームフラグをクリア
//------------------------------------------------------------------------------
void L6470::clearAlarm()
{
     //internalGetStatus();
     m_alarmFlag = 0;
}

//------------------------------------------------------------------------------
//   エンドリミット入力信号の状態を取得
//------------------------------------------------------------------------------
uint8_t L6470::getLimitFlag(uint8_t dir)
{
     return m_limitProc(dir);
}

//------------------------------------------------------------------------------
//   原点復帰処理の進行状態を取得する
//
//   戻り値: -1 実行中ではない(前回の原点復帰が正常に完了しなかった)
//            0 実行中ではない
//           >0 原点復帰シーケンスを実行中
//------------------------------------------------------------------------------
int L6470::getHomingState()
{
     return m_homingState;
}

//------------------------------------------------------------------------------
//   電源投入後、原点復帰が実行され、正常に完了しているか調べる
//------------------------------------------------------------------------------
bool L6470::isHomeCompleted()
{
     return m_homeCompleted;
}

//------------------------------------------------------------------------------
//   原点復帰を開始
//
//   dir: 最初に向かうエンドリミットの方向を指定
//        (DIR_FORWARD or DIR_REVERSE)
//   spd: 動作速度
//        (run コマンドや goUntil コマンドに与える値。SPEEDレジスタ値と同じ単位)
//------------------------------------------------------------------------------
void L6470::startHoming(uint8_t dir, uint32_t spd)
{
     if( m_homingState > 0 )
     {
          // 現在既に原点復帰動作中
          return;
     }

     // if( !m_LIMIT[DIR_REVERSE] || !m_LIMIT[DIR_FORWARD] )
     // {
     //      // エンドリミット信号を扱えない場合、原点復帰動作は実行不可
     //      // 単にABS_POSレジスタをゼロリセットする
     //      resetPos();
     //      m_homeCompleted = true;
     //      return;
     // }

     if( isHalted() )
     {
          softStop();    // 励磁が切れている場合は入れる
     }

     m_homeCompleted = false;                     // 原点復帰完了フラグをクリア
     m_homingDir = dir;                           // 方向をセット
     m_homingSpeed = spd;                         // 速度をセット
     // m_savedMaxSpeed = getParam(PRM_MAX_SPEED);   // 現在のMAX_SPEED値を退避
     m_homingState = 1;                           // 状態遷移フラグをセットして、動作開始
}

//------------------------------------------------------------------------------
//   指定速度、回転方向で連続駆動
//
//   dir : DIR_FORWARD(正転) or DIR_REVERSE(逆転)
//   spd : 速度(SPEEDレジスタ値の単位。pulse/sec単位との関係はgetSpeedを参照)
//
//   ※このコマンドで駆動を開始したらBUSYフラグはHighに戻る(駆動中もHighのまま)
//------------------------------------------------------------------------------
void L6470::run(uint8_t dir, uint32_t spd)
{
     transfer(CMD_RUN+(dir & 1), 3, spd);
}

//------------------------------------------------------------------------------
//   指定方向へ指定距離だけ相対移動
//
//   dir :      DIR_FORWARD(正転) or DIR_REVERSE(逆転)
//   distance : 移動量(pulse、絶対値)
//
//   ※速度は MAX_SPEEDパラメータによって決まる
//------------------------------------------------------------------------------
void L6470::relativeMove(uint8_t dir, uint32_t distance)
{
     transfer(CMD_MOVE+(dir & 1), 3, distance);
}

//------------------------------------------------------------------------------
//   指定位置へ移動（最小移動量となる回転方向で）（XYステージでは使わない機能）
//
//   pos : 移動先位置(pulse)
//
//   ※速度は MAX_SPEEDパラメータによって決まる
//------------------------------------------------------------------------------
void L6470::moveTo(int32_t pos)
{
     // Serial.println(getParam(PRM_CONFIG), HEX);
     transfer(CMD_GOTO, 3, (uint32_t)(pos & 0x3FFFFF));
}

//------------------------------------------------------------------------------
//   指定位置へ回転方向を指定して絶対移動
//
//   dir : DIR_FORWARD(正転) or DIR_REVERSE(逆転)
//   pos : 移動先位置(pulse)
//
//   ※速度は MAX_SPEEDパラメータによって決まる
//------------------------------------------------------------------------------
void L6470::moveTo(uint8_t dir, int32_t pos)
{
     // Serial.println(getParam(PRM_CONFIG), HEX);
     transfer(CMD_GOTO_DIR+(dir & 1), 3, (uint32_t)(pos & 0x3FFFFF));
}

//------------------------------------------------------------------------------
//   外部トリガ信号のON検出移動
//   指定した速度、回転方向で、外部トリガ信号(SW)がON(Low)になるまで動作
//   検出したら減速して停止
//
//   act : ACT_RESET = 信号検出時にABS_POSレジスタをゼロリセット
//       : ACT_MARK  = 信号検出時に位置レジスタの値をMARKレジスタへセット
//                     (ABS_POSレジスタはリセットされない)
//   dir : DIR_FORWARD(正転) or DIR_REVERSE(逆転)
//   spd : 速度(SPEEDレジスタ値の単位。pulse/sec単位との関係はgetSpeedを参照)
//
//   ※ SWの値は STATUSレジスタの SW_F ビットで確認できる
//   ※ SWの立下り検出の有無は STATUSレジスタの SW_EVN ビットで確認できる
//      (GetStatusコマンドの実行でビットはリセットされる)
//------------------------------------------------------------------------------
void L6470::goUntil(uint8_t act, uint8_t dir, uint32_t spd)
{
     transfer(CMD_GO_UNTIL|(act & 0x08)|(dir & 1), 3, spd);
}

//------------------------------------------------------------------------------
//   外部トリガ信号のOFF検出動作
//   指定した方向へ、SWがOFF(High)になるまで最小速度(5step/s)で移動
//   SWがOFFになったら停止
//
//   act : ACT_RESET = SWがOFFになったらABS_POSをゼロリセット
//       : ACT_MARK  = SWがOFFになったらABS_POSの値をMARKレジスタへコピー
//                     (ABS_POSはリセットされない)
//   dir : DIR_FORWARD(正転) or DIR_REVERSE(逆転)
//------------------------------------------------------------------------------
void L6470::releaseSW(uint8_t act, uint8_t dir)
{
     transfer(CMD_RELEASE_SW|(act & 0x08)|(dir & 1), 0, 0);
}

//------------------------------------------------------------------------------
//   原点(ABS_POS=0の位置)へ移動
//
//   ※速度は MAX_SPEEDパラメータによって決まる
//------------------------------------------------------------------------------
void L6470::goHome()
{
     transfer(CMD_GO_HOME, 0, 0);
}

//------------------------------------------------------------------------------
//   MARK位置へ移動
//
//   ※速度は MAX_SPEEDパラメータによって決まる
//------------------------------------------------------------------------------
void L6470::goMark()
{
     transfer(CMD_GO_MARK, 0, 0);
}

//------------------------------------------------------------------------------
//   ABS_POS位置レジスタをゼロリセット
//------------------------------------------------------------------------------
void L6470::resetPos()
{
     transfer(CMD_RESET_POS, 0, 0);
     m_homeCompleted = false;
}

//------------------------------------------------------------------------------
//   減速停止
//   減速度はDECレジスタの値が使われる
//   （既に停止中で励磁が切れている場合は励磁して保持）
//------------------------------------------------------------------------------
void L6470::softStop()
{
     sendF(CMD_SOFT_STOP);
     if( m_homingState > 0 )
     {
          m_homingState = HOMING_ABORT;
     }
}

//------------------------------------------------------------------------------
//   即時停止（既に停止中で励磁が切れている場合は励磁して保持）
//------------------------------------------------------------------------------
void L6470::hardStop()
{
     sendF(CMD_HARD_STOP);
     if( m_homingState > 0 )
     {
          m_homingState = HOMING_ABORT;
     }
}

//------------------------------------------------------------------------------
//   減速停止後、励磁を切る（既に停止中なら単に励磁を切る）
//------------------------------------------------------------------------------
void L6470::softHIZ()
{
     sendF(CMD_SOFT_HIZ);
     m_homeCompleted = false;
}

//------------------------------------------------------------------------------
//   即時停止し、励磁を切る（既に停止中なら単に励磁を切る）
//------------------------------------------------------------------------------
void L6470::hardHIZ()
{
     sendF(CMD_HARD_HIZ);
     m_homeCompleted = false;
}

//------------------------------------------------------------------------------
//   現在位置(ABS_POS)の値を取得する
//
//   戻り値 -2097152 ～ 2097151 (pulse)
//------------------------------------------------------------------------------
int32_t L6470::getAbsPos()
{
     return m_position;
}
void L6470::internalUpdatePosition()
{
     uint32_t pos = getParam(PRM_ABS_POS);

     if( pos & 0x200000 )
     { 
          // std::printf("0x%08X\n", pos);
          pos |= 0xFFC00000; 
     }
     else
     {
          pos &= 0x1FFFFF;
     }
     m_position = (int32_t)pos;
}

//------------------------------------------------------------------------------
//   現在の速度を取得
//
//   戻り値 pulse/sec (下記注釈参照)
//------------------------------------------------------------------------------
int32_t L6470::getSpeed()
{
     return m_speed;
}
void L6470::internalUpdateSpeed()
{
     // step/sec = SPEED * (2^(-28) / 250*(10^-9)) = SPEED/67.108864
     // (L6470のデータシート参照)

     // 上の変換式の結果は「pulse/sec」ではないことに注意。
     // これを pulse/sec に変換するには、StepModeに応じた補間係数を乗する。
     // (1/128マイクロステップ動作の場合は上記値の128倍となる)

     float multiplier = (float)pow(2, (getParam(PRM_STEP_MODE) & 0x07));
     m_speed = (int32_t)(multiplier*getParam(PRM_SPEED)/67.108864);
}

//------------------------------------------------------------------------------
//  パラメータをArduinoのEEPROMへ保存する
//
//  書き込むデータ (計 130バイト)
//  offset+0         : 32*4byte文のチェックサム
//  offset+1         : 上記チェックサムの補数
//  offset+2 ～ +129 : パラメータ値(各4バイト×32個、未使用域はゼロで埋める)
//------------------------------------------------------------------------------
// void L6470::writeParamToEEPROM(int offset)
// {
//     uint8_t sum = 0x00;
//     int address = offset + 2;
//     uint32_t param;
//     for( uint8_t n = 0 ; n < 32 ; n++ )
//     {
//         if( PARAM_ATTR[n] != ATTR_R )
//         {
//             param = (uint32_t)getParam(n);
//         }
//         else
//         {
//             param = 0;
//         }
//         for( uint8_t i = 0 ; i < 4 ; i++ )
//         {
//             uint8_t p = *(((uint8_t *)&param)+i);
//             EEPROM.write(address+i, p);
//             sum += p;
//         }
//         address += 4;
//     }
//     EEPROM.write(offset, sum);
//     EEPROM.write(offset+1, sum ^ 0xFF);
// }

//------------------------------------------------------------------------------
//  パラメータをArduinoのEEPROMから読み込んでドライバへ設定する
//------------------------------------------------------------------------------
// bool L6470::readParamFromEEPROM(int offset)
// {
//     uint8_t sum = EEPROM.read(offset);
//     if( EEPROM.read(offset+1) != (sum ^ 0xFF) )
//     {
//         return false;
//     }
//     uint8_t tmp = 0x00;
//     uint32_t param[32];
//     int address = offset + 2;
//     for( uint8_t n = 0 ; n < 32 ; n++ )
//     {
//         uint8_t *p = (uint8_t *)&param[n];
//         for( uint8_t i = 0 ; i < 4 ; i++ )
//         {
//             *p = EEPROM.read(address+i);
//             tmp += *p;
//             p++;
//         }
//         address += 4;
//     }
//     if( sum != tmp )
//     {
//         return false;
//     }
//     address = offset + 2;
//     for( uint8_t n = 0 ; n < 32 ; n++ )
//     {
//         if( PARAM_ATTR[n] != ATTR_R )
//         {
//             setParam(n, param[n]);
//         }
//         address += 4;
//     }
//     return true;
// }

