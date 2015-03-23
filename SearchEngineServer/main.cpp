//
//  main.cpp
//  SearchEngineServer
//
//  Created by Xu ZHANG on 3/22/15.
//  Copyright (c) 2015 Xu ZHANG. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "thread.h"
#include "wqueue.h"
#include "tcpacceptor.h"

class WorkItem
{
    TCPStream* m_stream;
    
public:
    WorkItem(TCPStream* stream) : m_stream(stream) {}
    ~WorkItem() { delete m_stream; }
    
    TCPStream* getStream() { return m_stream; }
};

class ConnectionHandler : public Thread
{
    wqueue<WorkItem*>& m_queue;
    
public:
    ConnectionHandler(wqueue<WorkItem*>& queue) : m_queue(queue) {}
    
    void* run() {
        
        // 从任务队列中移除一个任务来执行。
        for (int i = 0;; i++) {
            printf("thread %lu, loop %d - waiting for item...\n",
                   (long unsigned int)self(), i);
            WorkItem* item = m_queue.remove();
            printf("thread %lu, loop %d - got one item\n",
                   (long unsigned int)self(), i);
            TCPStream* stream = item->getStream();
            
            // 处理接收到的信息。
            char input[25600]; // 视你要传输的数据的大小，改一下缓存。
            stream->receive(input, sizeof(input)-1);
            
            // 在此处对input进行预处理，先判断是UPDATE还是QUERY，再根据后面的DATA进行操作。
            bool isUpdate = true;
            if (isUpdate) {
                // 对数据库进行更新操作。
                // Do something here.
                // 将数据库是否更新成功的信息返回给客户端。
                char err[] = "SUCCEED(or FAILED)";
                stream->send(err, sizeof(err));
            } else {
                // 对数据库进行查询操作。
                // Do something here.
                // 将查询结果返回。
                char result[] = "RESULT";
                stream->send(result, sizeof(result));
            }
            
            delete item;
        }
        
        return NULL;
    }
};

int main(int argc, char** argv)
{
    int workers = 5; // 开五个线程处理客户端请求。
    string ip = "localhost"; // IP地址。
    int port = 9999; // 端口号。
    
    // 创建任务队列。
    wqueue<WorkItem*>  queue;
    for (int i = 0; i < workers; i++) {
        ConnectionHandler* handler = new ConnectionHandler(queue);
        if (!handler) {
            printf("Could not create ConnectionHandler %d\n", i);
            exit(1);
        }
        handler->start();
    }
    
    // 开始监听。
    WorkItem* item;
    TCPAcceptor* connectionAcceptor;
    if (ip.length() > 0) {
        connectionAcceptor = new TCPAcceptor(port, (char*)ip.c_str());
    }
    else {
        connectionAcceptor = new TCPAcceptor(port);
    }
    if (!connectionAcceptor || connectionAcceptor->start() != 0) {
        printf("Could not create an connection acceptor\n");
        exit(1);
    }
    
    // 如果创建了连接，则将连接加入到任务队列。
    while (1) {
        TCPStream* connection = connectionAcceptor->accept();
        if (!connection) {
            printf("Could not accept a connection\n");
            continue;
        }
        item = new WorkItem(connection);
        if (!item) {
            printf("Could not create work item a connection\n");
            continue;
        }
        queue.add(item);
    }
    
    exit(0);
}