#pragma once

uint32_t fRTPInternalPushHEVCFrame(fRTPConnection* conn, uint8_t* data, uint32_t datalen, uint32_t timestamp);

uint32_t fRTPInternalSendHEVCNal(fRTPConnection* conn, uint8_t* data, uint32_t datalen, uint32_t timestamp);
