#ifdef _WIN32

#define INITGUID
// #include <iostream>
#include <windows.h>
#include <propkey.h>
#include <propvarutil.h>
#include <propsys.h>
#include <objbase.h>
#include <combaseapi.h>
#include <winbase.h>
#include <vector>
#include <bits/stdc++.h> // unordered map
#include <unordered_set>
#include <filesystem>
#include <shobjidl.h>

#include "tagFunctions.h"

using namespace std;

void createJsonTagsFile() {
    HANDLE hFile = CreateFileA("storedTags.json", (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Error: The hidden file was unable to be created or read." << endl;
        exit(1);
    }
}

void saveTagsToMap(unordered_map<int, vector<string>>& allTags, filesystem::directory_entry entry) {
/*
    Converting the filePath into a PCWSTR to later be able to view the tags (keywords)
    of the file.

    Credits to Stack Overflow
    https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode           
*/
string tempPath = entry.path().string();
wstring stemp = wstring(tempPath.begin(), tempPath.end());
PCWSTR filePath = stemp.c_str();

/*
    Open the current file in order to retrieve its unique FILE_ID_128 FileId (Windows only)
    Since FILE_ID_INFO is not supported on commercial clients, I used the fact that the nFileIndexHigh
    is unique within a volume.
    https://stackoverflow.com/questions/1866454/unique-file-identifier-in-windows
*/
HANDLE hFile = CreateFileA(entry.path().filename().string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
if (hFile == INVALID_HANDLE_VALUE) {
    cout << "Error: Unable to open the existing file named" << entry.path().filename() << endl;
    exit(1);
}

BY_HANDLE_FILE_INFORMATION fileInfo;
int fileId;
if (GetFileInformationByHandle(hFile, &fileInfo)) {
    fileId = fileInfo.nFileIndexHigh;
} else {
    cout << "Error finding fileId" << endl;
    exit(1);
}

CloseHandle(hFile);

// cout << "File path initially: " << tempPath << endl;
// wcout << "Converted file path: " << filePath << endl;

/*
    Get a property store for the file
*/
CoInitialize(NULL);
IPropertyStore* store = NULL;
HRESULT hr = SHGetPropertyStoreFromParsingName(filePath, NULL, GPS_DEFAULT, 
__uuidof(IPropertyStore), (void**)&store);

// std::cout << "HRESULT for property store:" << std::hex << std::uppercase << hr << std::endl;

if (SUCCEEDED(hr)) {
    // cout << "Successfuly got a property store" << endl; 

    PROPVARIANT keywords;
    hr = store->GetValue(PKEY_Keywords, &keywords);

    // cout << "HRESULT for keyword retrieval:" << hr << endl;
    // cout << keywords.vt << endl;

    vector<string> tags;

    if (SUCCEEDED(hr) && keywords.vt == (VT_LPWSTR|VT_VECTOR)) {
        
        // cout << "Successfully got into this loop!" << endl;
        // Iterate through each of the tags, just as a way to check
        ULONG numKeywords = keywords.calpstr.cElems;

        for (ULONG i = 0; i < numKeywords; i++) {
            /*
                This comment is about printing out a string to the terminal. 
                Currently, the string is a LPSTR (pointer to a string), which is why printing it like
                below will only print the first character of the tag. Instead, we add a wide string
                literal interpret as a wchar_t*. If we passed it alone like below, the compiler interprets
                the tag as a pointer and thus through conversion may treat it as a single character rather
                than an array of wchar_t characters.
                wcout << keywords.calpstr.pElems[i] << endl;
            */
            wstring ws(keywords.calpwstr.pElems[i]);
            tags.push_back(string(ws.begin(), ws.end()));
            // wcout << L"" << keywords.calpwstr.pElems[i] << endl;
        }

    }

    // for (int i = 0; i < tags.size(); i++) {
    //     cout << tags[i] << endl;
    // }

    PropVariantClear(&keywords);
    store->Release();

    allTags.insert({fileId, tags});
    cout << "Inserted with values" << fileId << endl;
}
CoUninitialize();
}

void saveTagsToDB(sqlite3& db, filesystem::directory_entry entry) {
    /*
    Converting the filePath into a PCWSTR to later be able to view the tags (keywords)
    of the file.

    Credits to Stack Overflow
    https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode           
    */
    string tempPath = entry.path().string();
    wstring stemp = wstring(tempPath.begin(), tempPath.end());
    PCWSTR filePath = stemp.c_str();

    /*
        Open the current file in order to retrieve its unique FILE_ID_128 FileId (Windows only)
        Since FILE_ID_INFO is not supported on commercial clients, I used the fact that the nFileIndexHigh
        is unique within a volume.
        https://stackoverflow.com/questions/1866454/unique-file-identifier-in-windows
    */
    HANDLE hFile = CreateFileA(entry.path().filename().string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Error: Unable to open the existing file named" << entry.path().filename() << endl;
        exit(1);
    }

    BY_HANDLE_FILE_INFORMATION fileInfo;
    int fileId;
    DWORD volumeId;
    if (GetFileInformationByHandle(hFile, &fileInfo)) {
        fileId = fileInfo.nFileIndexHigh;
        volumeId = fileInfo.dwVolumeSerialNumber;
    } else {
        cout << "Error finding fileId" << endl;
        exit(1);
    }

    CloseHandle(hFile);

    createFilesSQLTable(fileId, entry.path(), db, volumeId);

    CoInitialize(NULL);
    IPropertyStore* store = NULL;
    HRESULT hr = SHGetPropertyStoreFromParsingName(filePath, NULL, GPS_DEFAULT, 
    __uuidof(IPropertyStore), (void**)&store);


    if (SUCCEEDED(hr)) {

        PROPVARIANT keywords;
        hr = store->GetValue(PKEY_Keywords, &keywords);

        if (SUCCEEDED(hr) && keywords.vt == (VT_LPWSTR|VT_VECTOR)) {
            ULONG numKeywords = keywords.calpstr.cElems;

            for (ULONG i = 0; i < numKeywords; i++) {
                const char* sqlmsg = R"(INSERT INTO tags(file_id, tag) VALUES (?, ?) ON CONFLICT(file_id, tag) DO NOTHING;)";
                sqlite3_stmt* stmt;

                int rc = sqlite3_prepare_v2(&db, sqlmsg, -1, &stmt, nullptr);
                if (rc != SQLITE_OK) {
                    cerr << "Failed to prepare statement: " << sqlite3_errmsg(&db) << endl;
                    return;
                }

                sqlite3_bind_int(stmt, 1, fileId);
                wstring ws(keywords.calpwstr.pElems[i]);
                sqlite3_bind_text(stmt, 2, string(ws.begin(), ws.end()).c_str(), -1, SQLITE_STATIC);

                rc = sqlite3_step(stmt);
                if (rc != SQLITE_DONE) {
                    cerr << "Error executing statement: " << sqlite3_errmsg(&db) << endl;
                }

                sqlite3_finalize(stmt);
            }

        }
        PropVariantClear(&keywords);
        store->Release();
    }
    CoUninitialize();
}

/*
    https://stackoverflow.com/questions/62816916/how-to-get-drives-filesystem-information-in-winapi
*/
void getAndSaveDriveData(sqlite3& db) {
    WCHAR volumeName[MAX_PATH + 1] = { 0 };
    // LPSTR volumeName;
    WCHAR fileSystemName[MAX_PATH + 1] = { 0 };
    DWORD serialNumber = 0;
    DWORD maxComponentLen = 0;
    DWORD fileSystemFlags = 0;

    // Since we want the root path (drive name), first parameter is NULL
    if (GetVolumeInformation(NULL, volumeName, sizeof(volumeName), &serialNumber, &maxComponentLen, &fileSystemFlags, fileSystemName, sizeof(fileSystemName))) {
        
        // Check whether the drive has already been saved to the drives table.
        const char* sqlmsg = R"("SELECT drive_uuid FROM drives WHERE drive_id_win = ?")";
        sqlite3_stmt* stmt;

        int rc = sqlite3_prepare_v2(&db, sqlmsg, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            cerr << "Failed to prepare statement: " << sqlite3_errmsg(&db) << endl;
            return;
        }
        sqlite3_bind_text(stmt, 1, to_string(serialNumber).c_str(), -1, SQLITE_STATIC);
        bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

        if (!exists) {
            sqlmsg = R"(INSERT INTO drives(drive_uuid, drive_id_win, drive_id_mac, drive_name) 
            VALUES (?, ?, ?, ?) ON CONFLICT(drive_uuid, drive_id_win, drive_id_mac, drive_name) DO NOTHING;)";
            sqlite3_clear_bindings(stmt);

            rc = sqlite3_prepare_v2(&db, sqlmsg, -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                cerr << "Failed to prepare statement: " << sqlite3_errmsg(&db) << endl;
                return;
            }

            char convertedVolumeName[MAX_PATH + 1] = {0};
            int len = WideCharToMultiByte(CP_UTF8, 0, volumeName, -1, convertedVolumeName, MAX_PATH, NULL, NULL);
            if (len == 0) {
                std::cerr << "WideCharToMultiByte conversion failed!" << std::endl;
                sqlite3_finalize(stmt);
                return;
            }

            sqlite3_bind_text(stmt, 1, generateUuidV4().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, to_string(serialNumber).c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_null(stmt, 3);
            sqlite3_bind_text(stmt, 4, convertedVolumeName, -1, SQLITE_STATIC);
            
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to insert: " << sqlite3_errmsg(&db) << std::endl;
            }

            sqlite3_finalize(stmt);
        }


    }
}

void createFilesSQLTable(int fileId, filesystem::path filePath, sqlite3& db, DWORD volumeId) {
    const char* sqlmsg = R"(INSERT INTO files(file_uuid, file_id_win, file_id_mac, drive_id_win, drive_id_mac, file_path) 
    VALUES (?, ?, ?, ?, ?) ON CONFLICT(file_uuid, file_id_win, file_id_mac, drive_id, file_path) DO NOTHING;)";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(&db, sqlmsg, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(&db) << endl;
        return;
    }

    char convertedVolumeName[MAX_PATH + 1] = {0};

    sqlite3_bind_text(stmt, 1, generateUuidV4().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, fileId);
    sqlite3_bind_null(stmt, 3);
    sqlite3_bind_text(stmt, 4, to_string(volumeId).c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_null(stmt, 5);
    sqlite3_bind_text(stmt, 6, filePath.filename().string().c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Failed to insert: " << sqlite3_errmsg(&db) << std::endl;
    }

    sqlite3_finalize(stmt);
}
#endif