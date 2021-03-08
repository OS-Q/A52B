//
#ifndef __ESPNBNS_h__
#define __ESPNBNS_h__

#if defined BRIKI_MBC_WB_SAMD
#error "Sorry, NetBIOS library doesn't work with samd21 processor"
#endif 

#include <WiFi.h>
#include "AsyncUDP.h"

class NetBIOS
{
protected:
    AsyncUDP _udp;
    String _name;
    void _onPacket(AsyncUDPPacket& packet);

public:
    NetBIOS();
    ~NetBIOS();
    bool begin(const char *name);
    void end();
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_NETBIOS)
extern NetBIOS NBNS;
#endif

#endif
