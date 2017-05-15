/*
 * c++ -o /tmp/test article2vec.cpp -lglog -lgflags -std=c++11 -pthread -g
 *
 * GLOG_log_dir="." ./article2vec.bin -method wordvec -input tagresult.txt -ref vectors.txt -output article_vectors.txt
 * GLOG_log_dir="." ./article2vec.bin -method cluster -input tagresult.txt -ref classes.txt -output article_vectors.txt -hasid true
 *
 * For DEBUG cluster
 * GLOG_logtostderr=1 ./article2vec.bin -method cluster -input test.input -ref classes.txt -nclasses 500 -output article_vectors.txt
 * 宝宝 的 孩子 做 好 孩子
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <exception>
#include <algorithm>
#include <iterator>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <cmath>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "CommDef.h"
#include "utils/read_cmd.h"


template< typename StreamType >
bool bad_stream( const StreamType &stream )
{ return (stream.fail() || stream.bad()); }


enum MethodType {
    UNSPECIFIED,
    WORD_VECTOR,
    WORD_CLUSTER,
    WORD_TFIDF
};

static MethodType g_eMethod = UNSPECIFIED;


DEFINE_int32(nclasses, 0, "number of classes");
DEFINE_string(method, "", "wordvec, cluster, or tfidf");
DEFINE_string(input, "", "segmented corpus filename");
DEFINE_string(ref, "", "ref file contains word vectors or word clusterids or word idf");
DEFINE_string(output, "", "file to keep the result");
DEFINE_bool(hasid, false, "Whether has id or not");

namespace {

using namespace std;

static
bool validate_string_args(const char *flagname, const std::string &value)
{
    RET_MSG_VAL_IF(value.empty(), false, "You have to specify " << flagname);
    return true;
}
static bool input_dummy = gflags::RegisterFlagValidator(&FLAGS_input, &validate_string_args);
static bool wordvec_dummy = gflags::RegisterFlagValidator(&FLAGS_ref, &validate_string_args);
static bool output_dummy = gflags::RegisterFlagValidator(&FLAGS_output, &validate_string_args);

static
bool validate_method(const char *flagname, const std::string &value)
{
    RET_MSG_VAL_IF(!validate_string_args(flagname, value), false, "");
    if (value == "wordvec")
        g_eMethod = WORD_VECTOR;
    else if (value == "cluster")
        g_eMethod = WORD_CLUSTER;
    else if (value == "tfidf")
        g_eMethod = WORD_TFIDF;
    else
        RET_MSG_VAL(false, "Invalid method arg!");
    return true;
}
static bool method_dummy = gflags::RegisterFlagValidator(&FLAGS_method, &validate_method);


boost::shared_ptr<std::istream> open_input()
{
    auto deleteIfs = [](std::istream *p) {
        // DLOG_IF(INFO, p == &cin) << "Not delete cin";
        // DLOG_IF(INFO, p && p != &cin) << "Deleting ordinary in file obj";
        if (p && p != &cin)
            delete p;
    };

    boost::shared_ptr<std::istream> ifs(nullptr, deleteIfs);
    if (FLAGS_input == "-") {
        ifs.reset( &cin, deleteIfs );
    } else {
        ifs.reset( new ifstream(FLAGS_input, ios::in), deleteIfs );
        THROW_RUNTIME_ERROR_IF(!(*ifs), "Cannot open " << FLAGS_input << " for reading!");
    } // if

    return ifs;
}

boost::shared_ptr<std::ostream> open_output()
{
    auto deleteOfs = [](std::ostream *p) {
        // DLOG_IF(INFO, p == &cout) << "Not delete cout";
        // DLOG_IF(INFO, p && p != &cout) << "Deleting ordinary out file obj";
        if (p && p != &cout)
            delete p;
    };

    boost::shared_ptr<std::ostream> ofs(nullptr, deleteOfs);
    if (FLAGS_output == "-") {
        ofs.reset( &cout, deleteOfs );
    } else {
        ofs.reset( new ofstream(FLAGS_output, ios::out), deleteOfs );
        THROW_RUNTIME_ERROR_IF(!(*ofs), "Cannot open " << FLAGS_output << " for writting!");
    } // if

    return ofs;
}

} // namespace


typedef std::map< std::string, std::vector<float> >     WordVecTable;
typedef std::map< std::string, uint32_t >               WordClusterTable;


namespace Test {

using namespace std;

void test_file()
{
    DLOG(INFO) << "test_file() " << FLAGS_output;
    // boost::shared_ptr<ostream> ofs = open_output();
    boost::shared_ptr<istream> ifs = open_input();
    return;
}

void print_cluster_table( const WordClusterTable &table )
{
    for (const auto &v : table)
        cout << v.first << " " << v.second << endl;
}

void print_wordvec_table( const WordVecTable &table )
{
    for (const auto &v : table) {
        cout << v.first << " ";
        for (const auto &value : v.second)
            cout << value << " ";
        cout << endl;
    } // for
}

template < typename T >
ostream& print_non_zero_vector( ostream &os, const T &c )
{
    for (size_t i = 0; i < c.size(); ++i)
        if (fabs(c[i] - 0.0) > 1e-9)
            os << i << ":" << c[i] << endl;
    return os;
}

} // namespace Test

template < typename T >
std::ostream& print_container( std::ostream &os, const T &c )
{
    typedef typename T::value_type Type;
    copy( c.begin(), c.end(), ostream_iterator<Type>(os, " ") );
    os << endl;
    return os;
}


static
void do_with_wordvec( WordVecTable &wordVecTable )
{
    using namespace std;

    boost::shared_ptr<istream> ifs = open_input();
    boost::shared_ptr<ostream> ofs = open_output();

    auto processLine = [&](const string &line, vector<float> &result) {
        typedef boost::tuple<double&, float&> IterType;

        result.clear();
        result.resize(FLAGS_nclasses, 0.0);

        vector<double> sumVec( FLAGS_nclasses );
        string word;

        stringstream stream(line);
        size_t count = 0;
        while (stream >> word) {
            auto it = wordVecTable.find(word);
            if (it == wordVecTable.end()) {
                // DLOG(INFO) << "no word " << word << " found in wordvec table.";
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

    // auto writeToFile = [&](const vector<float> &vec) {
        // std::copy(vec.begin(), vec.end(), ostream_iterator<float>(ofs, " "));
        // ofs << endl;
    // };

    // 重复单词重复统计 yes
    // 若出现vector表中没有的单词，不计入求均值的size分母 yes
    // 空行直接跳过，还是返回全0? yes
    string line, id;
    vector<float> result;
    result.reserve(FLAGS_nclasses);
    while (getline(*ifs, line)) {
        if (FLAGS_hasid) {
            istringstream iss(line);
            iss >> id;
            getline(iss, line);
        } // if
        boost::trim(line);
        processLine(line, result);
        if (FLAGS_hasid) 
            *ofs << id << "\t";
        print_container(*ofs, result);
    } // while
}

static
void do_with_cluster(WordClusterTable &clusterTable)
{
    using namespace std;

    boost::shared_ptr<istream> ifs = open_input();
    boost::shared_ptr<ostream> ofs = open_output();

    auto processLine = [&](const string &line, vector<float> &result) {
        typedef boost::tuple<double&, float&> IterType;

        result.clear();
        result.resize(FLAGS_nclasses, 0.0);

        vector<double> workVec( FLAGS_nclasses, 0.0 );
        set<string>    wordSet;
        string         word;
        stringstream   stream(line);
        double         maxCount = 0.0;

        while (stream >> word) {
            auto ret = wordSet.insert(word);
            // 跳过重复单词
            if (!ret.second)
                continue;

            auto it = clusterTable.find(word);
            if (it == clusterTable.end()) {
                // DLOG(INFO) << "no word " << word << " found in cluster table.";
                continue;
            } // if
            uint32_t id = it->second;
            workVec[id] += 1.0;
            maxCount = workVec[id] > maxCount ? workVec[id] : maxCount;
        } // while

        if (maxCount < 1.0)
            return;

        // DLOG(INFO) << "Before normalization:";
        // Test::print_non_zero_vector(cout, workVec);
        std::for_each(workVec.begin(), workVec.end(), 
                [&](double &v){ v /= maxCount; });
        // DLOG(INFO) << "After normalization:";
        // Test::print_non_zero_vector(cout, workVec);

        BOOST_FOREACH( IterType v, boost::combine(workVec, result) )
            v.get<1>() = (float)(v.get<0>());
    };

    string line, id;
    vector<float> result;
    result.reserve(FLAGS_nclasses);
    while (getline(*ifs, line)) {
        if (FLAGS_hasid) {
            istringstream iss(line);
            iss >> id;
            getline(iss, line);
        } // if
        boost::trim(line);
        processLine(line, result);
        if (FLAGS_hasid) 
            *ofs << id << "\t";
        print_container(*ofs, result);
    } // while
}

static
void load_wordvec( WordVecTable &wordVecTable )
{
    using namespace std;

    ifstream ifs(FLAGS_ref, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "load_wordvec() cannot open " << FLAGS_ref);

    string line, word;
    size_t lineno = 0;
    while (getline(ifs, line)) {
        ++lineno;
        stringstream stream(line);
        vector<float> vec;
        vec.reserve(FLAGS_nclasses);
        stream >> word;
        if (bad_stream(stream)) {
            LOG(ERROR) << "bad stream error when reading line " << lineno
                    << " " << line;
            continue;
        } // if
        copy(istream_iterator<float>(stream), istream_iterator<float>(), 
                    back_inserter(vec));
        if (vec.size() != (size_t)FLAGS_nclasses) {
            LOG(ERROR) << "Invalid vector len when reading line " << lineno
                    << " " << line;
            continue;
        } // if
        auto ret = wordVecTable.insert(std::make_pair(word, WordVecTable::mapped_type()));
        if (!ret.second) {
            LOG(ERROR) << "Duplicate word " << word << " when reading line " << lineno;
            continue;
        } // if
        ret.first->second.swap(vec);
    } // while
}

static
void load_cluster(WordClusterTable &clusterTable)
{
    using namespace std;
    
    ifstream ifs(FLAGS_ref, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "load_wordvec() cannot open " << FLAGS_ref);

    string line, word;
    uint32_t clusterId;
    size_t lineno = 0;
    while (getline(ifs, line)) {
        ++lineno;
        stringstream stream(line);
        stream >> word >> clusterId;
        if (clusterId >= (size_t)FLAGS_nclasses) {
            LOG(ERROR) << clusterId << " read in line:" << lineno << " is not valid";
            continue;
        } // if
        auto ret = clusterTable.insert(std::make_pair(word, clusterId));
        if (!ret.second) {
            LOG(ERROR) << "Duplicate word " << word << " when reading line " << lineno;
            continue;
        } // if
    } // while
}


static
int get_nclasses()
{
    using namespace std;

    int nclasses = 0;
    string output;

    if (WORD_VECTOR == g_eMethod) {
        ostringstream oss;
        oss << "tail -1 " << FLAGS_ref << " | awk \'{print NF}\'" << flush;
        int retcode = Utils::read_cmd(oss.str(), output);
        THROW_RUNTIME_ERROR_IF(retcode, "get_nclasses() Parse ref file fail!");

        istringstream iss(output);
        iss >> nclasses;
        THROW_RUNTIME_ERROR_IF(!iss, "Read nclasses fail!");

        --nclasses;
        LOG(INFO) << "Detected nclasses = " << nclasses << " from file " << FLAGS_ref;

    } else if (WORD_CLUSTER == g_eMethod) {
        ostringstream oss;
        oss << "cat " << FLAGS_ref << " | awk \'{print $2}\' | sort -nr | head -1" << flush;
        int retcode = Utils::read_cmd(oss.str(), output);
        THROW_RUNTIME_ERROR_IF(retcode, "get_nclasses() Parse ref file fail!");
        
        istringstream iss(output);
        iss >> nclasses;
        THROW_RUNTIME_ERROR_IF(!iss, "Read nclasses fail!");

        ++nclasses;
        LOG(INFO) << "Detected nclasses = " << nclasses << " from file " << FLAGS_ref;
    } // if

    return nclasses;
}


namespace Test {
using namespace std;
void test_get_nclasses()
{
    FLAGS_ref = "../data/rnn_clusterID_index.txt";
    g_eMethod = WORD_CLUSTER;
    FLAGS_nclasses = get_nclasses();
    cout << "nclasses = " << FLAGS_nclasses << endl;
}
    
} // namespace Test


int main(int argc, char **argv)
{
    using namespace std;
    
    // Test::test_get_nclasses();
    // exit(0);

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    auto _do_with_wordvec = [] {
        WordVecTable wordVecTable;
        load_wordvec(wordVecTable);
        DLOG(INFO) << "Totally " << wordVecTable.size() << " word vector items loaded.";
        // Test::print_wordvec_table( wordVecTable );

        RET_MSG_IF(!wordVecTable.size(), "No valid word vector read, terminating!");
        do_with_wordvec( wordVecTable );
    };

    auto _do_with_cluster = [] {
        WordClusterTable clusterTable;
        load_cluster( clusterTable );
        DLOG(INFO) << "Totally " << clusterTable.size() << " cluster items loaded.";
        // Test::print_cluster_table( clusterTable );

        RET_MSG_IF(!clusterTable.size(), "No valid word vector read, terminating!");
        do_with_cluster( clusterTable );
    };

    try {
        FLAGS_nclasses = get_nclasses();
        switch (g_eMethod) {
        case WORD_VECTOR:
            _do_with_wordvec();
            break;
        case WORD_CLUSTER:
            _do_with_cluster();
            break;
        default:
            cerr << "method not set!" << endl;
        } // switch

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
    } // try

    return 0;
}

