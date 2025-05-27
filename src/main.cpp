#include <iostream>
#include <filesystem>
#include <shobjidl.h> // SHGetPropertyStoreFromParsingName()
#include <fstream>
#include <unordered_set>
#include <vector>
#include <sqlite3.h>
#include <string>

#include "tagFunctions.h"

// #include <nlohmann/json.hpp>
// using json = nlohmann::json;

using namespace std;

int main() {
    wstring filepath;
    cout << "Enter path: " << endl;
    getline(wcin, filepath);
    // wstring_convert<codecvt_utf8<wchar_t>> conv;
    // wstring filepath_utf16 = conv.from_bytes(filepath);

    // filesystem::path current_path(filepath_utf16);
    filesystem::path current_path(filepath);

    // filesystem::path current_path2 = filesystem::current_path();
    // cout << current_path2 << endl;
    /*
        Load the hidden file containing tags for each file within the directory, or create it if
        it does not exist. 
    */
    // createJsonTagsFile();
    sqlite3 *db;
    int rc;
    rc = sqlite3_open("dbTags.db", &db);
    if (rc) {
        cout << "Error: database could not be opened or created" << endl;
        sqlite3_close(db);
        return(1);
    }
    
    /*
        To ensure that tags are unique, use the unique constraint when creating the table.
        https://stackoverflow.com/questions/19337029/insert-if-not-exists-statement-in-sqlite
    */
    const char* sqlmsg = R"(
    CREATE TABLE IF NOT EXISTS drives (
        drive_uuid INTEGER PRIMARY KEY AUTOINCREMENT,
        drive_id_win INTEGER UNIQUE,
        drive_id_mac TEXT UNIQUE,
        drive_name TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS files (
        file_uuid INTEGER PRIMARY KEY AUTOINCREMENT,
        file_id_win INTEGER UNIQUE,
        file_id_mac INTEGER UNIQUE,
        drive_uuid TEXT NOT NULL,
        file_path TEXT NOT NULL,
        FOREIGN KEY (drive_uuid) REFERENCES drives(drive_uuid)
    );

    CREATE TABLE IF NOT EXISTS tags (
        tag_uuid INTEGER PRIMARY KEY AUTOINCREMENT,
        file_id INTEGER NOT NULL,
        tag_name TEXT NOT NULL,
        FOREIGN KEY (file_id) REFERENCES files(file_uuid),
        UNIQUE(file_id, tag_name)
    );
    )";

    rc = sqlite3_exec(db, sqlmsg, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        cout << "Error creating the tables for the database." << endl;
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
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
    
    // Validation check: print everything that was just saved to the database
    sqlmsg = R"(
        select * from tags
    )";

    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, sqlmsg, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cout << "Failed to prepare statement for printing tags: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* tagName = sqlite3_column_text(stmt, 2);
        std::cout << "tag_name: " << (tagName ? reinterpret_cast<const char*>(tagName) : "NULL") << std::endl;
    }

    if (rc != SQLITE_DONE) {
        std::cout << "Error retrieving data: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);

    sqlite3_close(db);
    return 0;
}