//
//  thread.h
//  SearchEngineServer
//
//  Created by Xu ZHANG on 3/22/15.
//  Copyright (c) 2015 Xu ZHANG. All rights reserved.
//

#ifndef __thread_h__
#define __thread_h__

#include <pthread.h>

class Thread
{
public:
    Thread();
    virtual ~Thread();
    
    int start();
    int join();
    int detach();
    pthread_t self();
    
    virtual void* run() = 0;
    
private:
    pthread_t  m_tid;
    int        m_running;
    int        m_detached;
};

#endif