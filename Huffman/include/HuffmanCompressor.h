#include "HuffmanTree.h"
#include "Bitstream.h"
#include <map>

using binary_map = map<uint8_t, Bitstream>;


class HuffmanCompress
{
 public:
    binary_map b_map;
    Bitstream bitstream;
    unique_ptr<HuffmanTree> tree = make_unique<HuffmanTree>();
    FileProcess* output = new FileProcess();
    

    void encode(FileProcess* input, string output_src);
    void decode(FileProcess* input, string output_src);
    void createBinMap(Bitstream bits, const std::shared_ptr<HuffmanNode>& cur);
    vector<uint8_t> sequence(FileProcess* input);
    
    void evaluateEfficency(FileProcess* input, FileProcess* output, clock_t start_time);
    ~HuffmanCompress()
    {
      delete output;
    }
    

};