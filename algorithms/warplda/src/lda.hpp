#pragma once

#include <string>
#include <iostream>
#include <utility>
#include "Bigraph.hpp"

class LDA
{
protected:
    Bigraph g;
public:
    LDA() {}
    virtual void loadBinary(std::string prefix);
    virtual void loadBinary(
                 std::istream &uIdxStream,
                 std::istream &vIdxStream,
                 std::istream &uLnkStream,
                 std::istream &vLnkStream
             );
    virtual void estimate(int K, float alpha, float beta, int niter, int perplexity_interval) = 0;
    virtual void inference(int niter, int perplexity_interval) = 0;
    virtual void loadModel(std::string prefix) = 0;
    virtual void storeModel(std::string prefix) = 0;
    virtual void loadZ(std::string prefix) = 0;
    virtual void storeZ(std::string prefix) = 0;
    virtual void storeZ(std::ostream &os) = 0;
    virtual void storeZ(std::vector<std::pair<uint32_t, uint32_t>> &result) = 0;
    virtual void writeInfo(std::string vocab, std::string info, uint32_t ntop) = 0;
};

template <unsigned MH>
class WarpLDA;
