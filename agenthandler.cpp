#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <math.h>
#include <map>

#include "agenthandler.h"
#include "settings.h"

namespace fs = boost::filesystem;

agentHandler::agentHandler()
{
    m_pretty = false;
    m_EOL = " ";
}

void agentHandler::setPretty(bool a)
{
    m_pretty = a;
    m_EOL = m_pretty ? "\n" : " ";
}

string agentHandler::indent(int level)
{
    if (!m_pretty)
        return "";

    string s;
    for (int i=0; i<level; i++) s += "  ";
    return s;
}

bool agentHandler::isNumeric(const std::string& string)
{
    std::size_t pos;
    double value = 0.0L;

    try
    {
        value = std::stod(string, &pos);
    }
    catch(std::invalid_argument&)
    {
        return false;
    }
    catch(std::out_of_range&)
    {
        return false;
    }

    return pos == string.size() && !std::isnan(value);
}

string agentHandler::validateData(string itemId, string key, string data)
{
    if (key.compare("@@data") == 0 && m_numericFields.count(itemId) > 0)
    {
        if (isNumeric(data))
            return data;

        string sl = data;
        std::transform(sl.begin(), sl.end(), sl.begin(), ::tolower);
         if (sl.compare("unavailable") == 0)
            return "null";
    }

    boost::replace_all(data, "\\", "\\\\");
    return "\"" + data + "\"";
}

int agentHandler::printTree (Settings *manager, const string &deviceName, const string &deviceUUID, const string &componentId, ptree &pt, int level, string ret, std::ostream &out)
{
    int rec_count = 0;

    if (level)
        ret += m_EOL;

    ret += indent(level) + "{" + m_EOL;

    string last = ret;

    // output first any unique subtrees
    for (ptree::iterator pos = pt.begin(); pos != pt.end();)
    {
        ret = last;
        string key = pos->first;

        string ind = indent(level+1);

        if (pos->first.compare("<xmlattr>") == 0)
        {
            string sequence;
            string timestamp;
            string dataItemId;

            int count = 0;
            for (ptree::iterator pos2 = pos->second.begin(); pos2 != pos->second.end();)
            {
                string key = pos2->first;
                string value = pos2->second.data();

                ++pos2;
                if (key.compare("sequence") == 0)
                    sequence = value;
                else if (key.compare("timestamp") == 0)
                    timestamp = value;
                else if (key.compare("dataItemId") == 0)
                    dataItemId = value;
                else
                {
                    count++;
                    if (count > 1)
                        ret += "," + m_EOL;

                    string output = "\"" + key + "\": " + validateData(dataItemId, key, value);
                    ret += indent(level+1) + output;
                }
            }

            if (dataItemId.length() > 0 && sequence.length() > 0)
            {
                string key = deviceUUID + "|" + componentId + "|" + dataItemId;

                // output only if the sequence number changes
                if (manager == nullptr || manager->check(key, sequence))
                {
                    stringstream ss;

                    ss << "{";
                    ss << "\"timestamp\": " << "\"" << timestamp << "\",";
                    ss << "\"sequence\": " << sequence << ",";
                    ss << "\"deviceName\": " << "\"" << deviceName << "\",";
                    ss << "\"deviceUUID\": " << "\"" << deviceUUID << "\",";
                    ss << "\"componentId\": " << "\"" << componentId << "\",";
                    ss << "\"dataItemId\": " << "\"" << dataItemId << "\",";
                    ss << ret;
                    for (int i=level; i>=0 ; i--)
                        ss << indent(i+1) + " }";
                    ss << " }" << endl;

#ifdef DEBUG
                    cout << ss.str();
#endif
                    out << ss.str();

                    rec_count++;
                }
            }

        }
        else {
            string key = pos->first;

            ret += indent(level+1) + "\"" + key + "\": ";
            if (pos->second.empty())
                ret += validateData("", key, pos->second.data());
            else
                rec_count += printTree(manager, deviceName, deviceUUID, componentId, pos->second, level + 1, ret, out);
        }

        ++pos;
        if (pos != pt.end())
            ret += ",";
        ret += m_EOL;
    }

    ret += indent(level) + "}";

    return rec_count;
}


