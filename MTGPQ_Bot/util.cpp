#include "util.hpp"
#include <sstream>
#include <Windows.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Util::SplitString Split a string into substrings delimited by a character.
/// \param [in] s String to be split.
/// \param [in] delim Delimiter character.
/// \return A vector of resulting substrings.
////////////////////////////////////////////////////////////////////////////////////////////////////
vector<string> Util::SplitString(string s, char delim) {
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) elems.push_back(item);
    return elems;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Util::GetFileNamesFromDir Return the name of all the files (and dirs) inside a directory that match the file_glob pattern.
/// \param [in] dir Directory where to search the file names.
/// \param [in] file_glob Glob pattern of the name of the files to search.
/// \param [in] files_only (Optional) true = return only files (default), false = also return directories.
/// \return The names of the files that matched.
////////////////////////////////////////////////////////////////////////////////////////////////////
vector<string> Util::GetFileNamesFromDir(string dir, string file_glob, bool files_only) {
    vector<string> names;
    string search_path = dir + "/" + file_glob;
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || !files_only)
                names.push_back(fd.cFileName);
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
    return names;
}
