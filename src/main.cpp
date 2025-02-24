#include <iostream>
#include <filesystem>
#include <shobjidl.h> // SHGetPropertyStoreFromParsingName()

#define INITGUID
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
        cout << "Current path: " << current_path << endl;

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

            cout << "File path initially: " << tempPath << endl;
            wcout << "Converted file path: " << filePath << endl;

            /*
                Get a property store for the file
            */
            CoInitialize(NULL);
            IPropertyStore* store = NULL;
            HRESULT hr = SHGetPropertyStoreFromParsingName(filePath, NULL, GPS_DEFAULT, 
            __uuidof(IPropertyStore), (void**)&store);

            std::cout << "HRESULT for property store:" << std::hex << std::uppercase << hr << std::endl;
            
            if (SUCCEEDED(hr)) {
                // IShellItem* pShellItem = NULL;
                // hr = SHCreateItemFromParsingName(filePath, NULL, IID_PPV_ARGS(&pShellItem));
                cout << "Successfuly got a property store" << endl; 

                PROPVARIANT keywords;
                hr = store->GetValue(PKEY_Keywords, &keywords);

                std::cout << "HRESULT for keyword retrieval:" << std::hex << std::uppercase << hr << std::endl;

                cout << keywords.vt << endl;

                if (SUCCEEDED(hr) && keywords.vt == (VT_LPWSTR|VT_VECTOR)) {
                    
                    std::cout << "Successfully got into this loop!" << endl;
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