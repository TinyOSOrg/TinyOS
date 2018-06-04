#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <src/tools/disk.h>

extern "C"
{
#define TINY_OS_NO_INTDEF
#include <shared/filesys.h>
}

dpt_unit dpts[DPT_UNIT_COUNT];

int main(int argc, const char *argv[])
{
    using namespace std;

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

    // 第一个分区用作afs分区
    dpts[0].type         = DISK_PT_NOFS;
    dpts[0].sector_begin = DPT_SECTOR_POSITION + 1;
    dpts[0].sector_end   = (DISK_IMPORT_DP_BEGIN - 1);
    strcpy(dpts[0].name, "sys");

#define LAST_DPT_IDX (DPT_UNIT_COUNT - 1)
    // 最后一个分区用作import分区
    dpts[LAST_DPT_IDX].type = DISK_PT_IMPORT;
    dpts[LAST_DPT_IDX].sector_begin = DISK_IMPORT_DP_BEGIN;
    dpts[LAST_DPT_IDX].sector_end = DISK_IMPORT_DP_END;
    strcpy(dpts[LAST_DPT_IDX].name, "import");
#undef LAST_DPT_IDX

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
    system(sst.str().c_str());
    system("rm -f mkdpttmp");

    cout << "DPT inited" << endl;
}
