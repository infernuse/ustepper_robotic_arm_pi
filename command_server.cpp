//------------------------------------------------------------------------------
//   command_server.cpp
//------------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "command_server.h"


//==============================================================================
//   Packet
//==============================================================================
//   コンストラクタ
//------------------------------------------------------------------------------
Packet::Packet()
{
     clear();
}

//------------------------------------------------------------------------------
Packet *Packet::clone()
{
     Packet *p = new Packet();
     memcpy(p->m_body, m_body, MAX_PACKET_SIZE);
     return p;
}

//------------------------------------------------------------------------------
//   現在の内部データをすべてクリアしてデフォルトの状態にする
//   シーケンシャル書き込みを行う場合は必ず最初にこのメソッドを実行する
//------------------------------------------------------------------------------
void Packet::clear()
{
     m_body[0] = STX;
     m_body[1] = DEFAULT_ID;
     m_body[2] = DEFAULT_SERIAL;
     m_body[3] = DEFAULT_DATALEN;
     m_body[4] = getChecksum();
     m_body[5] = ETX;

     m_rawBytePtr = 0;
}

//------------------------------------------------------------------------------
//   チェックサムを算出して返す
//------------------------------------------------------------------------------
uint8_t Packet::getChecksum()
{
     int length = getDataLength();
     uint8_t sum = 0x00;
     for( int n = 0 ; n < length ; n++ )
     {
          sum += m_body[4+n];
     }
     return sum;
}

//------------------------------------------------------------------------------
//   新しいパケットを準備する
//------------------------------------------------------------------------------
void Packet::create(uint8_t id, uint8_t serialNo)
{
     clear();
     m_body[1] = id;
     m_body[2] = serialNo;
}

//------------------------------------------------------------------------------
//   パケットへ「データ」を追加する
//   m_body[0] : STX(0x02)
//   m_body[1] : コマンドID
//   m_body[2] : シリアル番号
//   m_body[3] : (後続の)データ部の長さ(byte) = len
//   m_body[4] : データ[0]
//   m_body[5] : データ[1]
//   ...
//   m_body[4+len] : チェックサム
//   m_body[5+len] : ETX(0x03)
//------------------------------------------------------------------------------
void Packet::addPacketData(void *data, int size)
{
     int len = getDataLength();
     size = std::min(size, MAX_DATA_LENGTH - len);
     uint8_t *p = m_body + 4 + len;     // 次のデータ格納位置を指すポインタ
     memcpy(p, data, size);
     len += size;
     m_body[3] = (uint8_t)len;          // 「データ長」を更新
     m_body[4+len] = getChecksum();     // チェックサムを更新
     m_body[5+len] = ETX;
}

//------------------------------------------------------------------------------
//   パケットのバイト列へシーケンシャルにデータを書き込む
//------------------------------------------------------------------------------
bool Packet::push(uint8_t data)
{
     bool canPush = false;
     bool done = false;
     int ofs, len;

     // 書き込み位置別に，data が正しい（受け入れ可能な）データであるかチェックする
     switch( m_rawBytePtr )
     {
          case 0:
               canPush = (data == STX)? true : false;
               break;
          case 1:
          case 2:
               canPush = true;
               break;
          case 3:
               canPush = (0 <= data && data <= MAX_DATA_LENGTH)? true : false;
               break;
          default:
               ofs = m_rawBytePtr - 4;
               len = getDataLength();
               if( ofs < len )
               {
                    canPush = true;
               }
               else if( ofs == len )
               {
                    canPush = (data == getChecksum())? true : false;
               }
               else if( ofs == len+1 )
               {
                    canPush = (data == ETX)? true : false;
                    done = canPush;
               }
               break;
     }
     if( canPush )
     {
          // 書式通りの正規なデータであると判断したので，バイト列へ追加する
          m_body[m_rawBytePtr] = data;
          if( done )
          {
               // パケット全体を(ETXまで)取得できた
               m_rawBytePtr = 0;
          }
          else
          {
               m_rawBytePtr++;
          }
     }
     else
     {
          // パケットの書式を逸脱する不正なデータなので，読み込みをリセットする
          // Serial.print("[Packet] Unexpected data : ");
          // Serial.println(data);
          m_rawBytePtr = 0;
     }
     return done;
}

