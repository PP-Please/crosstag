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

// #include <nlohmann/json.hpp>
// using json = nlohmann::json;

using namespace std;

int main() {
    try {
        filesystem::path current_path = filesystem::current_path();
        // cout << "Current path: " << current_path << endl;

        /*
            Load the hidden file containing tags for each file within the directory, or create it if
            it does not exist. 
        */
       HANDLE hFile = CreateFileA("storedTags.json", (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            cout << "Error: The hidden file was unable to be created or read." << endl;
            exit(1);
        }

        ifstream tags("storedTags.json");

        for (auto const& entry: filesystem::directory_iterator{current_path}) {
            if (entry.path().filename() == "storedTags.json") continue;
    

            // cout << "Currently on file named" << entry.path().filename() << endl;
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
            hFile = CreateFileA(entry.path().filename().string().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                cout << "Error: Unable to open the existing file named" << entry.path().filename() << endl;
                exit(1);
            }

            BY_HANDLE_FILE_INFORMATION fileInfo;
            int fileId;
            if (GetFileInformationByHandle(hFile, &fileInfo)) {
                fileId = fileInfo.nFileIndexHigh;
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

                if (SUCCEEDED(hr) && keywords.vt == (VT_LPWSTR|VT_VECTOR)) {
                    
                    // cout << "Successfully got into this loop!" << endl;
                    // Iterate through each of the tags, just as a way to check
                    ULONG numKeywords = keywords.calpstr.cElems;

                    for (ULONG i = 0; i < numKeywords; i++) {
                        /*
                            Currently, the string is a LPSTR (pointer to a string), which is why printing it like
                            below will only print the first character of the tag. Instead, we add a wide string
                            literal interpret as a wchar_t*. If we passed it alone like belo, the compiler interprets
                            the tag as a pointer and thus through conversion may treat it as a single character rather
                            than an array of wchar_t characters.
                            wcout << keywords.calpstr.pElems[i] << endl;
                        */
                        wcout << L"" << keywords.calpwstr.pElems[i] << endl;
                    }

                }

                PropVariantClear(&keywords);
                store->Release();
            }
            CoUninitialize();
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error: " << e.what() << endl;
    }

}

// g++ -o main main.cpp -lole32 -loleaut32 -lpropsys