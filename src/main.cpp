#include <iostream>
#include <filesystem>
#include <shobjidl.h> // SHGetPropertyStoreFromParsingName()
#include <propkey.h>
#include <propvarutil.h>
#include <propsys.h>
#include <windows.h>
#include <objbase.h>
#include <combaseapi.h>

using namespace std;

int main() {
    try {
        filesystem::path current_path = filesystem::current_path();
        // cout << "Current path: " << current_path << endl;

        for (auto const& entry: filesystem::directory_iterator{current_path}) {

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
                Get a property store for the file
            */
            CoInitialize(NULL);
            IPropertyStore* store = NULL;
            HRESULT hr = SHGetPropertyStoreFromParsingName(filePath, NULL, GPS_READWRITE, 
            __uuidof(IPropertyStore), (void**)&store);
            
            if (SUCCEEDED(hr)) {
                // IShellItem* pShellItem = NULL;
                // hr = SHCreateItemFromParsingName(filePath, NULL, IID_PPV_ARGS(&pShellItem));

                PROPVARIANT keywords;
                hr = store->GetValue(PKEY_Keywords, &keywords);

                if (SUCCEEDED(hr) && keywords.vt == (VT_LPSTR|VT_VECTOR)) {

                    // Iterate through each of the tags, just as a way to check
                    ULONG numKeywords = keywords.calpstr.cElems;

                    for (ULONG i = 0; i < numKeywords; i++) {
                        wcout << keywords.calpstr.pElems[i] << endl;
                    }

                }

                PropVariantClear(&keywords);
            }
            store->Release();
            CoUninitialize();
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "Error: " << e.what() << endl;
    }

}

// g++ -std=c++17 -o main main.cpp -lstdc++fs -lole32 -lpropsys