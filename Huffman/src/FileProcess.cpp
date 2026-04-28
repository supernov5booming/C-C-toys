#include "FileProcess.h"
#include<vector>

void FileProcess::fileRead(const std::string& src)
{
    // 先释放之前的数据，确保异常安全
    delete[] data;
    data = nullptr;
    size = 0;

    // 安全检查：文件必须是 txt、jpg、huff
    if (src.substr(src.size() - 3) != "txt" && 
    src.substr(src.size() - 3) != "jpg" && 
    src.substr(src.size() - 4) != "huff" && 
    src.substr(src.size() - 3) != "png") {
        std::cerr << "False:not a required file!" << std::endl;
        throw "File type error";
    }

    // 打开文件
    std::ifstream fin(src, std::ios::binary | std::ios::ate);
    if (!fin.is_open()) {
        std::cerr << "Error:can not open a file -> " << src << std::endl;
        throw "文件无法打开";
    }

    int fileSize = (int)fin.tellg();
    size = fileSize;

    fin.seekg(0, std::ios::beg);

    data = new unsigned char[size];

    fin.read((char*)data, size);

    fin.close();
    
    std::cout << "---------------------------------------------------------------------" << std::endl;
    std::cout << "File Load Successfully!" << std::endl;
    if (src.substr(src.size() - 4) == "huff")
        std::cout << "File type:huff" << std::endl;
    else
        std::cout << "File type:" << src.substr(src.size() - 3) << std::endl;
    std::cout << "---------------------------------------------------------------------" << std::endl;
}

void FileProcess::fileWriteBytes(std::string& file, const std::string& src)
{
    std::ofstream fout(src, std::ios::binary);
    if (!fout.is_open()) {
        std::cerr << "File open failed!" << std::endl;
        return;
    }
    fout.write(file.c_str(), file.size());
    size = file.size() * 8;
    fout.close();
}


void FileProcess::fileWriteBits(std::vector<uint8_t>& input, const std::string& src)
{
    std::ofstream fout(src, std::ios::binary);
     if (!fout.is_open()) {
        std::cerr << "File open failed!" << std::endl;
        return;
    }
    fout.write((const char*)input.data(), input.size());
    size = input.size() * 8;
    fout.close();
}