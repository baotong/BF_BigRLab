#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

using namespace std;

int main(int argc, char **argv)
try {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " article_file score_file out_file" << endl;
        exit(0);
    } // if

    string line1, line2;
    ifstream in1(argv[1], ios::in);
    if (!in1)
        THROW_RUNTIME_ERROR("Cannot open " << argv[1]);
    ifstream in2(argv[2], ios::in);
    if (!in2)
        THROW_RUNTIME_ERROR("Cannot open " << argv[2]);
    ofstream ofs(argv[3], ios::out);
    if (!ofs)
        THROW_RUNTIME_ERROR("Cannot open " << argv[3] << " for writting");

    while (getline(in1, line1) && getline(in2, line2)) {
        ofs << line2 << "\t" << line1 << endl;
    } // while

    if (getline(in1, line1))
        THROW_RUNTIME_ERROR("line number mismatch in files " << argv[1] << " and " << argv[2]);
    if (getline(in2, line2))
        THROW_RUNTIME_ERROR("line number mismatch in files " << argv[1] << " and " << argv[2]);

    return 0;

} catch (const std::exception &ex) {
    cerr << ex.what() << endl;
}

