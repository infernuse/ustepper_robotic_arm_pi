#ifndef TCPSESSION_H
#define TCPSESSION_H

#include <vector>
using namespace std;

class TcpSession
{
     private:
          vector<unsigned char>    m_SendBuf;
          vector<unsigned char>    m_RecvBuf;
          int                      m_nState;
     public:
          enum{READY, RECEIVING, RECEIVED, SENDING, FINISHED, SESSION_FAILED};
          TcpSession();
          void SetReady();
          void Recv(int);
          void Send(int);
          int  GetState(){ return m_nState; }
          int  GetID();
          void GetParam(vector<unsigned char>&);
          void AddParamByte(unsigned char);
          void AddParamWord(unsigned short);
          void AddParamLong(unsigned int);
          void ProcessDone();
};

#endif
