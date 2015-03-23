//
//  tcpconnector.h
//  SearchEngineServer
//
//  Created by Xu ZHANG on 3/22/15.
//  Copyright (c) 2015 Xu ZHANG. All rights reserved.
//

#ifndef __tcpconnector_h__
#define __tcpconnector_h__

#include <netinet/in.h>
#include "tcpstream.h"

class TCPConnector
{
public:
    TCPStream* connect(const char* server, int port);
    TCPStream* connect(const char* server, int port, int timeout);
    
private:
    int resolveHostName(const char* host, struct in_addr* addr);
};

#endif