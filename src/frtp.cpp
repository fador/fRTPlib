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

uint32_t fRTPCreateOutConn(fRTPState* state, std::string sendAddr, int sendPort, int fromPort)
{
  fRTPConnection newConn;
 

  if ((newConn.socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    return FRTP_ERROR;
  }

  memset(&newConn.addrOut, 0, sizeof(newConn.addrOut));
  newConn.addrOut.sin_family = AF_INET;
  inet_pton(AF_INET, sendAddr.c_str(), &(newConn.addrOut.sin_addr));
  newConn.addrOut.sin_port = htons(sendPort);

  memset(&newConn.addrIn, 0, sizeof(newConn.addrIn));
  newConn.addrIn.sin_family = AF_INET;  
  newConn.addrIn.sin_addr.s_addr = htonl(INADDR_ANY);
  newConn.addrIn.sin_port = htons(fromPort);

  if (bind(newConn.socket, (struct sockaddr *) &newConn.addrIn, sizeof(newConn.addrIn)) < 0) {
    return FRTP_ERROR;
  }

  newConn.ID = fRTPGetID();

  state->_outgoing.push_back(newConn);
  return newConn.ID;
}

uint32_t fRTPCreateInConn(fRTPState* state, std::string listenAddr, int listenPort)
{
  fRTPConnection newConn;

  // Socket initialization
  newConn.socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (newConn.socket < 0) {
    return FRTP_ERROR;
  }

  // Define listen address and port
  memset((char *)&newConn.addrIn, 0, sizeof(newConn.addrIn));
  newConn.addrIn.sin_family = AF_INET;

  // Read IP from string
  inet_pton(AF_INET, listenAddr.c_str(), &(newConn.addrIn.sin_addr));
  newConn.addrIn.sin_port = htons(listenPort);  

  if (bind(newConn.socket, (struct sockaddr *) &newConn.addrIn, sizeof(newConn.addrIn)) < 0) {
    std::cerr << "Inbound connection bind failure" << std::endl;
    return FRTP_ERROR;
  }
  newConn.ID = fRTPGetID();
  state->_incoming.push_back(newConn);
  return newConn.ID;
}

uint32_t fRTPCreateConnPair(fRTPState * state, std::string listenAddr, int listenPort, std::string sendAddr, int sendPort)
{
  fRTPPair newPair;
  newPair.ID = fRTPGetID();
  newPair.incoming = fRTPCreateInConn(state, listenAddr, listenPort);
  newPair.outgoing = fRTPCreateOutConn(state, sendAddr, sendPort, listenPort);

  if (newPair.incoming == FRTP_ERROR || newPair.outgoing == FRTP_ERROR) return FRTP_ERROR;
  
  state->_pairs.push_back(newPair);

  return newPair.ID;
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