/*
 * GLOG_logtostderr=1 ./article2vec.bin -input tagresult.txt -wordvec vectors.txt -veclen 500 -output article_vectors.txt
 * GLOG_log_dir="." ./article2vec.bin -input tagresult.txt -wordvec vectors.txt -veclen 500 -output article_vectors.txt
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <exception>
#include <algorithm>
#include <iterator>
#include <memory>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <glog/logging.h>
#include <gflags/gflags.h>

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

#define ERR_RET_VAL(val, args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        __err_stream.flush(); \
        std::cerr << __err_stream.str() << std::endl; \
        return val; \
    } while (0)

#define COND_RET_VAL(cond, val, args) \
    do { \
        if (cond) ERR_RET_VAL(val, args); \
    } while (0)

template< typename StreamType >
bool bad_stream( const StreamType &stream )
{ return (stream.fail() || stream.bad()); }


DEFINE_int32(veclen, 0, "vector dimension");
DEFINE_string(input, "", "segmented corpus filename");
DEFINE_string(wordvec, "", "file contains word vectors");
DEFINE_string(output, "", "file to keep the result");

namespace {

using namespace std;

static
bool validate_veclen(const char *flagname, gflags::int32 value)
{
    COND_RET_VAL(value <= 0, false, "Invalid value for " << flagname);
    return true;
}
static bool veclen_dummy = gflags::RegisterFlagValidator(&FLAGS_veclen, &validate_veclen);

static
bool validate_string_args(const char *flagname, const std::string &value)
{
    COND_RET_VAL(value.empty(), false, "You have to specify " << flagname);
    return true;
}
static bool input_dummy = gflags::RegisterFlagValidator(&FLAGS_input, &validate_string_args);
static bool wordvec_dummy = gflags::RegisterFlagValidator(&FLAGS_wordvec, &validate_string_args);
static bool output_dummy = gflags::RegisterFlagValidator(&FLAGS_output, &validate_string_args);

} // namespace


typedef std::map< std::string, std::vector<float> >     WordVecTable;
WordVecTable g_WordVector;

namespace Test {

using namespace std;

void print_wordvec_table()
{
    for (const auto &v : g_WordVector) {
        cout << v.first << " ";
        for (const auto &value : v.second)
            cout << value << " ";
        cout << endl;
    } // for
}

template < typename T >
void print_container( const T &c )
{
    typedef typename T::value_type Type;
    copy( c.begin(), c.end(), ostream_iterator<Type>(cout, " ") );
    cout << endl;
}

} // namespace Test

static
void ariticle2vector()
{
    using namespace std;

    ifstream ifs(FLAGS_input, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("ariticle2vector() cannot open " << FLAGS_input << " for reading!");
    ofstream ofs(FLAGS_output, ios::out);
    if (!ofs)
        THROW_RUNTIME_ERROR("ariticle2vector() cannot open " << FLAGS_output << " for writting!");

    auto processLine = [](const string &line, vector<float> &result) {
        typedef boost::tuple<double&, float&> IterType;

        result.clear();
        result.resize(FLAGS_veclen, 0.0);

        vector<double> sumVec( FLAGS_veclen );
        string word;

        stringstream stream(line);
        size_t count = 0;
        while (stream >> word) {
            auto it = g_WordVector.find(word);
            if (it == g_WordVector.end()) {
                DLOG(INFO) << "no word " << word << " found in wordvec table.";
                continue;
            } // if
            ++count;
            vector<float> &wordVec = it->second;
            // Test::print_container(wordVec);
            BOOST_FOREACH( IterType v, boost::combine(sumVec, wordVec) )
                v.get<0>() += v.get<1>();
        } // while

        if (!count)
            return;

        std::for_each(sumVec.begin(), sumVec.end(), 
                [&](double &v){ v /= (double)count; });

        BOOST_FOREACH( IterType v, boost::combine(sumVec, result) )
            v.get<1>() = (float)(v.get<0>());
        // Test::print_container(result);
    };

    auto writeToFile = [&](const vector<float> &vec) {
        std::copy(vec.begin(), vec.end(), ostream_iterator<float>(ofs, " "));
        ofs << endl;
    };

    // 重复单词重复统计 yes
    // 若出现vector表中没有的单词，不计入求均值的size分母 yes
    // 空行直接跳过，还是返回全0? yes
    string line;
    vector<float> result;
    result.reserve(FLAGS_veclen);
    while (getline(ifs, line)) {
        boost::trim(line);
        processLine(line, result);
        writeToFile(result);
    } // while
}

static
void load_wordvec()
{
    using namespace std;

    ifstream ifs(FLAGS_wordvec, ios::in);

    if (!ifs)
        THROW_RUNTIME_ERROR("load_wordvec() cannot open " << FLAGS_wordvec);

    string line, word;
    size_t lineno = 0;
    while (getline(ifs, line)) {
        ++lineno;
        stringstream stream(line);
        vector<float> vec;
        vec.reserve(FLAGS_veclen);
        stream >> word;
        if (bad_stream(stream)) {
            LOG(ERROR) << "bad stream error when reading line " << lineno
                    << " " << line;
            continue;
        } // if
        copy(istream_iterator<float>(stream), istream_iterator<float>(), 
                    back_inserter(vec));
        if (vec.size() != FLAGS_veclen) {
            LOG(ERROR) << "Invalid vector len when reading line " << lineno
                    << " " << line;
            continue;
        } // if
        auto ret = g_WordVector.insert(std::make_pair(word, WordVecTable::mapped_type()));
        if (!ret.second) {
            LOG(ERROR) << "Duplicate word " << word << " when reading line " << lineno;
            continue;
        } // if
        ret.first->second.swap(vec);
    } // while
}


int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        load_wordvec();
        DLOG(INFO) << "Totally " << g_WordVector.size() << " word vector items loaded.";
        // Test::print_wordvec_table();

        if (!g_WordVector.size())
            ERR_RET_VAL(-1, "No valid word vector read, terminating!");

        ariticle2vector();

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
    } // try

    return 0;
}