//------------------------------------------------------------------------------
//   パケット全体を表すバイト列を取得する
//------------------------------------------------------------------------------
int Packet::getRawBytes(std::vector<uint8_t>& buffer)
{
     int len = getDataLength() + 6;     // '6' は，「STX,ID,SNO,LEN,SUM,ETX」のデータ以外の６バイト
     buffer.clear();
     buffer.insert(buffer.end(), m_body, m_body+len);
     return len;
}

//------------------------------------------------------------------------------
//   パケットのデータを読み取るためのメソッド群
//------------------------------------------------------------------------------
bool Packet::readUInt8Data(int offset, uint8_t *data)
{
     if( (0 <= offset) && (offset < getDataLength()) )
     {
          *data = m_body[offset+4];
          return true;
     }
     return false;
}
//------------------------------------------------------------------------------
bool Packet::readInt8Data(int offset, int8_t *data)
{
     return readUInt8Data(offset, (uint8_t *)data);
}
//------------------------------------------------------------------------------
bool Packet::readUInt16Data(int offset, uint16_t *data)
{
     if( (0 <= offset) && (offset < (getDataLength()-1)) )
     {
          *data = *((uint16_t *)(m_body+offset+4));
          return true;
     }
     return false;
}
//------------------------------------------------------------------------------
bool Packet::readInt16Data(int offset, int16_t *data)
{
     return readUInt16Data(offset, (uint16_t *)data);
}
//------------------------------------------------------------------------------
bool Packet::readUInt32Data(int offset, uint32_t *data)
{
     if( (0 <= offset) && (offset < (getDataLength()-3)) )
     {
          *data = *((uint32_t *)(m_body+offset+4));
          return true;
     }
     return false;
}
//------------------------------------------------------------------------------
bool Packet::readInt32Data(int offset, int32_t *data)
{
     return readUInt32Data(offset, (uint32_t *)data);
}




//==============================================================================
//   Queue
//==============================================================================
//   コンストラクタ
//------------------------------------------------------------------------------
// Queue::Queue()
// {
//      clear();
// }

// //------------------------------------------------------------------------------
// //   キューをクリア
// //------------------------------------------------------------------------------
// void Queue::clear()
// {
//      m_size = 0;
//      m_rdPtr = 0;
//      m_wrPtr = 0;
// }

// //------------------------------------------------------------------------------
// //   エンキュー
// //------------------------------------------------------------------------------
// bool Queue::push(uint8_t data)
// {
//      if( m_size == CAPACITY )
//      {
//           return false;
//      }
//      m_buffer[m_wrPtr] = data;
//      m_wrPtr = (m_wrPtr + 1) % CAPACITY;
//      ++m_size;
//      return true;
// }
// //------------------------------------------------------------------------------
// bool Queue::push(const uint8_t *data, int length)
// {
//      for( int n = 0 ; n < length ; n++ )
//      {
//           if( !push(data[n]) )
//           {
//                return false;
//           }
//      }
//      return true;
// }

// //------------------------------------------------------------------------------
// //   デキュー
// //------------------------------------------------------------------------------
// uint8_t Queue::pop()
// {
//      if( isEmpty() )
//      {
//           return 0;
//      }
//      uint8_t data = m_buffer[m_rdPtr];
//      m_rdPtr = (m_rdPtr + 1) % CAPACITY;
//      --m_size;
//      return data;
// }

// //------------------------------------------------------------------------------
// uint8_t Queue::peek()
// {
//      return m_buffer[m_rdPtr];
// }




//==============================================================================
//   CommandServer
//   TCPソケットを使ったサーバ
//==============================================================================
//   コンストラクタ
//------------------------------------------------------------------------------
TcpServer::TcpServer() : m_requestReceived(false),
     m_client(-1), m_terminated(false)
{
     m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if( m_socket < 0 )
     {
          perror("[TcpServer] socket() failed");
          m_state = INIT_FAILED;
          return;
     }

     // ソケットオプションを設定
     int opf = 1;
     setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opf, sizeof(opf));

     // アドレスをバインド
     struct sockaddr_in servAddr;
     memset(&servAddr, 0, sizeof(servAddr));
     servAddr.sin_family = AF_INET;
     servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     servAddr.sin_port = htons(PORT);
     if( bind(m_socket, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0 )
     {
          perror("[TcpServer] bind() failed");
          m_state = INIT_FAILED;
          return;
     }

     if( listen(m_socket, SOMAXCONN) < 0 )
     {
          perror("[TcpServer] listen() failed");
          m_state = INIT_FAILED;
          return;
     }

     m_state = LISTENING;
     std::printf("[TcpServer] Waiting for a new connection.\n");

     m_servThread = new std::thread([this](){ execute(); });
}

