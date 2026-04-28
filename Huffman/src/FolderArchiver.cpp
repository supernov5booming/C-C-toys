#include "FolderArchiver.h"
#include <fstream>
#include <iostream>
#include <cstring>

// 归档文件魔数
const char ARCHIVE_MAGIC[] = "HUFFARC";
const uint8_t ARCHIVE_VERSION = 1;

void FolderArchiver::collectFiles(const fs::path& folder, const fs::path& base) {
    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            FileEntry fe;
            fe.relative_path = fs::relative(entry.path(), base).string();
            fe.size = entry.file_size();
            fe.modify_time = 
                std::chrono::duration_cast<std::chrono::seconds>(
                    entry.last_write_time().time_since_epoch()
                ).count();
            entries_.push_back(fe);
        }
    }
}

bool FolderArchiver::archiveFolder(const std::string& folder_path, std::vector<uint8_t>& output) {
    fs::path folder(folder_path);
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        std::cerr << "Error: Not a valid folder: " << folder_path << std::endl;
        return false;
    }
    
    entries_.clear();
    collectFiles(folder, folder);
    
    // 写入魔数和版本
    output.insert(output.end(), ARCHIVE_MAGIC, ARCHIVE_MAGIC + 7);
    output.push_back(ARCHIVE_VERSION);
    
    // 写入文件数量 (8字节)
    uint64_t file_count = entries_.size();
    for (int i = 7; i >= 0; --i) {
        output.push_back((file_count >> (i * 8)) & 0xFF);
    }
    
    // 计算文件内容起始偏移
    size_t header_size = 8 + 8; // 魔数7+版本1 + 文件数量8
    for (const auto& entry : entries_) {
        header_size += 8 + 8 + 8 + 2 + entry.relative_path.length(); // size + offset + time + path_len + path
    }
    
    // 写入文件元数据
    uint64_t current_offset = header_size;
    for (auto& entry : entries_) {
        entry.offset = current_offset;
        
        // 文件大小 (8字节)
        for (int i = 7; i >= 0; --i) {
            output.push_back((entry.size >> (i * 8)) & 0xFF);
        }
        // 文件偏移 (8字节)
        for (int i = 7; i >= 0; --i) {
            output.push_back((entry.offset >> (i * 8)) & 0xFF);
        }
        // 修改时间 (8字节)
        for (int i = 7; i >= 0; --i) {
            output.push_back((entry.modify_time >> (i * 8)) & 0xFF);
        }
        // 路径长度 (2字节)
        uint16_t path_len = entry.relative_path.length();
        output.push_back((path_len >> 8) & 0xFF);
        output.push_back(path_len & 0xFF);
        // 路径字符串
        output.insert(output.end(), entry.relative_path.begin(), entry.relative_path.end());
        
        current_offset += entry.size;
    }
    
    // 写入文件内容
    for (const auto& entry : entries_) {
        fs::path full_path = folder / entry.relative_path;
        std::ifstream file(full_path, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Cannot read file: " << full_path << std::endl;
            return false;
        }
        
        std::vector<uint8_t> buffer(entry.size);
        file.read(reinterpret_cast<char*>(buffer.data()), entry.size);
        output.insert(output.end(), buffer.begin(), buffer.end());
    }
    
    std::cout << "Archived " << entries_.size() << " files" << std::endl;
    return true;
}

bool FolderArchiver::readArchiveHeader(const uint8_t* data, size_t& pos, size_t total_size) {
    // 检查魔数
    if (total_size < 8 || memcmp(data, ARCHIVE_MAGIC, 7) != 0) {
        std::cerr << "Error: Invalid archive format" << std::endl;
        return false;
    }
    pos = 7;
    
    // 检查版本
    uint8_t version = data[pos++];
    if (version != ARCHIVE_VERSION) {
        std::cerr << "Error: Unsupported archive version: " << (int)version << std::endl;
        return false;
    }
    
    // 读取文件数量
    if (pos + 8 > total_size) return false;
    uint64_t file_count = 0;
    for (int i = 0; i < 8; ++i) {
        file_count = (file_count << 8) | data[pos++];
    }
    
    entries_.clear();
    entries_.reserve(file_count);
    
    // 读取每个文件的元数据
    for (uint64_t i = 0; i < file_count; ++i) {
        if (pos + 26 > total_size) return false;
        
        FileEntry entry;
        
        // 文件大小
        entry.size = 0;
        for (int j = 0; j < 8; ++j) {
            entry.size = (entry.size << 8) | data[pos++];
        }
        // 文件偏移
        entry.offset = 0;
        for (int j = 0; j < 8; ++j) {
            entry.offset = (entry.offset << 8) | data[pos++];
        }
        // 修改时间
        entry.modify_time = 0;
        for (int j = 0; j < 8; ++j) {
            entry.modify_time = (entry.modify_time << 8) | data[pos++];
        }
        // 路径长度
        uint16_t path_len = (data[pos] << 8) | data[pos + 1];
        pos += 2;
        
        if (pos + path_len > total_size) return false;
        entry.relative_path.assign(reinterpret_cast<const char*>(data + pos), path_len);
        pos += path_len;
        
        entries_.push_back(entry);
    }
    
    return true;
}

bool FolderArchiver::extractArchive(const std::vector<uint8_t>& data, const std::string& output_folder) {
    size_t pos = 0;
    if (!readArchiveHeader(data.data(), pos, data.size())) {
        return false;
    }
    
    fs::path out_folder(output_folder);
    fs::create_directories(out_folder);
    
    for (const auto& entry : entries_) {
        fs::path out_path = out_folder / entry.relative_path;
        fs::create_directories(out_path.parent_path());
        
        if (entry.offset + entry.size > data.size()) {
            std::cerr << "Error: Corrupted archive, file data out of bounds" << std::endl;
            return false;
        }
        
        std::ofstream file(out_path, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Cannot create file: " << out_path << std::endl;
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(data.data() + entry.offset), entry.size);
        
        // 恢复修改时间
        auto time = std::chrono::seconds(entry.modify_time);
        fs::last_write_time(out_path, fs::file_time_type(time));
    }
    
    std::cout << "Extracted " << entries_.size() << " files" << std::endl;
    return true;
}
