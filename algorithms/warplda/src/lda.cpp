#include <iostream>
#include "lda.hpp"

void LDA::loadBinary(std::string fname)
{
    std::cerr << "DBG LDA::loadBinary() fname = " << fname << std::endl;
    if (!g.Load(fname))
        throw std::runtime_error(std::string("Load Binary failed : ") + fname);
	printf("Bigraph loaded from %s, %u documents, %u unique tokens, %lu total words\n", fname.c_str(), g.NU(), g.NV(), g.NE());
}

void LDA::loadBinary(
                 std::istream &uIdxStream,
                 std::istream &vIdxStream,
                 std::istream &uLnkStream,
                 std::istream &vLnkStream
                )
{
    g.Load(uIdxStream, vIdxStream, uLnkStream, vLnkStream);
}
