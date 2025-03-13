#include <iostream>
#include <filesystem>
#include <shobjidl.h> // SHGetPropertyStoreFromParsingName()
#include <fstream>
#include <unordered_set>
#include <vector>

#ifdef __WIN32
// #define INITGUID
// #include <windows.h>
// #include <propkey.h>
// #include <propvarutil.h>
// #include <propsys.h>
// #include <objbase.h>
// #include <combaseapi.h>
// #include <winbase.h>
// #include <bits/stdc++.h> // unordered map
#include "winFunctions.h"
#endif

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace std;

int main() {
    filesystem::path current_path = filesystem::current_path();

    /*
        Load the hidden file containing tags for each file within the directory, or create it if
        it does not exist. 
    */
    createJsonTagsFile();

    unordered_map<int, vector<string>> allTags;

    try {
        for (auto const& entry: filesystem::directory_iterator{current_path}) {
            if (entry.path().filename() == "storedTags.json" || entry.is_directory()) continue;

            saveTagsToMap(allTags, entry);
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