#ifndef XML2JSON_H
#define XML2JSON_H

#include <boost/property_tree/ptree.hpp>

#include <string>
#include <iostream>
#include <set>

using namespace std;
using boost::property_tree::ptree;

class xml2json
{
private:
    ptree m_ptree;
    string m_xml;
    set<string> m_numericFields;

    bool m_pretty;
    char m_EOL;

public:
    xml2json();

private:
    void setPretty(bool isPretty);
    string indent(int level);
    bool isNumeric(const string& string);
    string validateData(string key, string data);
    void printTree (ptree &pt, int level, std::ostream & ret);
    void fixup_json(ptree &pt, int depth);
    string getJSON_data(ptree &tree, string path);

public:
    void setNumericFields(set<string> numericFields);
    bool process(string xmlText);
    bool outputJSON(string outFilename);
    string getJSON(bool pretty);
    string getJSON_data(string path) { return getJSON_data(m_ptree, path); }
};

#endif // XML2JSON_H
