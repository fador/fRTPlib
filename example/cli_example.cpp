#include <iostream>
#include <thread>
#include <vector>


#include <frtp.h>

int main(int32_t argc, char* argv[])
{
  fRTPState *state = fRTPInit();


  uint32_t connID = fRTPCreateOutConn(state, "127.0.0.1", 8888, 8899);
  if (connID == FRTP_ERROR) {
    std::cerr << "Failed to create outbound connection" << std::endl;
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}