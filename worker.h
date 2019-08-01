#ifndef WORKER_H
#define WORKER_H

#include <string>

#include "httpreader.h"
#include "settings.h"
#include "agenthandler.h"

class Worker
{
private:
    Settings *m_settings;
    Settings m_itemManager;
    HttpReader  m_reader;
    agentHandler    m_handler;

    string      m_outputLocation;
    string      m_uri;
    int         m_interval;
    int         m_poll_count;
    string      m_next_sequence;

public:
    Worker();

    bool setup(Settings *settings, string outputLocation, string uri, string interval);
    void run();
    void poll();
};

#endif // WORKER_H