void agentHandler::fixup_json(ptree &pt, int depth)
{
    string data = pt.data();

    // Root ptree cannot have data
    if (depth == 0 && data.length() > 0)
        return;

    // Ptree cannot have both children and data

    // remove all spaces (include /r, /n etc) to test if it is a null string
    data.erase(std::remove_if(data.begin(), data.end(), ::isspace), data.end());

    if (data.length() == 0)
        pt.put_value("");

    data = pt.data();
    ptree::iterator it = pt.begin();
    if (data.length() > 0 && !pt.empty())
    {
        if (it->first.compare("<xmlattr>") == 0)
        {
            // put the value of this node as xml attrbutes under @@data
           it->second.push_back(ptree::value_type("@@data", data));
           pt.put_value("");
        }
    }

    // Check children
    for (; it != pt.end(); ++it) {
        ptree &ctree = it->second;

        fixup_json(ctree, depth + 1);
    }
}


void agentHandler::lookupNumericFields(ptree probeInfo)
{
    for (ptree::iterator p = probeInfo.begin(); p != probeInfo.end(); p++)
    {
        if (p->first.compare("<xmlattr>") == 0)
        {
            string dataItemId;
            string category = "";
            bool isNumeric = false;

            for (ptree::iterator pos2 = p->second.begin(); pos2 != p->second.end();)
            {
                string key = pos2->first;
                string value = pos2->second.data();

                transform(key.begin(), key.end(), key.begin(), ::tolower);

                ++pos2;
                if (key.compare("id") == 0)
                    dataItemId = value;
                else if (key.compare("units") == 0)
                    isNumeric = true;
                else if (key.compare("category") == 0)
                {
                    transform(value.begin(), value.end(), value.begin(), ::tolower);
                    category = value;
                    if (value.compare("sample") == 0)
                        isNumeric = true;
                }
            }

            if (isNumeric)
                m_numericFields.insert(dataItemId);
        }
        else
            lookupNumericFields(p->second);
    }
}

void agentHandler::setProbeInfo(string probeXml)
{
    ptree probeInfo;
    try {
         // Read the stringstream into a Boost property tree, m_ptree
        istringstream iss;
        iss.str (probeXml);

        boost::property_tree::read_xml( iss, probeInfo );
    }
    catch (exception & e)
    {
        std::cerr << e.what() << endl;
        return;
    }

    lookupNumericFields(probeInfo);

#ifdef DEBUG
    for (set<string>::iterator p = m_numericFields.begin(); p != m_numericFields.end(); p++)
        std::cout << *p << endl;
#endif
}

bool agentHandler::process(string xmlText)
{
    m_xml = xmlText;

    try {
         // Read the stringstream into a Boost property tree, m_ptree
        istringstream iss;
        iss.str (m_xml);

        boost::property_tree::read_xml( iss, m_ptree );

        fixup_json(m_ptree, 0);
    }
    catch (exception & e)
    {
        std::cerr << e.what() << endl;
        return false;
    }

    return true;
}


bool agentHandler::outputJSON(Settings *itemManager, string outputLocation)
{
    time_t now = time(nullptr);
    char timestamp[20] = "";
    strftime (timestamp, 20, "%Y%m%d-%H%M%S", localtime(&now));

    bool ret = true;

    setPretty(false);
    try {

        // each line is one component stream
        ptree& devices = m_ptree.get_child("MTConnectStreams.Streams");
        for (ptree::iterator pos = devices.begin(); pos != devices.end(); pos++)
        {
            ptree& device = pos->second;

            string deviceName = getJSON_data(device, "<xmlattr>.name");
            string deviceUUID = getJSON_data(device, "<xmlattr>.uuid");

            ofstream outfile;

            string outFilename;
            outFilename =  "MTConnect-" + deviceUUID + "-" + deviceName + "-" + timestamp + ".log";

            fs::path dir (outputLocation);
            fs::path file (outFilename);
            fs::path fullFilename = dir / file;

            outfile.open (fullFilename.string());
            int rec_count = 0;

            for (ptree::iterator p = device.begin(); p != device.end(); p++)
            {
                string key = p->first;
                if (key.compare("ComponentStream") == 0)
                {
                    ptree &stream = p->second;

                    string componentId = getJSON_data(stream, "<xmlattr>.componentId");

                    for (ptree::iterator s = stream.begin(); s != stream.end(); s++)
                    {
                        if (s->first.compare("<xmlattr>"))
                        {
                            string ss;
                            ss = "\"" + s->first + "\": ";
                            rec_count += printTree(itemManager, deviceName, deviceUUID, componentId, s->second, 0, ss, outfile);
                        }
                    }
                }
            }

            std::cout << "output: [ " << outFilename << " - records: " << rec_count << " ]" << std::endl;
            outfile.close();
        }

    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        cerr << m_xml << endl;
        ret = false;
    }

    return ret;
}


string agentHandler::getJSON_data(ptree & tree, string path)
{
    try {
        ptree& node = tree.get_child(path);
        return node.data();

    } catch (...) {
        return "";
    }
}
