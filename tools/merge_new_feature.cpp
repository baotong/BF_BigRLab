#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cstdio>
#include <glog/logging.h>

using namespace std;

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

static
uint32_t get_max_leaf_id( const char *filename, std::size_t &totalLine )
{
    string line, item;

    uint32_t maxIdx = 0, idx = 0;
    double value = 0.0;
    size_t lineno = 0, maxIdxLineno = 0;

    ifstream ifs( filename, ios::in );
    if (!ifs)
        THROW_RUNTIME_ERROR( "Cannot open file " << filename );

    while (getline(ifs, line)) {
        ++lineno;
        stringstream stream(line);
        while( stream >> item ) {
            if (sscanf(item.c_str(), "%u:%lf", &idx, &value) != 2)
                continue;
            if (idx > maxIdx) {
                maxIdx = idx;
                maxIdxLineno = lineno;
            } // if
        } // while
    } // while

    // DLOG(INFO) << "Found max idx in line " << maxIdxLineno << " maxIdx = " << maxIdx;
    
    totalLine = lineno;

    return maxIdx;
}

static
void merge_new_feature( const char *oldFile, const char *newFeatureFile,
                    const char *outFile, const uint32_t idxOffset,
                    const std::size_t totalLine )
{
    ifstream fOld( oldFile, ios::in );
    if (!fOld)
        THROW_RUNTIME_ERROR( "Cannot open file " << oldFile );
    ifstream fNew( newFeatureFile, ios::in );
    if (!fNew)
        THROW_RUNTIME_ERROR( "Cannot open file " << newFeatureFile );
    ofstream fOut( outFile, ios::out );
    if (!fOut)
        THROW_RUNTIME_ERROR( "Cannot open file " << outFile );

    string line, lineNew, item;
    float value = 0.0;
    size_t lineno = 0;

    while ( getline(fOld, line) && getline(fNew, lineNew) ) {
        ++lineno;
        stringstream stream;
        stream << line;
        stringstream streamNew( lineNew );
        uint32_t idx = 0;
        while (streamNew >> value) {
            stream << " " << (idx + idxOffset) << ":" << value;
            ++idx;
        } // while
        stream << flush;
        fOut << stream.str() << endl;
    } // while

    if (lineno != totalLine)
        THROW_RUNTIME_ERROR("Line number mismatch! " << totalLine << " lines in " 
            << oldFile << ", but " << lineno << " lines in " << newFeatureFile);
}


int main( int argc, char **argv )
{
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " train_data new_feature_data new_train_data" << endl;
        exit(-1);
    } // if

    try {
        google::InitGoogleLogging(argv[0]);

        size_t totalLine = 0;
        uint32_t maxLeafIdx = get_max_leaf_id( argv[1], totalLine );

        if (totalLine)
            merge_new_feature( argv[1], argv[2], argv[3], maxLeafIdx+1, totalLine );

        cout << "All job done!" << endl;

    } catch (const std::exception &ex) {
        cerr << "Exception caught in main: " << ex.what() << endl;
    } // try


    return 0;
}

