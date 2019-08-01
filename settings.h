#ifndef ITEMMANAGER_H
#define ITEMMANAGER_H

#include <string>
#include <map>

using namespace std;

class Settings
{
private:
    map< string, string> m_collection;
    bool m_dirty;

public:
    void restore(string filename);
    void save(string filename, bool checkDirty = true);

    void set(string &key, string sequence);
    string get(string &key);

    bool check(string &key, string sequence);

    void dump();

    Settings();
};

#endif // ITEMMANAGER_H
