#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct FileEntry {
    std::string relative_path;
    uint64_t size;
    uint64_t offset;
    uint64_t modify_time;
};

class FolderArchiver {
public:
    FolderArchiver() = default;
    
    bool archiveFolder(const std::string& folder_path, std::vector<uint8_t>& output);
    bool extractArchive(const std::vector<uint8_t>& data, const std::string& output_folder);
    
private:
    std::vector<FileEntry> entries_;
    
    void collectFiles(const fs::path& folder, const fs::path& base);
    bool writeArchiveHeader(std::vector<uint8_t>& output);
    bool readArchiveHeader(const uint8_t* data, size_t& pos, size_t total_size);
};
