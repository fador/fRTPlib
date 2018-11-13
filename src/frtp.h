#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <thread>
#ifdef _WIN32
  #define _WINSOCKAPI_
  #include <winsock2.h>
  #include <Ws2tcpip.h>
#else
  #include<arpa/inet.h>
  #include<sys/socket.h>
#endif

#define MAX_PACKET 65535
#define MAX_PAYLOAD 1500
#define FRTP_OK 0x0
#define FRTP_ERROR 0xFFFFFFFF

#define FRTP_API __declspec(dllexport)   
  
//#define FRTP_VERBOSE


enum fRTPFormat {
  FRTP_GENERIC,
  FRTP_HEVC,
  FRTP_OPUS
};

struct fRTPConfig_Opus {
  uint32_t samplerate;
  uint8_t channels;
  uint8_t configurationNumber;
};

struct fRTPFrameOut {
  uint32_t rtp_timestamp;
  uint32_t datalen;
  uint32_t rtp_ssrc;
  uint16_t rtp_sequence;
  uint8_t* data;
  uint8_t rtp_payload;
  uint8_t marker;
  fRTPFormat fmt;
};

struct fRTPConnection
{

  fRTPConnection() {
    config = nullptr;
    inFormat = FRTP_GENERIC;
    outFormat = FRTP_GENERIC;
  }

  fRTPConnection(const fRTPConnection& conn) {
    config = nullptr;
    inFormat = FRTP_GENERIC;
    outFormat = FRTP_GENERIC;
  }


  ~fRTPConnection() {
    free(config);
  }

  void setFormat(fRTPFormat in, fRTPFormat out) {
    inFormat = in;
    outFormat = out;
  }

  uint32_t ID;
  uint64_t socket;
  
  sockaddr_in addrIn;
  sockaddr_in addrOut;

  fRTPFormat inFormat;
  fRTPFormat outFormat;
  
  // Receiving
  std::thread* runnerThread;
  uint8_t* inPacketBuffer; // Buffer for incoming packet (MAX_PACKET)
  uint32_t inPacketBufferLen;

  uint8_t* frameBuffer; // Larger buffer for storing incoming frame
  uint32_t frameBufferLen;

  std::vector<fRTPFrameOut*> framesOut;


  // RTP
  uint16_t rtp_sequence;
  uint8_t  rtp_payload;
  uint32_t rtp_timestamp;
  uint32_t rtp_ssrc;

  // Statistics
  uint32_t processedBytes;
  uint32_t overheadBytes;
  uint32_t totalBytes;
  uint32_t processedPackets;

  uint8_t* config;
};

struct fRTPPair
{
  uint32_t ID;
  uint32_t incoming;
  uint32_t outgoing;
};

struct fRTPState {
  std::vector<fRTPConnection*> _connection;
};

uint32_t fRTPGetID();

FRTP_API fRTPState* fRTPInit();

FRTP_API uint32_t fRTPSetFormat(fRTPState * state, uint32_t connID, fRTPFormat in, fRTPFormat out);
FRTP_API uint32_t fRTPSetConfig(fRTPState * state, uint32_t connID, uint8_t* config);
FRTP_API fRTPFrameOut* fRTPGetReceived(fRTPState * state, uint32_t connID);
FRTP_API uint32_t fRTPCreateConn(fRTPState* state, std::string sendAddr, int sendPort, int fromPort);
FRTP_API uint32_t fRTPCloseConn(fRTPState* conn, uint32_t connID);
FRTP_API uint32_t fRTPPushFrame(fRTPState * state, uint32_t connID, uint8_t* data, uint32_t datalen, fRTPFormat fmt, uint32_t timestamp);
FRTP_API uint32_t fRTPPushFrame(fRTPConnection* conn, uint8_t* data, uint32_t datalen, fRTPFormat fmt, uint32_t timestamp);