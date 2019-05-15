#ifndef   TCPSERVER_H
#define   TCPSERVER_H

#include "tcpsession.h"

typedef void (*RecvCmdEvent)(TcpSession *);

class TcpServer
{
     private:
          int            m_nSocket;
          int            m_nAccept;
          int            m_nState;
          unsigned short m_nPort;
          char           m_szHost[256];
          RecvCmdEvent   m_pfnOnRecv;
          TcpSession     m_Session;

          void DoAccept();
          void ProcessSession();

     public:
          enum{INIT_FAILED = -1, LISTENING, PROC_SESSION, POLL_ERROR};
          TcpServer(unsigned short, RecvCmdEvent);
          ~TcpServer();
          void ProcessService();
          char *GetHostName(){ return m_szHost; }
          int  GetState(){ return m_nState; }
};

#endif
