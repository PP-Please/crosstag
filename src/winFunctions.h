#include <unordered_map>
#include <vector>
#include <string>

#ifndef WINFUNCTIONS_H
#define WINFUNCTIONS_H

using namespace std;

void createJsonTagsFile();
void saveTagsToMap(unordered_map<int, vector<string>>& tags, filesystem::directory_entry entry);

#endif