#include<cstdint>
#include<string>
#include<vector>
class Bitstream{
public:
    std::string bits;
    uint32_t size;
    Bitstream():bits(""),size(0){};
    
    void append(const char& c);
    void pop_back();
    std::string getbits();
};
