#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include "tcpserver.h"

//------------------------------------------------------------------------------
//   コンストラクタ
//------------------------------------------------------------------------------
TcpServer::TcpServer(unsigned short nPort, RecvCmdEvent e)
     : m_nPort(nPort), m_nAccept(-1), m_pfnOnRecv(e)
{
     m_nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if( m_nSocket < 0 )
     {
          perror("[TcpServer] socket() failed");
          m_nState = INIT_FAILED;
          return;
     }

     // ソケットオプションを設定
     int nON = 1;
     setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, &nON, sizeof(nON));

     // アドレスをバインド
     struct sockaddr_in servAddr;
     memset(&servAddr, 0, sizeof(servAddr));
     servAddr.sin_family = AF_INET;
     servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     servAddr.sin_port = htons(m_nPort);
     if( bind(m_nSocket, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0 )
     {
          perror("[TcpServer] bind() failed");
          m_nState = INIT_FAILED;
          return;
     }

     if( listen(m_nSocket, SOMAXCONN) < 0 )
     {
          perror("[TcpServer] listen() failed");
          m_nState = INIT_FAILED;
          return;
     }

     m_nState = LISTENING;
     printf("[TcpServer] Waiting for a new connection.\n");
}

//------------------------------------------------------------------------------
//   デストラクタ
//------------------------------------------------------------------------------
TcpServer::~TcpServer()
{
     close(m_nAccept);
     close(m_nSocket);
}

//------------------------------------------------------------------------------
//   サービス処理
//------------------------------------------------------------------------------
void TcpServer::ProcessService()
{
     switch( m_nState )
     {
          case LISTENING:
               DoAccept();
               break;
          case PROC_SESSION:
               ProcessSession();
               break;
     }
}

//------------------------------------------------------------------------------
//   クライアントのアクセプト処理
//------------------------------------------------------------------------------
void TcpServer::DoAccept()
{
     struct pollfd fd;
     fd.fd = m_nSocket;
     fd.events = POLLIN;
     int nRet = poll(&fd, 1, 0);
     if( (nRet == -1) && (errno != EINTR) )
     {
          perror("TcpServer::DoAccept] poll() failed");
          m_nState = POLL_ERROR;
          return;
     }
     if( (nRet > 0) && (fd.revents & POLLIN) )
     {
          struct sockaddr_in clientAddr;
          socklen_t len = (socklen_t)sizeof(clientAddr);
          m_nAccept = accept(m_nSocket, (struct sockaddr *)&clientAddr, &len);
          if( m_nAccept < 0 )
          {
               if( errno != EINTR )
               {
                    perror("[TcpServer::DoAccept] accept() failed");
               }
               return;
          }
          getnameinfo((struct sockaddr *)&clientAddr, len, m_szHost, sizeof(m_szHost), NULL, 0, NI_NUMERICHOST);
          printf("[TcpServer] Connected to : %s\n", m_szHost);
          m_nState = PROC_SESSION;
          m_Session.SetReady();
     }
}

//------------------------------------------------------------------------------
//   クライアントとのセッション処理
//------------------------------------------------------------------------------
void TcpServer::ProcessSession()
{
     switch( m_Session.GetState() )
     {
          case TcpSession::READY:
          case TcpSession::RECEIVING:
               m_Session.Recv(m_nAccept);
               break;
          case TcpSession::RECEIVED:
               m_pfnOnRecv(&m_Session);
               break;
          case TcpSession::SENDING:
               m_Session.Send(m_nAccept);
               break;
          case TcpSession::FINISHED:
               m_Session.SetReady();
               break;
          case TcpSession::SESSION_FAILED:
               close(m_nAccept);
               printf("[TcpServer] Disconnected from : %s\n", m_szHost);
               m_nAccept = -1;
               memset(m_szHost, 0, sizeof(m_szHost));
               m_nState = LISTENING;
               printf("[TcpServer] Waiting for a new connection.\n");
               break;
          default:
               break;
     }
}

