#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <math.h>
#include <map>

#include "xml2json.h"

xml2json::xml2json()
{
    m_pretty = false;
    m_EOL = ' ';
}

void xml2json::setPretty(bool a)
{
    m_pretty = a;
    m_EOL = m_pretty ? '\n' : ' ';
}

string xml2json::indent(int level)
{
    if (!m_pretty)
        return "";

    string s;
    for (int i=0; i<level; i++) s += "  ";
    return s;
}

bool xml2json::isNumeric(const std::string& string)
{
    std::size_t pos;
    long double value = 0.0;

    try
    {
        value = std::stof(string, &pos);
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

string xml2json::validateData(string key, string data)
{
    if (m_numericFields.count(key) > 0)
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

void xml2json::printTree (ptree &pt, int level, std::ostream & ret)
{
    if (level)
        ret << m_EOL;

    // map of subtrees with the same key
    std::map<string, std::vector<ptree>> m;

    // list of shared keys
    std::set<string> matches;

    // merge the subtrees witht the same key
    for (ptree::iterator pos = pt.begin(); pos != pt.end(); pos++)
    {
        string key = pos->first;
        std::map<string, std::vector<ptree>>::iterator i = m.find(key);

#if 1
        if (i != m.end())
        {
            // more than one subtree with this key
            i->second.push_back(pos->second);
            matches.insert(key);
        }
        else
#endif
        {
            std::vector<ptree> node;

            node.push_back(std::ref(pos->second));
            m.insert(std::pair<string, std::vector<ptree>>(key, node));
        }
    }

    ret << indent(level) << "{" << m_EOL;

    // output first any unique subtrees
    for (ptree::iterator pos = pt.begin(); pos != pt.end();)
    {
        string key = pos->first;


        // skip this one if the key is not unique
        if (matches.find(key) != matches.end())
        {
            ++pos;
            continue;
        }

        // remove it from the map
        std::map<string, std::vector<ptree>>::iterator ii = m.find(key);
        if (ii != m.end())
            m.erase(ii);

        string ind = indent(level+1);

        if (pos->first.compare("<xmlattr>") == 0)
        {
            for (ptree::iterator pos2 = pos->second.begin(); pos2 != pos->second.end();)
            {
                string key = pos2->first;
                string value = pos2->second.data();

                ret << indent(level+1) << "\"" << key << "\": " << validateData(key, value);

                ++pos2;
                if (pos2 != pos->second.end()) {
                    ret << "," << m_EOL;
                }
            }
        }
        else {
            string key = pos->first;

            ret << indent(level+1) << "\"" << key << "\": ";
            if (pos->second.empty())
                ret << validateData(key, pos->second.data());
            else
                printTree(pos->second, level + 1, ret);
        }

        ++pos;
        if (pos != pt.end() || m.size() != 0)
            ret << ",";
        ret << m_EOL;
    }

    // now output the substrees that share the same key
    for (std::map<string, std::vector<ptree>>::iterator i = m.begin(); i != m.end(); )
    {
        std::vector<ptree> &p = i->second;
        ret << indent(level + 1) << "\"" << i->first << "\": [";

        for (std::vector<ptree>::iterator pp = p.begin(); pp != p.end();)
        {
            printTree((*pp), level + 2, ret);
            ++pp;
            if (pp != p.end()) {
                ret << ",";
            }
        }

        ret << m_EOL << indent(level + 1) << "]" << m_EOL;

        i++;
        if (i != m.end()) {
            ret << ",";
        }

    }

    ret << indent(level) << "}";
}


void xml2json::fixup_json(ptree &pt, int depth)
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
        continue;


        // remove <xmlattr> nodes, bring all the attributes up to this level
        ptree::iterator it2 = ctree.begin();
        if (it2->first.compare("<xmlattr>") == 0)
        {
//            std::vector<std::pair<string, string>> kv;

            for (ptree::iterator pos2 = it2->second.begin(); pos2 != it2->second.end(); pos2++)
//                kv.push_back(make_pair(pos2->first, pos2->second.data()));
                  ctree.insert(it2, ptree::value_type(pos2->first, pos2->second.data()));

            it2->second.clear();

            ctree.erase("<xmlattr>");

//            for (std::vector<std::pair<string, string>>::iterator m = kv.begin(); m != kv.end(); m++)
//                ctree.add(m->first, m->second.data());
        }
    }
}

void xml2json::setNumericFields(set<string> numericFields)
{
    m_numericFields = numericFields;
}

bool xml2json::process(string xmlText)
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


bool xml2json::outputJSON(string outFilename)
{
    ofstream outfile;
    bool ret = true;

    setPretty(false);
    try {
        outfile.open (outFilename);

#if 0
        ptree& devices = m_ptree.get_child("MTConnectStreams.Streams");
        for (ptree::iterator pos = devices.begin(); pos != devices.end(); pos++)
        {
            outfile << "{\"" + pos->first + "\": ";
            printTree(pos->second, 0, outfile);
            outfile << "}\n";
        }
#endif

        ptree& devices = m_ptree.get_child("MTConnectStreams.Streams");
        for (ptree::iterator pos = devices.begin(); pos != devices.end(); pos++)
        {
            ptree& device = pos->second;

            string deviceName = getJSON_data(device, "<xmlattr>.name");
            string deviceUUID = getJSON_data(device, "<xmlattr>.uuid");

            for (ptree::iterator p = device.begin(); p != device.end(); p++)
            {
                if (p->first.compare("ComponentStream") == 0)
                {
                    outfile << "{";
                    outfile << "\"deviceName\": \"" + deviceName + "\", ";
                    outfile << "\"deviceUUID\": \"" + deviceUUID + "\", ";

                    outfile << "\"" + p->first + "\": ";
                    printTree(p->second, 0, outfile);
                    outfile << " }\n";
                }
            }
        }
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
        cerr << m_xml << endl;
        ret = false;
    }

    outfile.close();

    return ret;
}

string xml2json::getJSON(bool isPretty)
{
    std::stringstream oss;

    setPretty(isPretty);
    try {
        printTree(m_ptree, 0, oss);

    } catch (exception &e) {
        std::cerr << e.what() << endl;
        return "";
    }

    return oss.str();
}

string xml2json::getJSON_data(ptree & tree, string path)
{
    try {
        ptree& node = tree.get_child(path);
        return node.data();

    } catch (...) {
        return "";
    }

}
