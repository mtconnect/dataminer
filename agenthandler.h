#ifndef AGENTHANDLER_H
#define AGENTHANDLER_H

#include <boost/property_tree/ptree.hpp>

#include <string>
#include <iostream>
#include <set>

#include "settings.h"

using namespace std;
using boost::property_tree::ptree;

class agentHandler
{
private:
    ptree m_ptree;
    string m_xml;
    set<string> m_numericFields;

    bool m_pretty;
    string m_EOL;

public:
    agentHandler();

private:
    void setPretty(bool isPretty);
    string indent(int level);
    bool isNumeric(const string& string);
    string validateData(string itemId, string key, string data);
    void lookupNumericFields(ptree ptree);
    int printTree (Settings *manager, const string &deviceId, const string &deviceUUID, const string &componentId, ptree &pt, int level, string ret, ostream &out);
    void fixup_json(ptree &pt, int depth);
    string getJSON_data(ptree &tree, string path);

public:
    void setProbeInfo(string probeXml);
    set<string> &getNumericFields() { return m_numericFields; }
    bool process(string xmlText);
    bool outputJSON(Settings *manager, string outFilename);
    string getJSON_data(string path) { return getJSON_data(m_ptree, path); }
};

#endif // AGENTHANDLER_H
