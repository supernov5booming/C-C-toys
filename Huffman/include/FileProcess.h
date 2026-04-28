#include<iostream>
#include<fstream>
#include<filesystem>
#include<string>
#include<vector>
class FileProcess{
 public:
    unsigned char* data;
    unsigned int size;
    
    FileProcess():data(nullptr),size(0){};
    ~FileProcess() {
        delete[] data;
    }
    
    FileProcess(const FileProcess&) = delete;
    FileProcess& operator=(const FileProcess&) = delete;


    void fileWriteBits(std::vector<uint8_t>& file,const std::string& src);

    void fileWriteBytes(std::string& file,const std::string& src);

    void fileRead(const std::string& src);
    
    
};