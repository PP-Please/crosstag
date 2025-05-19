#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <sqlite3.h>
#include <random>

#ifndef WINFUNCTIONS_H
#define WINFUNCTIONS_H

using namespace std;

void createJsonTagsFile();
void saveTagsToMap(unordered_map<int, vector<string>>& tags, filesystem::directory_entry entry);
void saveTagsToDB(sqlite3& db, filesystem::directory_entry entry);
void getAndSaveDriveData(sqlite3& db);
void createFilesSQLTable(int fileId, filesystem::path filePath, sqlite3& db, DWORD volumeId);

#endif