#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>

#include "worker.h"

using namespace boost;

static Settings settings;
static string settingsName;

static Worker *createWorker(string outputLocation, string uri, string poll)
{
    Worker *worker = new Worker();

    if (!worker->setup(&settings, outputLocation, uri, poll))
        return nullptr;

    return worker;
}

static string getSettingsName(string progName)
{
    const char* homeDrive = getenv("HOMEDRIVE");
    const char* homePath = getenv("HOMEPATH");
    string homeDir = "";

    if (homeDrive != nullptr && homePath != nullptr)
    {
        // Windows
        homeDir = homeDrive;
        homeDir += homePath;
    }
    else {
        // unix
        const char *home = getenv("HOME");
        if (home != nullptr)
            homeDir = home;

        // homeDir is "" (current directory) if no environment
    }


    boost::filesystem::path dir (homeDir);
    boost::filesystem::path file (progName + ".settings");
    boost::filesystem::path fullFilename = dir / file;

    return fullFilename.string();
}

int main(int argc, char** argv)
{
    // Check command line arguments.
    if (argc < 2 || argc > 4)
    {
        std::cerr <<
            "Usage: " << endl <<
            "    dataminer <output location> <uri> [poll cycle in seconds - defaults is 60]" << endl <<
            "Example:" << endl <<
            "    dataminer /tmp https://smstestbed.nist.gov/vds" << endl <<
            "or" << endl <<
            "    dataminer <config file>" << endl;
        return -1;
    }

    settingsName = getSettingsName("dataminer");
    settings.restore(settingsName);

    std::vector<Worker*> worker_pool;

    boost::thread *bthread = nullptr;
    if (argc != 2)
    {
        string outputLocation = argv[1];
        string uri = argv[2];
        string poll = argc > 3 ? argv[3] : "60";

        Worker *worker = createWorker(outputLocation, uri, poll);

        if (worker == nullptr)
            return -1;

        worker_pool.push_back(worker);

        bthread = new boost::thread(boost::bind(&Worker::run, worker));

    }
    else {

        ifstream in(argv[1]);
        if (!in.is_open()) {
            std::cerr << "Cannot open config file " << argv[1] << "!" << endl;
            return -1;
        }

        typedef tokenizer< escaped_list_separator<char> > Tokenizer;
        vector< string > vec;
        string line;

        while (getline(in, line))
        {
            Tokenizer tok(line);
            vec.assign(tok.begin(), tok.end());

            unsigned long argc = vec.size();

            // empty line
            if (argc == 0)
                continue;

            if (argc != 2 && argc != 3)
            {
                std::cerr << "Invalid input [" << line << "]" << endl;
                return -1;
            }

            string outputLocation = vec[0];

            if (outputLocation[0] == '#')
                continue;

            string uri = vec[1];
            string poll = argc > 2 ? vec[2] : "60";

            Worker *worker = createWorker(outputLocation, uri, poll);

            if (worker == nullptr)
                continue;

            worker_pool.push_back(worker);

            bthread = new boost::thread(boost::bind(&Worker::run, worker));

            if (bthread == nullptr)
                return -1;
        }

    }

    if (bthread == nullptr)
        return -1;

    while (true)
    {
        settings.save(settingsName);
        boost::this_thread::sleep_for( boost::chrono::seconds(5) );
    }

    return 0;
}