//------------------------------------------------------------------------------
//   デストラクタ
//------------------------------------------------------------------------------
TcpServer::~TcpServer()
{
     m_terminated = true;
     m_servThread->join();
     delete m_servThread;
     close(m_client);
     close(m_socket);
}

//------------------------------------------------------------------------------
//   サーバ処理
//------------------------------------------------------------------------------
void TcpServer::execute()
{
     printf("[TcpServer] thread started.\n");
     while( !m_terminated )
     {
          m_mutex.lock();
          switch( m_state )
          {
               case LISTENING:
                    doAccept();
                    break;
               case PROC_SESSION:
                    doReceive();
                    doSend();
                    break;
          }
          m_mutex.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }
     printf("[TcpServer] thread terminated.\n");
}

//------------------------------------------------------------------------------
void TcpServer::doAccept()
{
     struct pollfd fd;
     fd.fd = m_socket;
     fd.events = POLLIN;
     int ret = poll(&fd, 1, 0);
     if( (ret == -1) && (errno != EINTR) )
     {
          perror("TcpServer::doAccept] poll() failed");
          m_state = POLL_ERROR;
          return;
     }
     if( (ret > 0) && (fd.revents & POLLIN) )
     {
          struct sockaddr_in clientAddr;
          socklen_t len = (socklen_t)sizeof(clientAddr);
          m_client = accept(m_socket, (struct sockaddr *)&clientAddr, &len);
          if( m_client < 0 )
          {
               if( errno != EINTR )
               {
                    perror("[TcpServer::doAccept] accept() failed");
               }
               return;
          }
          char remotehost[256];
          getnameinfo((struct sockaddr *)&clientAddr, len, remotehost, sizeof(remotehost), NULL, 0, NI_NUMERICHOST);
          std::printf("[TcpServer] Connected to : %s\n", remotehost);
          m_txBuffer.clear();
          m_rxBuffer.clear();
          m_requestReceived = false;
          m_requestPacket.clear();
          m_state = PROC_SESSION;
     }
}

//------------------------------------------------------------------------------
void TcpServer::doReceive()
{
     // 受信可能かどうか調べる
     fd_set mask;                  // selectの監視ディスクリプタセット
     struct timeval timeout;       // selectのタイムアウト値
     FD_ZERO(&mask);
     FD_SET(m_client, &mask);
     timeout.tv_sec = 0;
     timeout.tv_usec = 20000;
     int ret = select(m_client+1, &mask, NULL, NULL, &timeout);
     if( ret < 0 && errno != EINTR )
     {
          perror("[TcpServer::doReceive] select() failed");
          close(m_client);
          m_client = -1;
          m_state = LISTENING;
          std::printf("[TcpServer::doReceive] disconnected.\n");
          return;
     }
     if( ret > 0 )  //&& FD_ISSET(m_client, &mask) )
     {
          // std::printf("received\n");
          std::vector<uint8_t> buf(256);
          std::vector<uint8_t>::iterator is = buf.begin();
          std::vector<uint8_t>::iterator ie = buf.begin();
          ret = recv(m_client, &buf[0], buf.size(), 0);
          if( ret <= 0 )
          {
               // これはクライアント側が接続を切断したことを示す
               std::printf("[TcpServer::doReceive] disconnected by peer.\n");
               close(m_client);
               m_client = -1;
               m_state = LISTENING;
               return;
          }
          // std::printf("[TcpServer::doReceive] %d byte(s) received\n", ret);
          advance(ie, ret);
          while( is != ie )
          {
               m_rxBuffer.push_back(*is);
               ++is;
          }
          while( !m_rxBuffer.empty() && !m_requestReceived )
          {
               uint8_t c = m_rxBuffer.front();
               m_rxBuffer.pop_front();
               m_requestReceived = m_requestPacket.push(c);
          }
     }
}

