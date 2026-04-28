#include<iostream>
#include<memory>
#include<ctime>
#include<cstring>
#include "HuffmanCompressor.h"
#include "FolderArchiver.h"
using namespace std;

void printUsage(const char* program) {
    cout << "参数错误！" << endl;
    cout << "用法：" << endl;
    cout << "  单文件压缩：" << program << " cpr   input.txt  output.huff" << endl;
    cout << "  单文件解压：" << program << " decpr input.huff output.txt" << endl;
    cout << "  文件夹压缩：" << program << " cprf  input_folder output.huff" << endl;
    cout << "  文件夹解压：" << program << " decpr input.huff   output_folder" << endl;
}

bool compressFolder(const string& folder_path, const string& output_path) {
    FolderArchiver archiver;
    vector<uint8_t> archived_data;
    
    cout << "Archiving folder..." << endl;
    if (!archiver.archiveFolder(folder_path, archived_data)) {
        return false;
    }
    
    // 将打包后的数据写入临时文件
    FileProcess temp_file;
    temp_file.size = archived_data.size();
    temp_file.data = new unsigned char[archived_data.size()];
    memcpy(temp_file.data, archived_data.data(), archived_data.size());
    
    // 使用霍夫曼压缩
    unique_ptr<HuffmanCompress> com = make_unique<HuffmanCompress>();
    int time = clock();
    com->encode(&temp_file, output_path);
    com->evaluateEfficency(&temp_file, com->output, time);
    
    delete[] temp_file.data;
    temp_file.data = nullptr;
    
    return true;
}

bool decompressFolder(FileProcess* input, const string& output_folder) {
    unique_ptr<HuffmanCompress> com = make_unique<HuffmanCompress>();
    
    // 确保输出目录存在
    filesystem::create_directories(output_folder);
    
    // 先解压霍夫曼数据到临时文件
    string temp_file = output_folder + "archive.tmp";
    com->decode(input, temp_file);
    
    // 读取解压后的数据
    ifstream file(temp_file, ios::binary);
    if (!file) {
        cerr << "Error: Cannot read decompressed data" << endl;
        return false;
    }
    
    file.seekg(0, ios::end);
    size_t size = file.tellg();
    file.seekg(0, ios::beg);
    
    vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();
    
    // 删除临时文件
    filesystem::remove(temp_file);
    
    // 解压归档
    FolderArchiver archiver;
    return archiver.extractArchive(data, output_folder);
}

int main(int argc, char* argv[])
{
    try {
        if(argc == 4) {
            if(strcmp(argv[1], "cprf") == 0) {
                // 文件夹压缩
                if (!compressFolder(argv[2], argv[3])) {
                    cerr << "Folder compression failed!" << endl;
                    return 1;
                }
                cout << "Folder compressed successfully!" << endl;
            }
            else if(strcmp(argv[1], "cpr") == 0) {
                // 单文件压缩
                unique_ptr<FileProcess> f = make_unique<FileProcess>();
                f->fileRead(argv[2]);  
                unique_ptr<HuffmanCompress> com = make_unique<HuffmanCompress>();
                int time = clock();
                com->encode(f.get(), argv[3]);
                com->evaluateEfficency(f.get(), com->output, time);
            }
            else if(strcmp(argv[1], "decpr") == 0) {
                // 解压（自动判断单文件或文件夹）
                unique_ptr<FileProcess> f = make_unique<FileProcess>();
                f->fileRead(argv[2]);  
                
                // 检查是否是文件夹归档：通过检查输出路径是否以/或\结尾
                string output_path = argv[3];
                bool is_folder_output = !output_path.empty() && 
                                       (output_path.back() == '/' || output_path.back() == '\\');
                
                if (is_folder_output) {
                    if (!decompressFolder(f.get(), output_path)) {
                        cerr << "Folder decompression failed!" << endl;
                        return 1;
                    }
                    cout << "Decompress sucessfully!" << endl;
                } else {
                    unique_ptr<HuffmanCompress> com = make_unique<HuffmanCompress>();
                    com->decode(f.get(), argv[3]);
                    cout << "Decompress sucessfully!" << endl;
                }
            }
            else {
                cout << "error:command, you must use cpr / cprf / decpr" << endl;
                printUsage(argv[0]);
            }
        }
        else {
            printUsage(argv[0]);
        }
    }
    catch (const char* err) {
        cout << "\nX process error:" << err << endl;
        return 1;
    }

    return 0;
}
