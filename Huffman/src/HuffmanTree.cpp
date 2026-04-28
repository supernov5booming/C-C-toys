#include "HuffmanTree.h"
#include <iostream>
using namespace std;

void HuffmanTree::createFreqMap(FileProcess* file)
{
    if (!file || file->size == 0) return;

    f_m.clear();
    for (int i = 0; i < file->size; i++)
    {
        uint8_t ch = (uint8_t)file->data[i];
        f_m[ch]++;
    }
}

void HuffmanTree::pushNode()
{
    for (auto& pair : f_m)
    {
        auto node = make_shared<HuffmanNode>(pair.second, pair.first);
        pNode.emplace(node);
    }
}

void HuffmanTree::build(FileProcess* file)
{
    if (!file || file->size == 0) {
        cerr << "File is empty, cannot build Huffman tree" << endl;
        return;
    }

    createFreqMap(file);
    pushNode();

    while (pNode.size() > 1)
    {
        auto node1 = pNode.top();
        pNode.pop();

        auto node2 = pNode.top();
        pNode.pop();

        auto new_node = make_shared<HuffmanNode>(node1->freq + node2->freq, 0);

        new_node->left = node1;
        new_node->right = node2;

        pNode.emplace(new_node);
    }

    head = pNode.top();
    pNode.pop();
}

void HuffmanTree::serializeFreqMap(std::vector<uint8_t>& out)
{
    uint16_t count = static_cast<uint16_t>(f_m.size());
    out.push_back((count >> 8) & 0xFF);
    out.push_back(count & 0xFF);
    
    for (auto& pair : f_m) {
        out.push_back(pair.first);
        uint16_t freq = static_cast<uint16_t>(pair.second);
        out.push_back((freq >> 8) & 0xFF);
        out.push_back(freq & 0xFF);
    }
}

void HuffmanTree::deserializeFreqMap(const uint8_t* data, size_t& pos)
{
    f_m.clear();
    
    uint16_t count = (data[pos] << 8) | data[pos+1];
    pos += 2;
    
    for (uint16_t i = 0; i < count; i++) {
        uint8_t ch = data[pos++];
        uint16_t freq = (data[pos] << 8) | data[pos+1];
        pos += 2;
        f_m[ch] = freq;
    }
}

void HuffmanTree::buildFromFreqMap()
{
    while (!pNode.empty()) {
        pNode.pop();
    }
    
    for (auto& pair : f_m) {
        auto node = make_shared<HuffmanNode>(pair.second, pair.first);
        pNode.emplace(node);
    }

    while (pNode.size() > 1) {
        auto node1 = pNode.top();
        pNode.pop();

        auto node2 = pNode.top();
        pNode.pop();

        auto new_node = make_shared<HuffmanNode>(node1->freq + node2->freq, 0);
        new_node->left = node1;
        new_node->right = node2;

        pNode.emplace(new_node);
    }

    if (!pNode.empty()) {
        head = pNode.top();
        pNode.pop();
    }
}