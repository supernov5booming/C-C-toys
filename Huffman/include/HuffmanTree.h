#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include <queue>
#include <memory>
#include "FileProcess.h"

using namespace std;

struct HuffmanNode {
    uint32_t freq;
    uint8_t s;
    shared_ptr<HuffmanNode> left;
    shared_ptr<HuffmanNode> right;

    HuffmanNode(uint32_t f, uint8_t ss) 
        : freq(f), s(ss), left(nullptr), right(nullptr) {}
};

struct HuffmanCmp {
    bool operator()(const shared_ptr<HuffmanNode>& h1, const shared_ptr<HuffmanNode>& h2) {
        if (h1->freq != h2->freq) {
            return h1->freq > h2->freq;
        }
        return h1->s > h2->s;
    }
};

using pnode = priority_queue<shared_ptr<HuffmanNode>, vector<shared_ptr<HuffmanNode>>, HuffmanCmp>;
using freq_map = map<uint8_t, int>;

class HuffmanTree {
public:
    freq_map f_m;
    pnode pNode;       
    shared_ptr<HuffmanNode> head;
    size_t original_size;

    HuffmanTree() = default;  

    void createFreqMap(FileProcess* file);
    void pushNode();
    void build(FileProcess* file);
    
    void serializeFreqMap(std::vector<uint8_t>& out);
    void deserializeFreqMap(const uint8_t* data, size_t& pos);
    void buildFromFreqMap();
};