#include <stdlib.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>

#include "httpreader.h"
#include "agenthandler.h"
#include "worker.h"

Worker::Worker()
{
    m_interval = 60;
    m_poll_count = 0;
}

bool Worker::setup(Settings *settings, string outputLocation, string uri, string interval)
{
    m_outputLocation = outputLocation;
    m_uri = uri;
    m_settings = settings;

    m_interval = atoi(interval.c_str());
    if (m_interval <= 0) {
        std::cerr << "interval [" << interval << "] is invalid" << endl;
        return false;
    }

    m_next_sequence = settings->get(uri);
    std::cout << "----------------" << endl;
    std::cout << "Output Location: " << outputLocation << endl;
    std::cout << "Agent Uri:       " << uri << endl;
    std::cout << "Poll Interval:   " << interval << endl;
    std::cout << "Next Sequence #: " << m_next_sequence << endl;

    if (!boost::filesystem::exists(outputLocation))
    {
        std::cerr <<
            "Location " << outputLocation << " does not exist!\n";
        return false;
    }

    return m_reader.parseUri(uri);
}

void Worker::run()
{
    while (true)
    {
        poll();
        boost::this_thread::sleep_for( boost::chrono::seconds(m_interval) );
    }
}

void Worker::poll()
{
    m_poll_count++;

    // get the meta info if not ready
    set<string> &numericFields = m_handler.getNumericFields();
    if (numericFields.size() == 0)
    {
        m_reader.setQuery("/probe");
        string probeXml = m_reader.read();
        if (probeXml.length() == 0)
        {
            std::cerr << "No data!" << endl;
            return;
        }

        m_handler.setProbeInfo(probeXml);
        m_reader.close();
    }

    if (m_next_sequence.length() == 0)
        m_reader.setQuery("/current");
    else
        m_reader.setQuery("/sample?count=10000&from="+m_next_sequence);

    string xmlData = m_reader.read();

    if (xmlData.length() == 0)
    {
        std::cerr << "No data!" << endl;
        return;
    }

    try {
        m_handler.process(xmlData);
    }
    catch (exception & e)
    {
        std::cerr << e.what() << endl;
        return;
    }

    // don't output if data has not changed
    string sequence = m_handler.getJSON_data("MTConnectStreams.Header.<xmlattr>.nextSequence");
    if (sequence.compare(m_next_sequence) == 0)
    {
        std::cout << "========== { " << m_uri << " - "<< m_poll_count << ", [SKIPPED] next sequence = " << m_next_sequence << " } ==========" << std::endl;
        return;
    }

    m_next_sequence = sequence;
    m_settings->set(m_uri, m_next_sequence);

    if (sequence.length() == 0)
    {
        // last next_sequence may be invalid, reset to using "current" to fetch the latest data
        std::cout << "========== { " << m_uri << " - "<< m_poll_count << ", [SKIPPED] reset to fetch current data } ==========" << std::endl;
        return;
    }

    m_handler.outputJSON(&m_itemManager, m_outputLocation);
    std::cout << "========== { " << m_uri << " - " << m_poll_count << ", next sequence = " << m_next_sequence << " } ==========" << std::endl;
}

