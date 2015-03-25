//
//  main.cpp
//  SearchEngineServer
//
//  Created by Xu ZHANG on 3/22/15.
//  Copyright (c) 2015 Xu ZHANG. All rights reserved.
//


#define BUFFERSIZE 256

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "thread.h"
#include "wqueue.h"
#include "tcpacceptor.h"

#include <flann/flann.hpp>
#include <flann/io/hdf5.h>

using namespace flann;

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
    Index<L2<float> >& index;
    int nn;
    
    Matrix<float> *newpoints;
    
public:
    ConnectionHandler(wqueue<WorkItem*>& queue, int nn, Index<L2<float> >& index) : m_queue(queue), nn(nn), index(index) {}
    
    void* run() {
        
        // Remove a task from the task queue
        for (int i = 0;; i++) {
            printf("thread %lu, loop %d - waiting for client...\n",
                   (long unsigned int)self(), i);
            WorkItem* item = m_queue.remove();
            printf("thread %lu, loop %d - got one client\n", (long unsigned int)self(), i);
            TCPStream* stream = item->getStream();
            
            
            // handle the received message
            char input[25600]; // buffer for transfer data
            stream->receive(input, sizeof(input)-1);
            printf("thread %lu, receive: %s\n", (long unsigned int)self(), input);
            
            string s(input);
            bool isUpdate = false;
            bool isQuery = false;
            
            if (s.find_first_of("UPDATE")==0)
            {
                isUpdate = true;
                printf("thread %lu, UPDATE\n",(long unsigned int)self());
            }
            else if(s.find_first_of("QUERY")==0)
            {
                isQuery = true;
                printf("thread %lu, QUERY\n",(long unsigned int)self());
            }
            
            
            // preprocess the input, judge is UPDATE or QUERY，and then do with data
            if (isUpdate) {
                // update index with queries
                Matrix<float> newpoint;
                string filequery = s.substr(7);
                load_from_file(newpoint, filequery, "query");
                
                index.addPoints(newpoint,2); //add new points and rebuild the index.
                string fileindex = "index.idx";
                index.save(fileindex);
                
                // Determine whether the update was successful，and return the information（successed or failed） to client
                char err[] = "UPDATE SUCCEED";
                stream->send(err, sizeof(err));
            }
            else if(isQuery){
                
                //search with a  query
                Matrix<float> query;
                string filequery = s.substr(6);
                load_from_file(query, filequery, "query");
                printf("thread %lu, file: %s\n",(long unsigned int)self(), filequery.c_str());
                
                Matrix<int> indices(new int[query.rows*nn], query.rows, nn);
                Matrix<float> dists(new float[query.rows*nn], query.rows, nn);
                index.knnSearch(query, indices, dists, nn, flann::SearchParams(128));
                
                //save results
                char fileresult[] = "result.hdf5";
                flann::save_to_file(indices,fileresult,"result");
                
                //prepare filename of the results
                char resulttitle[] = "RESULT";
                int len = strlen(resulttitle)+strlen(fileresult)+BUFFERSIZE;
                char * result = (char *)malloc(len);
                snprintf(result, len, "%s %s", resulttitle, fileresult);
                
                //send the results file name
                stream->send(result, sizeof(result));
                
            }
            
            delete item;
        }
        
        return NULL;
    }
};



int main(int argc, char** argv)
{
    int workers = 2; // Five thread to handle users' request
    string ip = "localhost"; // IP
    int port = 9999; // port
    
    
    //index and data prepare
    int nn = 3; //near neighbor.
    Matrix<float> dataset;
    load_from_file(dataset, "dataset.hdf5","dataset");
    Index<L2<float> > index(dataset, flann::KDTreeIndexParams(4));
    index.buildIndex();
    
    //or we can load index by
    //flann::Index<L2<float> > index(dataset, flann::SavedIndexParams("index"));
    
    // Task queue
    wqueue<WorkItem*>  queue;
    for (int i = 0; i < workers; i++) {
        ConnectionHandler* handler = new ConnectionHandler(queue,nn,index);
        if (!handler) {
            printf("Could not create ConnectionHandler %d\n", i);
            exit(1);
        }
        handler->start();
    }
    
    // Listen
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
    
    // If new connection, add the connection into task queue.
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