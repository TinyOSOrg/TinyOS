#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

extern "C"
{
#define TINY_OS_NO_INTDEF
#include <shared/filesys/dpt.h>
}

dpt_unit dpts[DPT_UNIT_COUNT];

int main(int argc, const char *argv[])
{
    if(argc != 2)
    {
        cout << "Usage: mkdpt [disk image file name]" << endl;
        return 0;
    }

    for(size_t i = 1; i != DPT_UNIT_COUNT; ++i)
    {
        dpt_unit &dpt    = dpts[i];
        dpt.type         = DISK_PT_NONEXISTENT;
        dpt.sector_begin = 0;
        dpt.sector_end   = 0;
        strcpy(dpt.name, ("init-pt-" + to_string(i)).c_str());
    }
    dpts[0].type         = DISK_PT_NOFS;
    dpts[0].sector_begin = DPT_SECTOR_POSITION + 1;
    dpts[0].sector_end   = 262143;
    strcpy(dpts[0].name, "init-pt-0");

    ofstream fout("mkdpttmp", ofstream::binary | ofstream::out);
    if(!fout)
    {
        cout << "Failed to open tmp file: mkdpttmp" << endl;
        return -1;
    }

    fout.write((const char*)&dpts, sizeof(dpts));
    fout.close();

    stringstream sst;
    sst << "dd if=mkdpttmp of=" << argv[1] << " bs=512 count=1 seek="
        << to_string(DPT_SECTOR_POSITION) << " conv=notrunc";
    int x = system(sst.str().c_str());

    x = system("rm -f mkdpttmp");

    (void)x;

    cout << "DPT inited" << endl;
}
