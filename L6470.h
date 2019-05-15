#ifndef   L64X0_DRIVER_H
#define   L64X0_DRIVER_H

#include <cstdint>
#include <functional>

//------------------------------------------------------------------------------
class L6470
{
     private:
          enum{
               CMD_NOP          = 0x00,
               CMD_MOVE         = 0x40,
               CMD_RUN          = 0x50,
               CMD_GOTO         = 0x60,
               CMD_GOTO_DIR     = 0x68,
               CMD_GO_HOME      = 0x70,
               CMD_GO_MARK      = 0x78,
               CMD_GO_UNTIL     = 0x82,
               CMD_RELEASE_SW   = 0x92,
               CMD_SOFT_HIZ     = 0xA0,
               CMD_HARD_HIZ     = 0xA8,
               CMD_SOFT_STOP    = 0xB0,
               CMD_HARD_STOP    = 0xB8,
               CMD_RESET_DEVICE = 0xC0,
               CMD_GET_STATUS   = 0xD0,
               CMD_RESET_POS    = 0xD8
          };
          typedef uint8_t (*LimitInputProc)(int);

          uint8_t   m_SS;          // SSピン番号
          uint8_t   m_BUSY;        // BUSYピン番号
          // uint8_t   m_LIMIT[2];    // エンドリミット信号入力ピン番号[-/+]

          // LimitInputProc m_limitProc;        // エンドリミット信号の入力状態を外部に問い合わせるための関数
          std::function<uint8_t(int)> m_limitProc;     // エンドリミット信号の入力状態を外部に問い合わせるための関数

          uint16_t  m_status;                // 直近で取得したSTATUSレジスタの値
          int32_t   m_position;              // 直近で取得したABS_POSレジスタの値
          int32_t   m_speed;                 // 直近で取得した速度値(pulse/sec単位) 
          uint8_t   m_alarmFlag;             // アラーム要因を示すフラグ(STATUSレジスタ値のBit14～Bit9を反転してLSBへシフトした値)
          bool      m_halted;                // 現在励磁しているか（していなければtrue)
          bool      m_inMotion;              // 現在動作中か(動作中ならtrue)
          bool      m_switchEvent;           // 外部スイッチイベントの有無を示すフラグ
          int       m_homingState;           // 原点復帰の状態遷移フラグ
          uint8_t   m_homingDir;             // 原点復帰の初動方向
          uint32_t  m_homingSpeed;           // 原点復帰速度(SPEEDレジスタ単位)
          uint32_t  m_homingTimer;           // 原点復帰の待ち時間管理用カウンタ
          uint32_t  m_savedMaxSpeed;         // 原点復帰での低速動作時に元のMAX_SPEEDレジスタ値を退避するためのバッファ
          bool      m_homeCompleted;         // 電源投入後に原点復帰動作が正常に完了したらtrue
          bool      m_enableLimitInput;      // エンドリミット信号入力を扱う場合はtrue

          enum{HOMING_ABORT = 99};      // 原点復帰中に softStop, hardStop で停止させた場合、m_homingState がこの値になる
                                        // (softHIZ, hardHIZ で停止させた場合は execHoming() 内で -1 になる)

          void send(uint8_t);
          void sendF(uint8_t d);
          void transfer(uint8_t addr, uint8_t bytes, uint32_t val);

          void initialize();

          void     execHoming();
          uint16_t internalGetStatus();
          void internalUpdatePosition();
          void internalUpdateSpeed();

          uint8_t getAlarm_UVLO(uint16_t s)
          {
               return (s & 0x0200)? ALM_NONE : ALM_UVLO;
          }
          uint8_t getAlarm_UVLO_ADC(uint16_t s)
          {
               return ALM_NONE;
               // return (s & 0x0400)? ALM_NONE : ALM_UVLO_ADC;
          }
          uint8_t getAlarm_TH_WARN(uint16_t s)
          {
               return (s & 0x0400)? ALM_NONE : ALM_TH_WARN;
          }
          uint8_t getAlarm_TH_SD(uint16_t s)
          {
               return (s & 0x0800)? ALM_NONE : ALM_TH_SD;
          }
          uint8_t getAlarm_OCD(uint16_t s)
          {
               return (s & 0x1000)? ALM_NONE : ALM_OCD; 
          }
          uint8_t getAlarm_STEP_LOSS_A(uint16_t s)
          {
               return (s & 0x2000)? ALM_NONE : ALM_STEP_LOSS_A;
          }
          uint8_t getAlarm_STEP_LOSS_B(uint16_t s)
          {
               return (s & 0x4000)? ALM_NONE : ALM_STEP_LOSS_B;
          }

