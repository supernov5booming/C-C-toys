#include <iostream>
#include <string>
#include <ctime>
#include "HuffmanCompressor.h"
using namespace std;

void HuffmanCompress::evaluateEfficency(FileProcess* input, FileProcess* output, clock_t start_time) {
    cout << "---------------------------------------------------------------------" << endl;
    cout << "Compressed successfully!" << endl << endl;
    cout << "Original file size: " << input->size << " Bytes (" << input->size * 8 << " Bits)" << endl;
    cout << "Compressed file size: " << output->size / 8 << " Bytes (" << output->size << " Bits)" << endl;
    double rate = (double)(output->size / 8) / input->size * 100;
    cout << "Compression ratio: " << rate << "%" << endl;
    cout << "Compression time: " << clock() - start_time << " ms" << endl;
    cout << "---------------------------------------------------------------------" << endl;
}

void HuffmanCompress::decode(FileProcess* input, string output_src) {
    if (!input || input->size == 0) return;

    const uint8_t* data = input->data;
    size_t pos = 0;
    
    size_t original_size = (data[pos] << 24) | (data[pos+1] << 16) | 
                          (data[pos+2] << 8) | data[pos+3];
    pos += 4;
    
    uint8_t flag = data[pos++];
    
    vector<uint8_t> result;
    result.reserve(original_size);
    
    if (flag == 0x01) {
        for (size_t i = 0; i < original_size && pos < input->size; i++) {
            result.push_back(data[pos++]);
        }
    } else {
        uint8_t padding = data[pos++];
        
        tree->deserializeFreqMap(data, pos);
        tree->original_size = original_size;
        tree->buildFromFreqMap();
        
        string bit_str;
        size_t compressed_data_size = input->size - pos;
        size_t total_bits = compressed_data_size * 8 - padding;
        bit_str.reserve(total_bits);
        
        for (size_t i = pos; i < input->size; i++) {
            uint8_t byte = data[i];
            for (int j = 7; j >= 0; j--) {
                if (bit_str.size() >= total_bits) break;
                bit_str += (byte & (1 << j)) ? '1' : '0';
            }
            if (bit_str.size() >= total_bits) break;
        }
        
        auto cur = tree->head;
        for (char c : bit_str) {
            if (result.size() >= original_size) break;
            
            if (c == '0') {
                cur = cur->left;
            } else {
                cur = cur->right;
            }
            
            if (!cur->left && !cur->right) {
                result.push_back(cur->s);
                cur = tree->head;
            }
        }
    }
    
    std::ofstream fout(output_src, std::ios::binary);
    if (!fout.is_open()) {
        std::cerr << "File open failed!" << std::endl;
        return;
    }
    fout.write((const char*)result.data(), result.size());
    fout.close();
    
    std::cout << "---------------------------------------------------------------------" << std::endl;
    std::cout << "Decompressed successfully!" << std::endl;
    std::cout << "Original file size: " << original_size << " Bytes" << std::endl;
    std::cout << "---------------------------------------------------------------------" << std::endl;
}

void HuffmanCompress::createBinMap(Bitstream bits, const shared_ptr<HuffmanNode>& cur)
{
    if (!cur->left && !cur->right) {
        b_map[cur->s] = bits;
        return;
    }
    if (cur->left) {
        Bitstream left_bits = bits;
        left_bits.append('0');
        createBinMap(left_bits, cur->left);
    }
    if (cur->right) {
        Bitstream right_bits = bits;
        right_bits.append('1');
        createBinMap(right_bits, cur->right);
    }
}

vector<uint8_t> HuffmanCompress::sequence(FileProcess* input)
{
    vector<uint8_t> bin_s;
    if (!input || input->size == 0)
        return bin_s;

    string out;
    out.reserve(input->size * 8);

    for (int i = 0; i < input->size; i++) {
        uint8_t ch = (uint8_t)input->data[i];
        if (b_map.find(ch) == b_map.end()) continue;
        out += b_map[ch].bits;
    }

    int len = 0;
    uint8_t res = 0;
    for (auto c : out) {
        res = (res << 1) | (c == '1');
        len++;
        if (len == 8) {
            bin_s.push_back(res);
            res = 0;
            len = 0;
        }
    }

    if (len > 0) {
        res <<= (8 - len);
        bin_s.push_back(res);
    }

    return bin_s;
}

void HuffmanCompress::encode(FileProcess* input, string output_src){
    if (!input || input->size == 0) return;

    b_map.clear();
    Bitstream bits;

    tree->build(input);
    tree->original_size = input->size;
    createBinMap(bits, tree->head);
    
    string out;
    out.reserve(input->size * 8);
    for (int i = 0; i < input->size; i++) {
        uint8_t ch = (uint8_t)input->data[i];
        if (b_map.find(ch) == b_map.end()) continue;
        out += b_map[ch].bits;
    }
    
    uint8_t padding = (8 - (out.size() % 8)) % 8;
    
    vector<uint8_t> pInput;
    int len = 0;
    uint8_t res = 0;
    for (auto c : out) {
        res = (res << 1) | (c == '1');
        len++;
        if (len == 8) {
            pInput.push_back(res);
            res = 0;
            len = 0;
        }
    }
    if (len > 0) {
        res <<= (8 - len);
        pInput.push_back(res);
    }
    
    vector<uint8_t> output_data;
    
    size_t orig_size = input->size;
    output_data.push_back((orig_size >> 24) & 0xFF);
    output_data.push_back((orig_size >> 16) & 0xFF);
    output_data.push_back((orig_size >> 8) & 0xFF);
    output_data.push_back(orig_size & 0xFF);
    
    vector<uint8_t> freq_data;
    tree->serializeFreqMap(freq_data);
    
    size_t compressed_size = 6 + freq_data.size() + pInput.size();
    if (compressed_size >= input->size) {
        output_data.push_back(0x01);
        output_data.insert(output_data.end(), input->data, input->data + input->size);
    } else {
        output_data.push_back(0x00);
        output_data.push_back(padding);
        output_data.insert(output_data.end(), freq_data.begin(), freq_data.end());
        output_data.insert(output_data.end(), pInput.begin(), pInput.end());
    }
    
    output->fileWriteBits(output_data, output_src);
}