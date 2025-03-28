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

string generateUuidV4() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

    uint32_t part1 = dist(gen);
    uint16_t part2 = dist(gen) & 0xFFFF;
    uint16_t part3 = (dist(gen) & 0x0FFF) | 0x4000;
    uint16_t part4 = (dist(gen) & 0x3FFF) | 0x8000;
    uint32_t part5 = dist(gen);
    uint32_t part6 = dist(gen);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << part1 << "-";
    ss << std::setw(4) << part2 << "-";
    ss << std::setw(4) << part3 << "-";
    ss << std::setw(4) << part4 << "-";
    ss << std::setw(4) << (part5 >> 16) << std::setw(4) << (part5 & 0xFFFF);
    
    return ss.str();
}

#endif