          static void controlTask(void *param);

     public:
          enum{ SPI_CHANNEL = 0 };
          enum{
               ALM_NONE        = 0x00,
               ALM_UVLO        = 0x01,  // 電圧低下ロックアウト       (L6470/L6480)
               // ALM_UVLO_ADC    = 0x02,  // ADC入力電圧低下ロックアウト(L6480のみ)
               ALM_TH_WARN     = 0x04,  // サーマル警報               (L6470/L6480)
               ALM_TH_SD       = 0x08,  // サーマルシャットダウン     (L6470/L6480)
               ALM_OCD         = 0x10,  // 過電流検出                 (L6470/L6480)
               ALM_STEP_LOSS_A = 0x20,  // A相ストール検出            (L6470/L6480)
               ALM_STEP_LOSS_B = 0x40,  // B相ストール検出            (L6470/L6480)
          };

          // パラメータの識別子
          // この識別子の値は、パラメータの「アドレス」とは異なるので注意
          enum{
               PRM_ABS_POS    =  0,
               PRM_EL_POS     =  1,
               PRM_MARK       =  2,
               PRM_SPEED      =  3,
               PRM_ACC        =  4,
               PRM_DEC        =  5,
               PRM_MAX_SPEED  =  6,
               PRM_MIN_SPEED  =  7,
               PRM_KVAL_HOLD  =  8,
               PRM_KVAL_RUN   =  9,
               PRM_KVAL_ACC   = 10,
               PRM_KVAL_DEC   = 11,
               PRM_INT_SPEED  = 12,
               PRM_ST_SLP     = 13,
               PRM_FN_SLP_ACC = 14,
               PRM_FN_SLP_DEC = 15,
               PRM_K_THERM    = 16,
               PRM_ADC_OUT    = 17,
               PRM_OCD_TH     = 18,
               PRM_STALL_TH   = 19,
               PRM_FS_SPD     = 20,
               PRM_STEP_MODE  = 21,
               PRM_ALARM_EN   = 22,
               PRM_CONFIG     = 23,
               PRM_STATUS     = 24,
               // PRM_GATECFG1   = 25,
               // PRM_GATECFG2   = 26,
          };

          enum{DIR_REVERSE = 0, DIR_FORWARD = 1};
          enum{ACT_RESET = 0, ACT_MARK = 0x08};

          enum{     // パラメータのアクセス属性
               ATTR_R = 0,    // 読み取り専用
               ATTR_R_WS,     // モータ停止中に限り書き込み可
               ATTR_R_WH,     // 非励磁時に限り書き込み可
               ATTR_R_WR,     // 常時読み書き可
          };

          static const uint8_t PARAM_ADDR[32];    // 各パラメータのアドレス
          static const uint8_t PARAM_SIZE[32];    // 各パラメータのサイズ(バイト数)
          static const uint8_t PARAM_ATTR[32];    // 各パラメータのアクセス属性

          L6470(uint8_t ss, uint8_t busy, std::function<uint8_t(int)> proc);
          ~L6470();

          uint16_t getStatus();
          uint8_t  getCurrentDirection();
          bool     isBusy();
          bool     isInMotion();
          bool     isHalted();
          bool     isCommandError();
          void     execControl();
          bool     isExtSwitchTriggered();
          bool     isAlarmHappened();
          uint8_t  getAlarmFlag();
          uint8_t  getLimitFlag(uint8_t dir);
          int      getHomingState();
          bool     isHomeCompleted();
          void     clearAlarm();
          void     startHoming(uint8_t dir, uint32_t spd);
          void     run(uint8_t dir, uint32_t spd);
          void     relativeMove(uint8_t dir, uint32_t distance);
          void     moveTo(int32_t pos);
          void     moveTo(uint8_t dir, int32_t pos);
          void     goUntil(uint8_t act, uint8_t dir, uint32_t spd);
          void     releaseSW(uint8_t act, uint8_t dir);
          void     goHome();
          void     goMark();
          void     resetPos();
          void     softStop();
          void     hardStop();
          void     softHIZ();
          void     hardHIZ();
          int32_t  getAbsPos();
          int32_t  getSpeed();
          uint32_t getParam(uint8_t id);
          void     setParam(uint8_t id, uint32_t val);
          bool     readParamFromEEPROM(int offset);
          void     writeParamToEEPROM(int offset);
};

#endif
