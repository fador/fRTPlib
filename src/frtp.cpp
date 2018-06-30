#include <iostream>
#include <thread>


#include "frtp.h"
#include "frtp_hevc.h"

uint32_t fRTPGetID()
{
  static uint32_t curID = 1;
  return curID++;
}

fRTPState * fRTPInit()
{

#ifdef _WIN32
  WSADATA wsaData;
  int iResult;
  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    return nullptr;
  }
#endif
  fRTPState* state = new fRTPState();
  return state;
}


fRTPConnection* fRTPInternalConnIDToStruct(fRTPState * state, uint32_t connID) {
  for (uint32_t i = 0; i < state->_connection.size(); i++) {
    if (state->_connection[i]->ID == connID) {
      return state->_connection[i];
    }
  }
  return nullptr;
}

uint32_t fRTPInternalRecvRTP(fRTPConnection* conn)
{
  sockaddr_in fromAddr;
  while (1) {
    int fromAddrSize = sizeof(fromAddr);
    int32_t ret = recvfrom(conn->socket, (char *)conn->inPacketBuffer, conn->inPacketBufferLen, 0, (SOCKADDR *)&fromAddr, &fromAddrSize);
    if (ret == -1) {
      /*
      int _error = WSAGetLastError();
      if(_error != 10035)
        std::cerr << "Socket error" << _error << std::endl;
      return FRTP_ERROR;
      */
    }
    else {
      std::cerr << "Received" << ret << "bytes" << std::endl;
      //return FRTP_OK;
    }
    
  }

}


uint32_t fRTPCreateConn(fRTPState* state, std::string sendAddr, int sendPort, int fromPort)
{
  fRTPConnection* newConn = new fRTPConnection();
 

  if ((newConn->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return FRTP_ERROR;
  }

  memset(&newConn->addrOut, 0, sizeof(newConn->addrOut));
  newConn->addrOut.sin_family = AF_INET;
  inet_pton(AF_INET, sendAddr.c_str(), &(newConn->addrOut.sin_addr));
  newConn->addrOut.sin_port = htons(sendPort);

  memset(&newConn->addrIn, 0, sizeof(newConn->addrIn));
  newConn->addrIn.sin_family = AF_INET;  
  newConn->addrIn.sin_addr.s_addr = htonl(INADDR_ANY);
  newConn->addrIn.sin_port = htons(fromPort);

  if (bind(newConn->socket, (struct sockaddr *) &newConn->addrIn, sizeof(newConn->addrIn)) < 0) {
    return FRTP_ERROR;
  }

  struct timeval read_timeout;
  read_timeout.tv_sec = 0;
  read_timeout.tv_usec = 10;

  newConn->inPacketBuffer = new uint8_t[MAX_PACKET];
  newConn->inPacketBufferLen = MAX_PACKET;

  // Start a blocking recv thread
  newConn->runnerThread = new std::thread(fRTPInternalRecvRTP, newConn);
  
  newConn->ID = fRTPGetID();

  state->_connection.push_back(newConn);
  return newConn->ID;
}

uint32_t fRTPInternalRemoveConn(fRTPState * state, uint32_t connID)
{
  for (uint32_t i = 0; i < state->_connection.size(); i++) {
    if (state->_connection[i]->ID == connID) {
      state->_connection.erase(state->_connection.begin()+i);
      return FRTP_OK;
    }
  }
  return FRTP_ERROR;
}



uint32_t fRTPCloseConn(fRTPState * state, uint32_t connID)
{
  fRTPConnection* conn = fRTPInternalConnIDToStruct(state, connID);

  if (conn == nullptr) return FRTP_ERROR;


  if (conn->runnerThread != nullptr) {
    delete conn->runnerThread;
    conn->runnerThread = nullptr;
  }

#ifdef _WIN32
  closesocket(conn->socket);
#else
  close(conn->socket);
#endif  

  return fRTPInternalRemoveConn(state, connID);
}

uint32_t fRTPInternalSendRTP(fRTPConnection* conn, uint8_t* data, uint32_t datalen, uint8_t marker)
{
  uint8_t buffer[MAX_PACKET];
    
  buffer[0] = 2 << 6; // RTP version
  buffer[1] = (conn->rtp_payload & 0x7f) | (marker << 7);
  *(uint16_t*)&buffer[2] = htons(conn->rtp_sequence);
  *(uint32_t*)&buffer[4] = htonl(conn->rtp_timestamp);
  *(uint32_t*)&buffer[8] = htonl(conn->rtp_ssrc);
  memcpy(&buffer[12], data, datalen);


  if (sendto(conn->socket, (const char*)buffer, datalen + 12, 0, (struct sockaddr*) &conn->addrOut, sizeof(conn->addrOut)) == -1)
  {
#ifdef FRTP_VERBOSE
    std::cerr << "[fRTP] sendto failure" << std::endl;
#endif
    return FRTP_ERROR;
  }

  conn->rtp_sequence++; // Overflow ok

  // Update statistics
  conn->processedBytes += datalen;
  conn->overheadBytes += 12;
  conn->totalBytes += datalen+12;
  conn->processedPackets++;
  return FRTP_OK;
}


uint32_t fRTPPushFrame(fRTPState * state, uint32_t connID, uint8_t* data, uint32_t datalen, fRTPFormat fmt, uint32_t timestamp)
{

  for (uint32_t i = 0; i < state->_connection.size(); i++) {
    if (state->_connection[i]->ID == connID) {
      return fRTPPushFrame(state->_connection[i], data, datalen, fmt, timestamp);
    }
  }
  return FRTP_ERROR;
}


uint32_t fRTPPushFrame(fRTPConnection* conn, uint8_t* data, uint32_t datalen, fRTPFormat fmt, uint32_t timestamp)
{

  switch (fmt) {
    case FRTP_HEVC:
      fRTPInternalPushHEVCFrame(conn, data, datalen, timestamp);
      break;
    case FRTP_OPUS:
      break;
    default:
      break;
  }

  return FRTP_OK;
}