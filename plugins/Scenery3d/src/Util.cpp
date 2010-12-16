#include "Util.hpp"
#include <fstream>
#include <sstream>

using namespace std;

void trim_right(string& source, const string& t)
{
    size_t found;
    found = source.find_last_not_of(t);
    if (found != string::npos)
    {
        source.erase(found + 1);
    }
    else
    {
        source.clear();
    }
}

void trim_right(string& source)
{
    string trim_chars(" \t\f\v\n\r");
    trim_right(source, trim_chars);
}

vector<string> splitStr(const string& line, char delim)
{
    int cutAt;
    string str(line);
    vector<string> results;
    while ((cutAt = str.find_first_of(delim)) != str.npos)
    {
        if (cutAt > 0)
        {
            results.push_back(str.substr(0, cutAt));
        }
        str = str.substr(cutAt + 1);
    }
    if (str.length() > 0)
    {
        results.push_back(str);
    }
    return results;
}

float parseFloat(const string& str)
{
    stringstream ss(str);
    float f = 0;
    ss >> f;
    if (ss.fail()) {
        throw (str + " is not a number");
    }
    return f;
}

unsigned int parseInt(const string& str)
{
    stringstream ss(str);
    unsigned int i = 0;
    ss >> i;
    if (ss.fail()) {
        throw (str + " is not a number");
    }
    return i;
}

