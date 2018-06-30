#include <iostream>
#include <thread>
#include <vector>

#include <frtp.h>

#include <kvazaar.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(int32_t argc, char* argv[])
{
  fRTPState *state = fRTPInit();

  uint32_t width = 1280;
  uint32_t height = 720;

  uint16_t outPort = 8888;
  uint16_t inPort = 8899;

  FILE* inputFile = stdin;

  if (argc > 1) {
    inputFile = fopen(argv[1], "rb");
    if (!inputFile) {
      std::cerr << "Unable to open file " << argv[1] << std::endl;
      return EXIT_FAILURE;
    }
  }
  if (argc > 2) {
    outPort = atoi(argv[2]);
  }
  if (argc > 3) {
    inPort = atoi(argv[3]);
  }

#ifdef _WIN32
  if (inputFile == stdin) {
    _setmode(_fileno(stdin), _O_BINARY);
  }
#endif

  uint32_t connID = fRTPCreateConn(state, "127.0.0.1", outPort, inPort);
  if (connID == FRTP_ERROR) {
    std::cerr << "Failed to create outbound connection" << std::endl;
    return EXIT_FAILURE;
  }

  kvz_encoder* enc = NULL;
  const kvz_api * const api = kvz_api_get(8);
  kvz_config* config = api->config_alloc();
  api->config_init(config);
  api->config_parse(config, "preset", "ultrafast");
  config->width = width;
  config->height = height;
  config->hash = kvz_hash::KVZ_HASH_NONE;
  config->intra_period = 32;
  config->vps_period = 1;
  config->qp = 32;

  enc = api->encoder_open(config);
  if (!enc) {
    fprintf(stderr, "Failed to open encoder.\n");
    return EXIT_FAILURE;
  }

  kvz_picture *img_in[16];
  for (uint32_t i = 0; i < 16; i++) {
    img_in[i] = api->picture_alloc_csp(KVZ_CSP_420, width, height);
  }

  bool done = false;
  uint8_t inputCounter = 0;
  uint8_t outputCounter = 0;
  uint32_t frame = 0;

  while (!done) {
    kvz_data_chunk* chunks_out = NULL;
    kvz_picture *img_rec = NULL;
    kvz_picture *img_src = NULL;
    uint32_t len_out = 0;
    kvz_frame_info info_out;


    if (!fread(img_in[inputCounter]->y, width*height, 1, inputFile)) {
      done = true;
      continue;
    }
    if (!fread(img_in[inputCounter]->u, width*height>>2, 1, inputFile)) {
      done = true;
      continue;
    }
    if (!fread(img_in[inputCounter]->v, width*height>>2, 1, inputFile)) {
      done = true;
      continue;
    }
    
    if (!api->encoder_encode(enc,
      img_in[inputCounter],
      &chunks_out,
      &len_out,
      &img_rec,
      &img_src,
      &info_out)) {
      fprintf(stderr, "Failed to encode image.\n");
      for (uint32_t i = 0; i < 16; i++) {
        api->picture_free(img_in[i]);
      }
      return EXIT_FAILURE;
    }
    inputCounter = (inputCounter + 1) % 16;


    if (chunks_out == NULL && img_in == NULL) {
      // We are done since there is no more input and output left.
      goto cleanup;
    }

    if (chunks_out != NULL) {
      uint64_t written = 0;
      // Write data into the output file.
      for (kvz_data_chunk *chunk = chunks_out;
        chunk != NULL;
        chunk = chunk->next) {
        written += chunk->len;
      }
      uint8_t* outData = (uint8_t*)malloc(written);
      uint32_t dataPos = 0;
      for (kvz_data_chunk *chunk = chunks_out;
        chunk != NULL;
        chunk = chunk->next) {
        memcpy(&outData[dataPos], chunk->data, chunk->len);
        dataPos += chunk->len;
      }
      outputCounter = (outputCounter + 1) % 16;

      // Do the RTP sending
      frame++;
      if (fRTPPushFrame(state, connID, outData, written, fRTPFormat::FRTP_HEVC, (90000 / 30)*frame) == FRTP_ERROR) {
        std::cerr << "RTP push failure" << std::endl;
      }

      free(outData);
    }
  }

  cleanup:

  return EXIT_SUCCESS;
}