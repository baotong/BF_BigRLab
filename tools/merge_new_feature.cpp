/*
 * c++ -o merge merge_new_feature.cpp -lglog -lgflags -std=c++11 -g -O3
 */
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cstdio>
#include <glog/logging.h>
#include <gflags/gflags.h>

using namespace std;

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)


DEFINE_string(req, "", "request name like \"maxidx\", \"mrege\", \"merge_gbdt\".");
DEFINE_string(src, "", "source file to read.");
DEFINE_string(newf, "", "new feature file for merging.");
DEFINE_string(dst, "", "dest file to write.");
DEFINE_int64(offset, -1, "offset idx");


static
uint32_t get_max_idx( const char *filename, std::size_t &totalLine )
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
                    const char *outFile, const uint32_t idxOffset )
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

    while ( getline(fOld, line) && getline(fNew, lineNew) ) {
        stringstream stream;
        stream << line;
        stringstream streamNew( lineNew );
        uint32_t idx = 0;
        while (streamNew >> value) {
            if (value != 0.0)
                stream << " " << (idx + idxOffset) << ":" << value;
            ++idx;
        } // while
        stream << flush;
        fOut << stream.str() << endl;
    } // while

    if (getline(fOld, line))
        THROW_RUNTIME_ERROR("File record size not match in " << oldFile << " and " << newFeatureFile);
    if (getline(fNew, lineNew))
        THROW_RUNTIME_ERROR("File record size not match in " << oldFile << " and " << newFeatureFile);

    cout << "Merge done!" << endl;
}

static
void merge_gbdt( const char *oldFile, const char *newFeatureFile,
                    const char *outFile, const uint32_t idxOffset )
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
    uint32_t value = 0;

    while ( getline(fOld, line) && getline(fNew, lineNew) ) {
        stringstream stream;
        stream << line;
        stringstream streamNew( lineNew );
        while (streamNew >> value) {
            stream << " " << (value + idxOffset) << ":1";
        } // while
        stream << flush;
        fOut << stream.str() << endl;
    } // while

    if (getline(fOld, line))
        THROW_RUNTIME_ERROR("File record size not match in " << oldFile << " and " << newFeatureFile);
    if (getline(fNew, lineNew))
        THROW_RUNTIME_ERROR("File record size not match in " << oldFile << " and " << newFeatureFile);

    cout << "Merge done!" << endl;
}

static
void gbdt_new( const char *oldFile, const char *newFeatureFile,
                    const char *outFile )
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
    char label[1024];
    uint32_t value = 0;

    while ( getline(fOld, line) && getline(fNew, lineNew) ) {
        if (sscanf( line.c_str(), "%s", label ) != 1 )
            THROW_RUNTIME_ERROR("Cannot read label!");
        stringstream stream;
        stream << label;
        stringstream streamNew( lineNew );
        while (streamNew >> value) {
            stream << " " << value << ":1";
        } // while
        stream << flush;
        fOut << stream.str() << endl;
    } // while

    if (getline(fOld, line))
        THROW_RUNTIME_ERROR("File record size not match in " << oldFile << " and " << newFeatureFile);
    if (getline(fNew, lineNew))
        THROW_RUNTIME_ERROR("File record size not match in " << oldFile << " and " << newFeatureFile);

    cout << "Merge done!" << endl;
}


int main( int argc, char **argv )
{
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        if (FLAGS_req == "maxidx") {
            if (FLAGS_src.empty())
                THROW_RUNTIME_ERROR("src file not specified!");
            size_t totalLine = 0;
            uint32_t maxIdx = get_max_idx( FLAGS_src.c_str(), totalLine );
            cout << "Total " << totalLine << " lines in file " << FLAGS_src 
                << ", max index is: " << maxIdx << endl;
        } else if (FLAGS_req == "merge") {
            if (FLAGS_src.empty())
                THROW_RUNTIME_ERROR("src file not specified!");
            if (FLAGS_newf.empty())
                THROW_RUNTIME_ERROR("new feature file not specified!");
            if (FLAGS_dst.empty())
                THROW_RUNTIME_ERROR("dst file not specified!");
            if (FLAGS_offset <= 0)
                THROW_RUNTIME_ERROR("invalid offset value!");
            merge_new_feature( FLAGS_src.c_str(), FLAGS_newf.c_str(), FLAGS_dst.c_str(),
                                (uint32_t)FLAGS_offset );
        } else if (FLAGS_req == "merge_gbdt") {
            if (FLAGS_src.empty())
                THROW_RUNTIME_ERROR("src file not specified!");
            if (FLAGS_newf.empty())
                THROW_RUNTIME_ERROR("new feature file not specified!");
            if (FLAGS_dst.empty())
                THROW_RUNTIME_ERROR("dst file not specified!");
            if (FLAGS_offset <= 0)
                THROW_RUNTIME_ERROR("invalid offset value!");
            merge_gbdt( FLAGS_src.c_str(), FLAGS_newf.c_str(), FLAGS_dst.c_str(),
                                (uint32_t)FLAGS_offset );
        } else if (FLAGS_req == "gbdt_new") {
            if (FLAGS_src.empty())
                THROW_RUNTIME_ERROR("src file not specified!");
            if (FLAGS_newf.empty())
                THROW_RUNTIME_ERROR("new feature file not specified!");
            if (FLAGS_dst.empty())
                THROW_RUNTIME_ERROR("dst file not specified!");
            gbdt_new( FLAGS_src.c_str(), FLAGS_newf.c_str(), FLAGS_dst.c_str() );
        } else {
            cerr << "Invalid request!" << endl;
        } // if

    } catch (const std::exception &ex) {
        cerr << "Exception caught in main: " << ex.what() << endl;
    } // try


    return 0;
}

