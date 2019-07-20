#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <string>

#include "httpreader.h"
#include "xml2json.h"

namespace fs = boost::filesystem;

static int poll_count = 0;
static string last_sequence = "";

void poll(HttpReader &reader, string &outputLocation)
{
    poll_count++;

    string xmlData = reader.read();

    if (xmlData.length() == 0)
        return;

    // convert xml to json
    xml2json x;

    x.setNumericFields({"sequence", "@@data"});

    try {
        x.process(xmlData);
    }
    catch (exception & e)
    {
        cerr << e.what() << endl;
        return;
    }

#if DEBUG
    string json = x.getJSON(true);

    cout << json << endl;
    std::cout << "========== { " << poll_count << " } ==========" << std::endl;

#else
    // don't output if data has not changed
    string sequence = x.getJSON_data("MTConnectStreams.Header.<xmlattr>.lastSequence");
    if (sequence.compare(last_sequence) == 0)
    {
        std::cout << "========== { " << poll_count << ": [SKIPPED] last sequence=" << last_sequence << " } ==========" << std::endl;
        return;
    }
    last_sequence = sequence;

    time_t now = time(0);
    char timestamp[20] = "";
    strftime (timestamp, 20, "%Y%m%d-%H%M%S", localtime(&now));

    char outFilename[80];
    sprintf(outFilename, "MTComponentStreams-%s.log", timestamp);

    fs::path dir (outputLocation);
    fs::path file (outFilename);
    fs::path fullFilename = dir / file;

    x.outputJSON(fullFilename.string());

    std::cout << "========== { " << poll_count << ": " << outFilename << " } ==========" << std::endl;
#endif
}


int main(int argc, char** argv)
{
    // Check command line arguments.
    if (argc != 3)
    {
        std::cerr <<
            "Usage: dataminer <output location> <uri>\n" <<
            "Example:\n" <<
            "    dataminer /tmp https://smstestbed.nist.gov/vds/current\n";
        return -1;
    }
    string outputLocation = argv[1];
    string uri = argv[2];

    if (!boost::filesystem::exists(outputLocation))
    {
        std::cerr <<
            "Location " << outputLocation << " does not exist!\n";
        return -1;
    }

    HttpReader reader(uri);

    if (!reader.connect())
    {
        std::cerr <<
            "Cannot connect to MT agent from [" << uri << "]!\n";
        return -1;
    }

    while (true)
    {
        poll(reader, outputLocation);
        boost::this_thread::sleep_for( boost::chrono::seconds(60) );
    }

    reader.close();

    cout << "DONE" << std::endl;
    return 0;
}
