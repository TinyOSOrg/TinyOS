#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        cout << "Usage: bin_trans binary_file_name" << endl;
        return -1;
    }

    ifstream fin(argv[1], ifstream::in | ifstream::binary);
    if(!fin)
    {
        cout << "Failed to open file: " << argv[1] << endl;
        return -1;
    }

    string content = string(istreambuf_iterator<char>(fin),
                            istreambuf_iterator<char>());
    int count = 0;
    for(char c : content)
    {
        cout << (int)c << ", ";
        if(count++ % 20 == 0)
            cout << endl;
    }
    cout << "\b\b";
    
    return 0;
}
