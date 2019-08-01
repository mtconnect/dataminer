#include <iostream>
#include <fstream>

#include "settings.h"


Settings::Settings()
{
    m_dirty = false;
}

void Settings::set(string &key, string sequence)
{
    map<string, string>::iterator i = m_collection.find(key);

    if (i != m_collection.end())
        i->second = sequence;
    else
        m_collection.insert(pair<string, string>(key, sequence));

    m_dirty = true;
}

string Settings::get(string &key)
{
    std::map<string, string>::iterator i = m_collection.find(key);

    if (i != m_collection.end())
        return i->second;

    return "";
}

bool Settings::check(string &key, string sequence)
{
    map<string, string>::iterator i = m_collection.find(key);

    if (i == m_collection.end())
    {
        m_collection.insert(pair<string, string>(key, sequence));
        return true;
    }
    else if (sequence.compare(i->second))
    {
        i->second = sequence;
        return true;
    }
    else
        return false;
}

void Settings::dump()
{
    for (map<string, string>::iterator p = m_collection.begin(); p != m_collection.end(); p++)
        std::cout << p->first << ": " << p->second << std::endl;
}

void Settings::restore(string filename)
{
    std::ifstream os(filename);
    string input;
    while (std::getline(os, input))
    {
        unsigned long pos = input.find_first_of("|");
        string key = input.substr(0, pos);
        string value = input.substr(pos + 1);

        std::cout << "Restoring - " << key << " = " << value << std::endl;
        m_collection.insert(pair<string, string>(key, value));

    }
    os.close();
    m_dirty = false;
}

void Settings::save(string filename, bool checkDirty)
{
    if (checkDirty && m_dirty == false)
        return;

    std::ofstream os(filename);
    for (map<string, string>::iterator p = m_collection.begin(); p != m_collection.end(); p++)
    {
        std::cout << "Updating - " << p->first << " = " << p->second << std::endl;
        os << p->first << "|" << p->second << std::endl;
    }

    os.close();
    m_dirty = false;
}
