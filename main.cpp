#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <thread>

#include "sdl_handler.h"
#include "udp_server.hpp"

int
main(int argc, char* argv[])
{
  auto p_udpServer = std::make_shared<UdpServer>();
  p_udpServer->run();

  inputStart(std::move(p_udpServer));

  std::cout << "Bye" << std::endl;

  return 0;
}