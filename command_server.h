//------------------------------------------------------------------------------
//   command_server.h
//------------------------------------------------------------------------------
#ifndef   COMMAND_SERVER_H
#define   COMMAND_SERVER_H

#include <cstdint>
#include <vector>
#include <deque>
#include <map>
#include <thread>
#include <mutex>
#include "robot.h"

//------------------------------------------------------------------------------
class Packet
{
     public:
          enum{ STX = 0x02, ETX = 0x03 };
          enum{ MAX_PACKET_SIZE = 256 };
          enum{ MAX_DATA_LENGTH = 250 };     // STX, ETX, ID, SNO, LEN, SUM の６バイトを差し引いた残り
          enum{
               OFFSET_STX = 0,
               OFFSET_ID  = 1,
               OFFSET_SERIAL = 2,
               OFFSET_LEN = 3,
          };

     private:
          enum{ DEFAULT_ID = 0x00 };
          enum{ DEFAULT_SERIAL = 0x00 };
          enum{ DEFAULT_DATALEN = 0x00 };

          uint8_t m_body[MAX_PACKET_SIZE];
          int     m_rawBytePtr;

          uint8_t getChecksum();

     public:
          Packet();
          Packet *clone();
          void clear();
          void create(uint8_t id, uint8_t serialNo);
          void addPacketData(void *data, int size);
          bool push(uint8_t data);
          int  getRawBytes(std::vector<uint8_t>& buffer);

          uint8_t  getID(){ return m_body[1]; };
          uint8_t  getSerialNo(){ return m_body[2]; };
          uint8_t  getDataLength(){ return m_body[3]; };
          bool     readUInt8Data(int offset, uint8_t *data);
          bool     readInt8Data(int offset, int8_t *data);
          bool     readUInt16Data(int offset, uint16_t *data);
          bool     readInt16Data(int offset, int16_t *data);
          bool     readUInt32Data(int offset, uint32_t *data);
          bool     readInt32Data(int offset, int32_t *data);
};


//------------------------------------------------------------------------------
// class Queue
// {
//      private:
//           enum{ CAPACITY = 256 };
//           uint8_t  m_buffer[CAPACITY];
//           int      m_rdPtr;
//           int      m_wrPtr;
//           int      m_size;

//      public:
//           Queue();
//           void    clear();
//           bool    push(uint8_t);
//           bool    push(const uint8_t *, int);
//           uint8_t pop();
//           uint8_t peek();
//           bool    isEmpty() const { return (m_size == 0); }
// };


//------------------------------------------------------------------------------
class TcpServer
{
     private:
          enum{PORT = 12468};
          enum{INIT_FAILED = -1, LISTENING, PROC_SESSION, POLL_ERROR};
          std::deque<uint8_t> m_rxBuffer;
          std::vector<uint8_t> m_txBuffer;
          Packet m_requestPacket;
          bool m_requestReceived;
          std::thread *m_servThread;
          std::mutex m_mutex;
          int  m_socket;
          int  m_client;
          int  m_state;
          bool m_terminated;

          void execute();
          void doAccept();
          void doReceive();
          void doSend();

     public:
          TcpServer();
          ~TcpServer();

          void enableRequest();
          void sendResponse(Packet& response);

          bool available(){ return m_requestReceived; }
          Packet *getRequest();    //{ return &m_requestPacket; }
};


//------------------------------------------------------------------------------
class CommandObject
{
     public:
          enum
          {
               MOTOR_BASE     = 0,
               MOTOR_SHOULDER = 1,
               MOTOR_ELBOW    = 2,

               NUM_MOTORS     = 3,
          };
          enum
          {
               STS_OK      = 0,    // コマンドは正常に実行した
               STS_UNABLE  = 1,    // このコマンドは現在実行できない
               STS_INVALID = 2,    // コマンドパラメータの指定が無効
               STS_FAIL    = 4     // コマンドの実行に失敗した
          };

     protected:
          int       m_id;
          Robot    *m_robot;

          virtual uint8_t execute(Packet *request){ return STS_OK; }
          virtual void setResponseData(Packet *response){}

     public:
          CommandObject(int id, Robot *robot);
          void processRequest(Packet *request, Packet *response);
};

//------------------------------------------------------------------------------
class EnableCommand : public CommandObject
{
     public:
          enum{ID = 0};
     protected:
          uint8_t execute(Packet *request);
     public:
          EnableCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class ResetCommand : public CommandObject
{
     public:
          enum{ID = 1};
     protected:
          uint8_t execute(Packet *request);
     public:
          ResetCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class StopCommand : public CommandObject
{
     public:
          enum{ID = 2};
     protected:
          uint8_t execute(Packet *request);
     public:
          StopCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class HomingCommand : public CommandObject
{
     public:
          enum{ID = 3};
     protected:
          uint8_t execute(Packet *request);
     public:
          HomingCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class MovetoCommand : public CommandObject
{
     public:
          enum{ID = 4};
     protected:
          uint8_t execute(Packet *request);
     public:
          MovetoCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class Move3DCommand : public CommandObject
{
     public:
          enum{ID = 5};
     protected:
          uint8_t execute(Packet *request);
     public:
          Move3DCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class ReadParamCommand : public CommandObject
{
     public:
          enum{ID = 6};
     private:
          uint16_t m_motorID;
     protected:
          uint8_t execute(Packet *request);
          void setResponseData(Packet *response);
     public:
          ReadParamCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class WriteParamCommand : public CommandObject
{
     public:
          enum{ID = 7};
     protected:
          uint8_t execute(Packet *request);
     public:
          WriteParamCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class SaveParamCommand : public CommandObject
{
     public:
          enum{ID = 8};
     protected:
          uint8_t execute(Packet *request);
     public:
          SaveParamCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class StatusCommand : public CommandObject
{
     public:
          enum{ID = 9};
     protected:
          void setResponseData(Packet *response);
     public:
          StatusCommand(Robot *robot);
};

//------------------------------------------------------------------------------
class GripperCommand : public CommandObject
{
     public:
          enum{ID = 10};
     protected:
          uint8_t execute(Packet *request);
     public:
          GripperCommand(Robot *robot);
};

//------------------------------------------------------------------------------
// class WriteTeachCommand : public CommanddObject
// {
//      public:
//           enum{ID = 11};
//      protected:
//           uint8_t execute(Packet *request);
//      public:
//           WriteTeachCommand(Robot *robot);
// };
//
// //------------------------------------------------------------------------------
// class SaveTeachCommand : public CommanddObject
// {
//      public:
//           enum{ID = 12};
//      protected:
//           uint8_t execute(Packet *request);
//      public:
//           SaveTeachCommand(Robot *robot);
// };

//------------------------------------------------------------------------------
class CommandManager
{
     private:
          std::map<int, CommandObject *> m_command;
          TcpServer    m_server;
          Robot       *m_robot;
          std::thread *m_thread;
          bool         m_terminated;

          void execute();

     public:
          CommandManager(Robot *robot);
          ~CommandManager();
};

#endif
