#include <iostream>
#include <filesystem>
#include <shobjidl.h> // SHGetPropertyStoreFromParsingName()
#include <fstream>

#define INITGUID
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

#include "winFunctions.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;

int main() {
    filesystem::path current_path = filesystem::current_path();
    // cout << "Current path: " << current_path << endl;

    /*
        Load the hidden file containing tags for each file within the directory, or create it if
        it does not exist. 
    */
    createJsonTagsFile();

    unordered_map<int, vector<string>> allTags;

    try {

        // ifstream tags("storedTags.json");

        for (auto const& entry: filesystem::directory_iterator{current_path}) {
            if (entry.path().filename() == "storedTags.json" || entry.is_directory()) continue;

            saveTagsToMap(allTags, entry);
        //     // cout << "Currently on file named" << entry.path().filename() << endl;
        //     /*
        //         Converting the filePath into a PCWSTR to later be able to view the tags (keywords)
        //         of the file.

        //         Credits to Stack Overflow
        //         https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode           
        //     */
        //     string tempPath = entry.path().string();
        //     wstring stemp = wstring(tempPath.begin(), tempPath.end());
        //     PCWSTR filePath = stemp.c_str();

        //     /*
        //         Open the current file in order to retrieve its unique FILE_ID_128 FileId (Windows only)
        //         Since FILE_ID_INFO is not supported on commercial clients, I used the fact that the nFileIndexHigh
        //         is unique within a volume.
        //         https://stackoverflow.com/questions/1866454/unique-file-identifier-in-windows
        //     */
        //     hFile = CreateFileA(entry.path().filename().string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        //     if (hFile == INVALID_HANDLE_VALUE) {
        //         cout << "Error: Unable to open the existing file named" << entry.path().filename() << endl;
        //         exit(1);
        //     }

        //     BY_HANDLE_FILE_INFORMATION fileInfo;
        //     int fileId;
        //     if (GetFileInformationByHandle(hFile, &fileInfo)) {
        //         fileId = fileInfo.nFileIndexHigh;
        //     } else {
        //         cout << "Error finding fileId" << endl;
        //         exit(1);
        //     }

        //     CloseHandle(hFile);

        //     // cout << "File path initially: " << tempPath << endl;
        //     // wcout << "Converted file path: " << filePath << endl;

        //     /*
        //         Get a property store for the file
        //     */
        //     CoInitialize(NULL);
        //     IPropertyStore* store = NULL;
        //     HRESULT hr = SHGetPropertyStoreFromParsingName(filePath, NULL, GPS_DEFAULT, 
        //     __uuidof(IPropertyStore), (void**)&store);

        //     // std::cout << "HRESULT for property store:" << std::hex << std::uppercase << hr << std::endl;
            
        //     if (SUCCEEDED(hr)) {
        //         // cout << "Successfuly got a property store" << endl; 

        //         PROPVARIANT keywords;
        //         hr = store->GetValue(PKEY_Keywords, &keywords);

        //         // cout << "HRESULT for keyword retrieval:" << hr << endl;
        //         // cout << keywords.vt << endl;

        //         vector<string> tags;

        //         if (SUCCEEDED(hr) && keywords.vt == (VT_LPWSTR|VT_VECTOR)) {
                    
        //             // cout << "Successfully got into this loop!" << endl;
        //             // Iterate through each of the tags, just as a way to check
        //             ULONG numKeywords = keywords.calpstr.cElems;

        //             for (ULONG i = 0; i < numKeywords; i++) {
        //                 /*
        //                     This comment is about printing out a string to the terminal. 
        //                     Currently, the string is a LPSTR (pointer to a string), which is why printing it like
        //                     below will only print the first character of the tag. Instead, we add a wide string
        //                     literal interpret as a wchar_t*. If we passed it alone like below, the compiler interprets
        //                     the tag as a pointer and thus through conversion may treat it as a single character rather
        //                     than an array of wchar_t characters.
        //                     wcout << keywords.calpstr.pElems[i] << endl;
        //                 */
        //                wstring ws(keywords.calpwstr.pElems[i]);
        //                tags.push_back(string(ws.begin(), ws.end()));
        //                 // wcout << L"" << keywords.calpwstr.pElems[i] << endl;
        //             }

        //         }

        //         // for (int i = 0; i < tags.size(); i++) {
        //         //     cout << tags[i] << endl;
        //         // }

        //         PropVariantClear(&keywords);
        //         store->Release();

        //         allTags.insert({fileId, tags});
        //         cout << "Inserted with values" << fileId << endl;
        //     }
        //     CoUninitialize();
        // }
        } 
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error: " << e.what() << endl;
    }
    /*
        Check if the tags have already been saved to the storedTags JSON file and if not, create the JSON object
        to be added to the file.
    */
    ifstream jsonData("storedTags.json");
    if (!jsonData.is_open()) {
        cout << "Error: file could not be opened." << endl;
        exit(1);
    }
    json data;
    if (jsonData.peek() == EOF) {
        data = json::array();
    } else {
        data = json::parse(jsonData);
    }

    jsonData.close();
    if (jsonData.is_open()) {
        cout << "Error: the file was not closed after being opened." << endl;
        exit(1);
    }

    for (auto mapIt = allTags.begin(); mapIt != allTags.end(); mapIt++) {
        bool found = false;
        for (auto& it : data) {
            /*
                Loop checking if there exists an existing fileId which has already been saved before. If not, we can just add
                all the tags into a new object.
            */
            if (it.contains("fileId") && it["fileId"] == mapIt->first) {
                found = true;
                /*
                    Add all the already saved tags into an unordered set, and then iterate through the vector containing
                    the tags to be added to see if they already exist. If they do, continue and if not, add them to the list.
                    I decided to add to an unordered set because for m already saved tags and n tags to be saved, if we were
                    to use a traditional loop, the worse case would mean we check the m tags n times i.e. O(mn) complexity.
                    If we were to save it into a set instead however, we would need O(m) complexity to add everything to the
                    set and lookups are O(1).
                */
               unordered_set<string> existingTags;

               for (const auto& tag : it["Tags"]) {
                    cout << tag << endl;
                    existingTags.insert(tag);
                }

                for (const auto& tag : mapIt->second) {
                    if (existingTags.find(tag) == existingTags.end()) {
                        existingTags.insert(tag);
                        it["Tags"] = tag;
                    }
                }
                break;
            } 
        }

        if (!found) {
            json tagsToAdd;
            tagsToAdd["fileId"] = mapIt->first;
            tagsToAdd["Tags"] = mapIt->second;
            // data["Tags"] = tagsToAdd;
            data.push_back(tagsToAdd);
        }
    }

    cout << data.dump() << endl;
    ofstream jsonOutput("storedTags.json");
    if (!jsonOutput.is_open()) {
        cerr << "Error: Could not open file for writing." << endl;
        return 1;
    }

    // Write the modified data to the file
    jsonOutput << data.dump(4); // Using dump with indentation
    jsonOutput.close();

    return 0;
}
/*
    g++ -o main main.cpp -lole32 -loleaut32 -lpropsys

    cmake -G "Unix Makefiles" ..
    make    
*/