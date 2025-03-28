#include <iostream>
#include <filesystem>
#include <shobjidl.h> // SHGetPropertyStoreFromParsingName()
#include <fstream>
#include <unordered_set>
#include <vector>
#include <sqlite3.h>
#include <string>

#include "tagFunctions.h"
#include <random>

// #include <nlohmann/json.hpp>
// using json = nlohmann::json;

using namespace std;

int main() {
    filesystem::path current_path = filesystem::current_path();

    /*
        Load the hidden file containing tags for each file within the directory, or create it if
        it does not exist. 
    */
    // createJsonTagsFile();
    sqlite3 *db;
    int rc = 0;
    rc = sqlite3_open("dbTags.db", &db);
    if (rc) {
        cout << "Error: database could not be opened or created" << endl;
        return(1);
    }

    const char* sqlmsg = R"(
    CREATE TABLE IF NOT EXISTS drives (
        drive_uuid STRING PRIMARY KEY,
        drive_id_win INTEGER UNIQUE,
        drive_id_mac TEXT UNIQUE,
        drive_name TEXT NOT NULL,
    );

    CREATE TABLE IF NOT EXISTS files (
        file_uuid INTEGER PRIMARY KEY,
        file_id_win INTEGER UNIQUE,
        file_id_mac INTEGER UNIQUE,
        drive_id_win dword [unique]
        drive_id_mac string [unique]
        file_path TEXT NOT NULL,
        FOREIGN KEY (drive_id) REFERENCES drives(drive_uuid)
    );

    CREATE TABLE IF NOT EXISTS tags (
        file_id INTEGER PRIMARY KEY,
        tag_name TEXT NOT NULL,
        FOREIGN KEY (file_id) REFERENCES files(file_id)
    );
    )";

    rc = sqlite3_exec(db, sqlmsg, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        cout << "Error creating the tables for the database." << endl;
        return(1);
    }

    getAndSaveDriveData(*db);

    try {
        for (auto const& entry: filesystem::directory_iterator{current_path}) {
            if (entry.path().filename() == "storedTags.json" || entry.is_directory() || entry.path().filename() == "dbTags.db") continue;

            saveTagsToDB(*db, entry);
        } 
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error: " << e.what() << endl;
    }
    
    sqlite3_close(db);
    return 0;
}
/*
    g++ -o main main.cpp -lole32 -loleaut32 -lpropsys

    cmake -G "Unix Makefiles" ..
    make    
*/