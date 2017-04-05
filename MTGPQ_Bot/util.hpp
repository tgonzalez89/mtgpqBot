#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>

using namespace std;

class Util
{
public:
    static vector<string> SplitString(string s, char delim);
    static vector<string> GetFileNamesFromDir(string folder, string file, bool files_only = true);
};

#endif // UTIL_H
