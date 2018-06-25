#include <iostream>
#include <cstdint>

#include "frtp.h"
#include "frtp_internal.h"
#include "frtp_hevc.h"


static uint32_t fRTPInternalNextStart(fRTPConnection* conn, uint8_t* data, uint32_t offset, uint32_t datalen, uint8_t& startLen)
{
  uint8_t zeros = 0;
  uint32_t pos = 0;
  while (offset + pos < datalen) {
    if (zeros >= 2 && (data[offset + pos] == 1 || data[offset + pos] == 2)) {
      startLen = zeros + 1;
      return offset + pos + 1;
    }
    if (data[offset + pos] == 0) zeros++;
    else zeros = 0;
    
    pos++;
  }
  return FRTP_ERROR;
}

uint32_t fRTPInternalPushHEVCFrame(fRTPConnection* conn, uint8_t* data, uint32_t datalen, uint32_t timestamp)
{
  uint8_t startLen;
  uint32_t previousOffset = 0;
  uint32_t offset = fRTPInternalNextStart(conn, data, 0, datalen, startLen);
  do {    
    offset = fRTPInternalNextStart(conn, data, offset, datalen, startLen);
    if (offset > 4 && offset != FRTP_ERROR) {
      fRTPInternalSendHEVCNal(conn, &data[previousOffset], offset - previousOffset - startLen, timestamp);
      previousOffset = offset;
    }
  } while (offset != FRTP_ERROR);
  fRTPInternalSendHEVCNal(conn, &data[previousOffset], datalen- previousOffset,timestamp);

  return FRTP_OK;
}

uint32_t fRTPInternalSendHEVCNal(fRTPConnection* conn, uint8_t* data, uint32_t datalen, uint32_t timestamp)
{
  uint8_t buffer[MAX_PACKET];
  uint32_t bufferlen = 0;
  uint32_t dataLeft = datalen;
  uint32_t dataPos = 0;
  // Send unfragmented
  if (datalen <= MAX_PAYLOAD) {
    fRTPInternalSendRTP(conn, data, datalen, 0);
  }
  // Send fragmented
  else {
    uint8_t nalType = (data[0] >> 1) & 0x3F;

    buffer[0] = 49 << 1; // fragmentation unit
    buffer[1] = 1; // TID

    buffer[2] = nalType;
    buffer[2] |= 1 << 7; // S-bit

    while (dataLeft + 3 > MAX_PAYLOAD) {
      memcpy(&buffer[3], &data[dataPos], MAX_PAYLOAD - 3);
      fRTPInternalSendRTP(conn, buffer, MAX_PAYLOAD, 0);
      dataPos += MAX_PAYLOAD;
      dataLeft -= MAX_PAYLOAD;
      buffer[2] &= ~(1 << 7);
    }
    buffer[2] |= 1 << 6;
    memcpy(&buffer[3], &data[dataPos], dataLeft);
    fRTPInternalSendRTP(conn, buffer, dataLeft+3, 1);
  }

  return FRTP_OK;  
}