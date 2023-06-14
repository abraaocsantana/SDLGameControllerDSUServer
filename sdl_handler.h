#pragma once

#define VERBOSE_SENSORS
#define VERBOSE_AXES

class UdpServer;

void
inputStart(std::shared_ptr<UdpServer> p_updServer);