//------------------------------------------------------------------------------
void TcpServer::doSend()
{
     if( m_txBuffer.empty() )
     {
          return;
     }

     // 送信可能か調べる
     fd_set mask;                  // selectの監視ディスクリプタセット
     struct timeval timeout;       // selectのタイムアウト値
     FD_ZERO(&mask);
     FD_SET(m_client, &mask);
     timeout.tv_sec = 0;
     timeout.tv_usec = 20000;
     int ret = select(m_client+1, NULL, &mask, NULL, &timeout);
     if( ret < 0 && errno != EINTR )
     {
          perror("[TcpServer::doSend] select() failed");
          close(m_client);
          m_client = -1;
          std::printf("[TcpServer::doSend] disconnected.\n");
          m_state = LISTENING;
     }
     if( ret > 0 && FD_ISSET(m_client, &mask) )
     {
          ret = send(m_client, &m_txBuffer[0], m_txBuffer.size(), 0);
          if( ret > 0 )
          {
               std::vector<uint8_t>::iterator ie = m_txBuffer.begin();
               advance(ie, ret);
               m_txBuffer.erase(m_txBuffer.begin(), ie);
          }
          else
          {
               perror("[TcpServer::doSend] send() failed");
               close(m_client);
               m_client = -1;
               m_state = LISTENING;
               std::printf("[TcpServer::doSend] disconnected.\n");
          }
     }
}

