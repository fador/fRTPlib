#include <iostream>
#include <cstdint>

#include "frtp.h"
#include "frtp_internal.h"
#include "frtp_opus.h"


uint32_t fRTPInternalPushOPUSFrame(fRTPConnection* conn, uint8_t* data, uint32_t datalen, uint32_t timestamp)
{
  uint8_t buffer[MAX_PACKET];
  uint32_t bufferlen = 0;
  uint32_t dataLeft = datalen;
  uint32_t dataPos = 0;
  conn->rtp_payload = 97;

  fRTPConfig_Opus* config = (fRTPConfig_Opus*)conn->config;
  uint8_t configByte = (config->configurationNumber << 3) | (config->channels > 1 ? (1 << 2) : 0) | 0;

  buffer[0] = configByte;

  memcpy(&buffer[1], data, datalen);
  if (fRTPInternalSendRTP(conn, buffer, datalen + 1, 0) == FRTP_ERROR) {
    return FRTP_ERROR;
  }


  return FRTP_OK;  
}