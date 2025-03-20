#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

// #ifndef WINFUNCTIONS_H
// #define WINFUNCTIONS_H

using namespace std;

class tagFunctions {
    public:
    void createJsonTagsFile();
    void saveTagsToMap(unordered_map<int, vector<string>>& tags, filesystem::directory_entry entry);
};

// #endif