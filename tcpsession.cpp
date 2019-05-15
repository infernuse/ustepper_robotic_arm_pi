#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "tcpsession.h"


//------------------------------------------------------------------------------
//   コンストラクタ
//------------------------------------------------------------------------------
TcpSession::TcpSession() : m_nState(READY)
{
}

//------------------------------------------------------------------------------
//   受信準備完了
//------------------------------------------------------------------------------
void TcpSession::SetReady()
{
     m_RecvBuf.clear();
     m_SendBuf.clear();
     m_nState = RECEIVING;
}

//------------------------------------------------------------------------------
//   受信処理
//------------------------------------------------------------------------------
void TcpSession::Recv(int sock)
{
     if( m_nState != RECEIVING )
     {
          return;
     }

     // 受信可能かどうか調べる
     fd_set mask;                  // selectの監視ディスクリプタセット
     struct timeval timeout;       // selectのタイムアウト値
     FD_ZERO(&mask);
     FD_SET(sock, &mask);
     timeout.tv_sec = 0;
     timeout.tv_usec = 20000;
     int nRet = select(sock+1, &mask, NULL, NULL, &timeout);
     if( nRet < 0 && errno != EINTR )
     {
          perror("[TcpSession::Recv] select() failed");
          m_nState = SESSION_FAILED;
          return;
     }
     if( nRet > 0 && FD_ISSET(sock, &mask) )
     {
          // 受信を行う
          vector<unsigned char> buf(256);
          vector<unsigned char>::iterator is = buf.begin();
          vector<unsigned char>::iterator ie = buf.begin();
          nRet = recv(sock, &buf[0], buf.size(), 0);
          if( nRet <= 0 )
          {
               // これはクライアント側が接続を切断したことを示す
               m_nState = SESSION_FAILED;
               return;
          }
          advance(ie, nRet);
          m_RecvBuf.insert(m_RecvBuf.end(), is, ie);
          if( m_RecvBuf.size() >= 4 )
          {
               int nSize = (int)ntohs(*((unsigned short *)&m_RecvBuf[2]));
               if( (int)m_RecvBuf.size() >= nSize+4 )
               {
                    m_nState = RECEIVED;
               }
          }
     }
}

//------------------------------------------------------------------------------
//   送信処理
//------------------------------------------------------------------------------
void TcpSession::Send(int sock)
{
     if( m_nState != SENDING )
     {
          return;
     }

     // 送信可能か調べる
     fd_set mask;                  // selectの監視ディスクリプタセット
     struct timeval timeout;       // selectのタイムアウト値
     FD_ZERO(&mask);
     FD_SET(sock, &mask);
     timeout.tv_sec = 0;
     timeout.tv_usec = 20000;
     int ret = select(sock+1, NULL, &mask, NULL, &timeout);
     if( ret < 0 && errno != EINTR )
     {
          perror("[TcpSession::Send] select() failed");
          m_nState = SESSION_FAILED;
     }
     if( ret > 0 && FD_ISSET(sock, &mask) )
     {
          int nRet = send(sock, &m_SendBuf[0], m_SendBuf.size(), 0);
          if( nRet > 0 )
          {
               vector<unsigned char>::iterator ie = m_SendBuf.begin();
               advance(ie, nRet);
               m_SendBuf.erase(m_SendBuf.begin(), ie);
               if( m_SendBuf.empty() )
               {
                    m_nState = FINISHED;
               }
          }
          else
          {
               m_nState = SESSION_FAILED;
          }
     }
}

//------------------------------------------------------------------------------
//   コマンド識別子を取得
//------------------------------------------------------------------------------
int TcpSession::GetID()
{
     if( m_RecvBuf.size() > 0 )
     {
          return m_RecvBuf[0];
     }
     return 0;
}

//------------------------------------------------------------------------------
//   受信パケットのパラメータ部を取得
//------------------------------------------------------------------------------
void TcpSession::GetParam(vector<unsigned char>& buf)
{
     buf.clear();
     if( m_nState == RECEIVED )
     {
          vector<unsigned char>::iterator is = m_RecvBuf.begin();
          advance(is, 4);
          buf.insert(buf.end(), is, m_RecvBuf.end());
     }
}

//------------------------------------------------------------------------------
//   送信データ(パラメータ)を設定
//------------------------------------------------------------------------------
void TcpSession::AddParamByte(unsigned char d)
{
     if( m_nState < RECEIVED )
     {
          return;
     }

     if( m_SendBuf.empty() )
     {
          m_SendBuf.resize(4);
          m_SendBuf[0] = m_RecvBuf[0];
          m_SendBuf[1] = m_RecvBuf[1];
     }

     m_SendBuf.push_back(d);
}
void TcpSession::AddParamWord(unsigned short d)
{
     if( m_nState < RECEIVED )
     {
          return;
     }

     if( m_SendBuf.empty() )
     {
          m_SendBuf.resize(4);
          m_SendBuf[0] = m_RecvBuf[0];
          m_SendBuf[1] = m_RecvBuf[1];
     }

     vector<unsigned char> tmp(2);
     *((unsigned short *)&tmp[0]) = htons(d);
     m_SendBuf.insert(m_SendBuf.end(), tmp.begin(), tmp.end());
}
void TcpSession::AddParamLong(unsigned int d)
{
     if( m_nState < RECEIVED )
     {
          return;
     }

     if( m_SendBuf.empty() )
     {
          m_SendBuf.resize(4);
          m_SendBuf[0] = m_RecvBuf[0];
          m_SendBuf[1] = m_RecvBuf[1];
     }

     vector<unsigned char> tmp(4);
     *((unsigned int *)&tmp[0]) = htonl(d);
     m_SendBuf.insert(m_SendBuf.end(), tmp.begin(), tmp.end());
}

//------------------------------------------------------------------------------
//   ホスト側のコマンド処理を完了し、送信を開始する
//------------------------------------------------------------------------------
void TcpSession::ProcessDone()
{
     if( m_nState != RECEIVED )
     {
          return;
     }

     if( m_SendBuf.empty() )
     {
          m_SendBuf.resize(4);
          m_SendBuf[0] = m_RecvBuf[0];
          m_SendBuf[1] = m_RecvBuf[1];
     }
     *((unsigned short *)&m_RecvBuf[2]) = htons(m_RecvBuf.size()-4);
     m_nState = SENDING;
}

