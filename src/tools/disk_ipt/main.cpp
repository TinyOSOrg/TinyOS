#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <src/tools/disk.h>

extern "C"
{
#define TINY_OS_NO_INTDEF
#include <shared/filesys.h>
}

using namespace std;

int main(int argc, char *argv[])
{
    if(argc < 4 || (argc - 2) % 2 == 1)
    {
        cout << "Usage: disk_ipt disk_img [filename1 dstname1] ..." << endl;
        return -1;
    }

    // 打开临时输出文件
    ofstream tout("disk_ipt_tmp", ofstream::out | ofstream::binary);

    uint32_t file_count = (argc - 2) / 2;
    tout.write((const char*)&file_count, sizeof(uint32_t));

    // 写入临时文件的总字节数
    size_t total_byte_size = sizeof(uint32_t);

    for(int i = 2; i < argc; i += 2)
    {
        // 读取文件所有内容

        ifstream fin(argv[i], ifstream::in | ifstream::binary);
        if(!fin)
        {
            cout << "Failed to open file " << argv[i] << endl;
            return -1;
        }

        fin.seekg(0, ios::end);
        vector<char> data(fin.tellg());

        fin.seekg(0, ios::beg);
        fin.read(data.data(), data.size());

        fin.close();

        // 写入目标分区号
        uint32_t dp = 0;
        tout.write((const char*)&dp, sizeof(uint32_t));

        // 写入目标路径
        tout.write(argv[i + 1], IPT_PATH_BUF_SIZE);

        // 写入目标文件大小
        uint32_t file_size = data.size();
        tout.write((const char*)&file_size, sizeof(uint32_t));

        // 写入目标文件内容
        tout.write((const char*)data.data(), data.size());

        total_byte_size += sizeof(uint32_t) * 2 + IPT_PATH_BUF_SIZE
                         + data.size();
    }

    tout.close();

    uint32_t sec_cnt = total_byte_size / 512 +
                        (total_byte_size % 512 ? 1 : 0);

    stringstream sst;
    sst << "dd if=disk_ipt_tmp of=" << argv[1] << " bs=512 count="
        << sec_cnt << " seek=" << to_string(DISK_IMPORT_DP_BEGIN)
        << " conv=notrunc";
    system(sst.str().c_str());
    system("rm -f disk_ipt_tmp");

    cout << "Import dp inited" << endl;
}
