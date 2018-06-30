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
    if (zeros >= 2 && data[offset + pos] == 1) {
      startLen = zeros + 1;
      return offset + pos +1;
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
  previousOffset = offset;
  conn->rtp_payload = 96;
  while (offset != FRTP_ERROR) {
    offset = fRTPInternalNextStart(conn, data, offset, datalen, startLen);
    if (offset > 4 && offset != FRTP_ERROR) {
      if (fRTPInternalSendHEVCNal(conn, &data[previousOffset], offset - previousOffset - startLen, timestamp) == FRTP_ERROR) {
        return FRTP_ERROR;
      }
      previousOffset = offset;
    }
  }
  return fRTPInternalSendHEVCNal(conn, &data[previousOffset], datalen- previousOffset,timestamp);

}

uint32_t fRTPInternalSendHEVCNal(fRTPConnection* conn, uint8_t* data, uint32_t datalen, uint32_t timestamp)
{
  uint8_t buffer[MAX_PACKET];
  uint32_t bufferlen = 0;
  uint32_t dataLeft = datalen;
  uint32_t dataPos = 0;

  // Send unfragmented
  if (datalen <= MAX_PAYLOAD) {
#ifdef FRTP_VERBOSE
    std::cerr << "[fRTP] send unfrag size " << datalen << " type " << (uint32_t)((data[0] >> 1) & 0x3F) << std::endl;
#endif
    fRTPInternalSendRTP(conn, data, datalen, 0);
  }

  // Send fragmented
  else {
#ifdef FRTP_VERBOSE
    std::cerr << "[fRTP] send frag size " << datalen << std::endl;
#endif
    uint8_t nalType = (data[0] >> 1) & 0x3F;

    buffer[0] = 49 << 1; // fragmentation unit
    buffer[1] = 1; // TID

    // Set the S bit with NAL type
    buffer[2] = 1 << 7 | nalType;
    dataPos = 2;
    dataLeft -= 2;
    
    // Send full payload data packets
    while (dataLeft + 3 > MAX_PAYLOAD) {
      memcpy(&buffer[3], &data[dataPos], MAX_PAYLOAD - 3);
      if (fRTPInternalSendRTP(conn, buffer, MAX_PAYLOAD, 0) == FRTP_ERROR) {
        return FRTP_ERROR;
      }
      dataPos += MAX_PAYLOAD-3;
      dataLeft -= (MAX_PAYLOAD-3);

      // Clear extra bits
      buffer[2] = nalType;
    }
    // Signal end and send the rest of the data
    buffer[2] |= 1 << 6;
    memcpy(&buffer[3], &data[dataPos], dataLeft);
    if (fRTPInternalSendRTP(conn, buffer, dataLeft + 3, 1) == FRTP_ERROR) {
      return FRTP_ERROR;
    }
  }

  return FRTP_OK;  
}