//------------------------------------------------------------------------------
//   レスポンスを送信
//------------------------------------------------------------------------------
void TcpServer::sendResponse(Packet& response)
{
     m_mutex.lock();
     std::vector<uint8_t> buffer;
     response.getRawBytes(buffer);
     m_txBuffer.insert(m_txBuffer.end(), buffer.begin(), buffer.end());
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
//   リクエストの受信を許可
//------------------------------------------------------------------------------
void TcpServer::enableRequest()
{
     m_mutex.lock();
     if( m_requestReceived )
     {
          m_requestReceived = false;
          m_requestPacket.clear();
     }
     m_mutex.unlock();
}

//------------------------------------------------------------------------------
//   受信したリクエストを取得する
//------------------------------------------------------------------------------
Packet *TcpServer::getRequest()
{
     m_mutex.lock();
     Packet *p = m_requestPacket.clone();
     m_mutex.unlock();
     return p;
}


//==============================================================================
//   CommandObject
//   コマンドを表現するための基本クラス
//==============================================================================
//   コンストラクタ
//------------------------------------------------------------------------------
CommandObject::CommandObject(int id, Robot *robot)
     : m_id(id), m_robot(robot)
{
}

//------------------------------------------------------------------------------
//   リクエストを処理
//------------------------------------------------------------------------------
void CommandObject::processRequest(Packet *request, Packet *response)
{
     uint8_t status = execute(request);
     response->create(m_id, request->getSerialNo());
     response->addPacketData(&status, 1);
     if( status == STS_OK )
     {
          setResponseData(response);
     }
}

//==============================================================================
//   EnableCommand (0)
//   モータの励磁をON/OFFするコマンド
//==============================================================================
EnableCommand::EnableCommand(Robot *robot)
     : CommandObject(EnableCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   +00 (2)   モータID (0, 1, 2)
//   +02 (2)   動作(0:励磁を切る、それ以外:励磁を入れる)
//------------------------------------------------------------------------------
uint8_t EnableCommand::execute(Packet *request)
{
     uint16_t motorID;
     if( !request->readUInt16Data(0, &motorID) || (motorID >= NUM_MOTORS) )
     {
          return STS_INVALID;
     }
     if( m_robot->isInMotion(motorID) || m_robot->isAlarmHappened(motorID) )
     {
          return STS_UNABLE;
     }
     uint16_t action;
     if( !request->readUInt16Data(2, &action) )
     {
          return STS_INVALID;
     }

     m_robot->enableMotor(motorID, action? true : false);

     return STS_OK;
}




//==============================================================================
//   ResetCommand (1)
//   アラームを解除するコマンド
//==============================================================================
ResetCommand::ResetCommand(Robot *robot)
     : CommandObject(ResetCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   +00 (2)   モータID (0, 1, 2)
//             これ以外の値を指定した場合は全軸を対象
//------------------------------------------------------------------------------
uint8_t ResetCommand::execute(Packet *request)
{
     uint16_t motorID;
     if( !request->readUInt16Data(0, &motorID) )  //|| (motorID >= NUM_MOTORS) )
     {
          return STS_INVALID;
     }
     for( uint16_t axis = 0 ; axis < NUM_MOTORS ; axis++ )
     {
          if( (motorID == axis) || (motorID >= NUM_MOTORS) )
          {
               if( !m_robot->isInMotion(axis) )
               {
                    m_robot->clearAlarm(axis);
               }
          }
     }
     return STS_OK;
}

//==============================================================================
//   StopCommand (2)
//   停止
//==============================================================================
StopCommand::StopCommand(Robot *robot)
     : CommandObject(StopCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   +00 (2)   モータID (0, 1, 2) 
//             これ以外の値を指定した場合は全軸を対象
//   +02 (2)   動作(0:即時停止、それ以外:減速停止)
//------------------------------------------------------------------------------
uint8_t StopCommand::execute(Packet *request)
{
     uint16_t motorID;
     if( !request->readUInt16Data(0, &motorID) ) //|| (motorID >= NUM_MOTORS) )
     {
          return STS_INVALID;
     }
     uint16_t action;
     if( !request->readUInt16Data(2, &action) )
     {
          return STS_INVALID;
     }
     for( uint16_t axis = 0 ; axis < NUM_MOTORS ; axis++ )
     {
          if( (motorID == axis) || (motorID >= NUM_MOTORS) )
          {
               if( !m_robot->isAlarmHappened(axis) )
               {
                    if( action )
                    {
                         m_robot->softStop(axis);
                    }
                    else
                    {
                         m_robot->hardStop(axis);
                    }
               }
          }
     }
     return STS_OK;
}

//==============================================================================
//   HomingCommand (3)
//   原点復帰
//==============================================================================
HomingCommand::HomingCommand(Robot *robot)
     : CommandObject(HomingCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   パラメータなし
//------------------------------------------------------------------------------
uint8_t HomingCommand::execute(Packet *request)
{
     if( !m_robot->startHoming() )
     {
          return STS_UNABLE;
     }
     return STS_OK;
}




//==============================================================================
//   MovetoCommand (4)
//   指定位置移動
//==============================================================================
MovetoCommand::MovetoCommand(Robot *robot)
     : CommandObject(MovetoCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   +00 (2)   モータID (0, 1, 2)
//   +02 (4)   移動先位置(pulse)
//------------------------------------------------------------------------------
uint8_t MovetoCommand::execute(Packet *request)
{
     uint16_t motorID;
     if( !request->readUInt16Data(0, &motorID) || (motorID >= NUM_MOTORS) )
     {
          return STS_INVALID;
     }
     if( m_robot->isInMotion(motorID) || m_robot->isAlarmHappened(motorID) )
     {
          return STS_UNABLE;
     }
     int32_t destpos;
     if( !request->readInt32Data(2, &destpos) )
     {
          return STS_INVALID;
     }
     bool ret = m_robot->startMotion(motorID, destpos);
     if( !ret )
     {
          return STS_UNABLE;
     }
     return STS_OK;
}

//==============================================================================
//   Move3DCommand (5)
//   ３軸の各目標位置を指定し，同時に駆動させる
//==============================================================================
Move3DCommand::Move3DCommand(Robot *robot)
     : CommandObject(Move3DCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   +00 (4)   BASE の移動先座標
//   +04 (4)   SHOULDER の移動先座標
//   +04 (4)   ELBOW の移動先座標
//------------------------------------------------------------------------------
uint8_t Move3DCommand::execute(Packet *request)
{
     int32_t destpos[3];
     uint32_t distance[3];
     uint32_t longest = 0;
     bool    b[3] = {true, true, true};
     uint8_t dir[3];

     for( int axis = 0 ; axis < NUM_MOTORS ; axis++ )
     {
          if( !request->readInt32Data(axis*4, &destpos[axis]) )
          {
               return STS_INVALID;
          }
          if( m_robot->isInMotion(axis) || m_robot->isAlarmHappened(axis) )
          {
               // このコマンドは，全軸が動作可能でないと実行できない
               return STS_UNABLE;
          }
          int32_t abspos = m_robot->getMotorPosition(axis);
          if( abspos == destpos[axis] )
          {
               b[axis] = false;    // この軸は動かす必要はない
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
          return STS_OK; // どの軸も動かす必要がない
     }
     // 各軸の速度を決定し，駆動開始
     for( int axis = 0 ; axis < NUM_MOTORS ; axis++ )
     {
          if( b[axis] )
          {
               int32_t speed = 16 * distance[axis] / longest;
               m_robot->setMotorParam(axis, L6470::PRM_MAX_SPEED, speed);
               m_robot->startMotion(axis, destpos[axis]);
          }
     }
     return STS_OK;
}



//==============================================================================
//   ReadParamCommand (6)
//   パラメータ取得
//==============================================================================
ReadParamCommand::ReadParamCommand(Robot *robot)
     : CommandObject(ReadParamCommand::ID, robot), m_motorID(0)
{
}

//------------------------------------------------------------------------------
//   +00 (2)   モータID (0, 1, 2)
//------------------------------------------------------------------------------
uint8_t ReadParamCommand::execute(Packet *request)
{
     if( request->readUInt16Data(0, &m_motorID) || (m_motorID >= NUM_MOTORS) )
     {
          return STS_INVALID;
     }
     return STS_OK;
}

//------------------------------------------------------------------------------
void ReadParamCommand::setResponseData(Packet *response)
{
     for( uint8_t n = 0 ; n < 32 ; n++ )
     {
          uint32_t param = m_robot->getMotorParam(m_motorID, n);
          response->addPacketData(&param, 4);
     }
}




//==============================================================================
//   WriteParamCommand (7)
//   パラメータ書き込み
//==============================================================================
WriteParamCommand::WriteParamCommand(Robot *robot)
     : CommandObject(WriteParamCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   +00 (2)   モータID (0, 1, 2)
//   +02 (2)   パラメータID (0〜31)
//   +04 (4)   パラメータ値
//------------------------------------------------------------------------------
uint8_t WriteParamCommand::execute(Packet *request)
{
     uint16_t motorID;
     uint16_t paramID;
     uint32_t value;

     if( !request->readUInt16Data(0, &motorID) || (motorID >= NUM_MOTORS) )
     {
          return STS_INVALID;
     }
     if( !request->readUInt16Data(2, &paramID) || (paramID >= 32) )
     {
          return STS_INVALID;
     }
     if( !request->readUInt32Data(4, &value) )
     {
          return STS_INVALID;
     }
     m_robot->setMotorParam(motorID, paramID, value);

     return STS_OK;
}




//==============================================================================
//   SaveParamCommand (8)
//   パラメータをFLASHへ保存
//==============================================================================
SaveParamCommand::SaveParamCommand(Robot *robot)
     : CommandObject(SaveParamCommand::ID, robot)
{

}

//------------------------------------------------------------------------------
//   +00 (2)   モータID (0, 1, 2)
//------------------------------------------------------------------------------
uint8_t SaveParamCommand::execute(Packet *request)
{
     // uint16_t motorID;
     // if( !request->readUInt16Data(0, &motorID) || (motorID >= NUM_MOTORS) )
     // {
     //      return STS_INVALID;
     // }

     // char path[20];
     // sprintf(path, "/param_%d.dat", motorID);

     // File f = SPIFFS.open(path, FILE_WRITE);
     // if( !f )
     // {
     //      return STS_FAIL;
     // }
     // for( uint8_t n = 0 ; n < 32 ; n++ )
     // {
     //      uint32_t param = m_robot->stepper(motorID)->getParam(n);
     //      f.write((uint8_t *)&param, 4);
     // }
     // f.close();
     return STS_OK;
}




//==============================================================================
//   StatusCommand (9)
//   ステータス取得
//==============================================================================
StatusCommand::StatusCommand(Robot *robot)
     : CommandObject(StatusCommand::ID, robot)
{
}

//------------------------------------------------------------------------------
void StatusCommand::setResponseData(Packet *response)
{
     int32_t pos, spd;
     uint8_t v;

     // グリッパーの変位
     v = m_robot->getGripperValue();
     response->addPacketData(&v, 1);

     for( int n = 0 ; n < NUM_MOTORS ; n++ )
     {
          uint16_t status = m_robot->getMotorStatus(n);

          pos = m_robot->getMotorPosition(n);
          response->addPacketData(&pos, 4);  // +00

          spd = m_robot->getMotorSpeed(n);
          response->addPacketData(&spd, 4);  // +04

          v = m_robot->isHalted(n)? 0xFF : 0x00;
          response->addPacketData(&v, 1);    // +08

          v = m_robot->isInMotion(n)? 0xFF : 0x00;
          response->addPacketData(&v, 1);    // +09

          v = m_robot->isHomeCompleted(n)? 0xFF : 0x00;
          response->addPacketData(&v, 1);    // +10

          v = m_robot->getLimitState(n, L6470::DIR_REVERSE)? 0x00 : 0xFF;
          response->addPacketData(&v, 1);    // +11

          v = (status & 0x04)? 0xFF : 0x00;
          response->addPacketData(&v, 1);    // +12

          v = m_robot->getLimitState(n, L6470::DIR_FORWARD)? 0x00 : 0xFF;
          response->addPacketData(&v, 1);    // +13

          v = m_robot->getAlarmFlag(n);
          response->addPacketData(&v, 1);    // +14

          v = 0;    // reserved
          response->addPacketData(&v, 1);    // +15
     }
}


//==============================================================================
//   GripperCommand (10)
//   グリッパー制御
//==============================================================================
GripperCommand::GripperCommand(Robot *robot)
     : CommandObject(GripperCommand::ID, robot)
{
}

//------------------------------------------------------------------------------
uint8_t GripperCommand::execute(Packet *request)
{
     uint8_t value;
     if( !request->readUInt8Data(0, &value) || (value > 100) )
     {
          return STS_INVALID;
     }

     m_robot->moveGripper(value);
     return STS_OK;
}


//==============================================================================
//   CommandManager
//==============================================================================
//   コンストラクタ
//------------------------------------------------------------------------------
CommandManager::CommandManager(Robot *robot) : m_robot(robot), m_terminated(false)
{
     m_command[EnableCommand::ID    ] = new EnableCommand(robot);
     m_command[ResetCommand::ID     ] = new ResetCommand(robot);
     m_command[StopCommand::ID      ] = new StopCommand(robot);
     m_command[HomingCommand::ID    ] = new HomingCommand(robot);
     m_command[MovetoCommand::ID    ] = new MovetoCommand(robot);
     m_command[Move3DCommand::ID    ] = new Move3DCommand(robot);
     m_command[ReadParamCommand::ID ] = new ReadParamCommand(robot);
     m_command[WriteParamCommand::ID] = new WriteParamCommand(robot);
     m_command[SaveParamCommand::ID ] = new SaveParamCommand(robot);
     m_command[StatusCommand::ID    ] = new StatusCommand(robot);
     m_command[GripperCommand::ID   ] = new GripperCommand(robot);

     m_thread = new std::thread([this](){ execute(); });
}

//------------------------------------------------------------------------------
//   デストラクタ
//------------------------------------------------------------------------------
CommandManager::~CommandManager()
{
     m_terminated = true;
     m_thread->join();
     delete m_thread;
     for( std::map<int, CommandObject *>::iterator i = m_command.begin() ; i != m_command.end() ; ++i )
     {
          delete i->second;
     }
}

//------------------------------------------------------------------------------
//   コマンド処理スレッド
//------------------------------------------------------------------------------
void CommandManager::execute()
{
     Packet response;

     while( !m_terminated )
     {
          if( m_server.available() )
          {
               Packet *request = m_server.getRequest();
               int id = request->getID();
               // std::printf("[CommandManager] Request received (%d)\n", id);
               std::map<int, CommandObject *>::iterator f = m_command.find(id);
               if( f != m_command.end() )
               {
                    f->second->processRequest(request, &response);
                    m_server.sendResponse(response);
               }
               delete request;
               m_server.enableRequest();
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
     }
}

//-- End of CmdMgr.cpp ---------------------------------------------------------
