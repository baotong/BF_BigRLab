#include "rpc_module.h"
#include "AlgMgrService.h"
#include "KnnService.h"
#include "WordAnnDB.h"
#include <iostream>
#include <fstream>
#include <cctype>
#include <thread>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>

using std::cout; using std::cerr; using std::endl;

static boost::shared_ptr<WordAnnDB> g_pWordAnnDB;

static std::string                  g_strAlgMgrAddr;
static uint16_t                     g_nAlgMgrPort = 0;
static std::string                  g_strThisAddr;
static uint16_t                     g_nThisPort = 0;

// args for ann
DEFINE_bool(build, false, "work mode build or nobuild");
DEFINE_string(idata, "", "input data filename");
DEFINE_int32(nfields, 0, "num of fields in input data file.");
DEFINE_int32(ntrees, 0, "num of trees to build.");
DEFINE_string(idx, "", "filename to save the tree.");
DEFINE_string(wt, "", "filename to save words table.");
// args for server
DEFINE_string(algmgr, "", "Address algorithm server manager, in form of addr:port");
DEFINE_string(svraddr, "", "Address of this RPC algorithm server, in form of addr:port");
DEFINE_int32(n_work_threads, 10, "Number of work threads on RPC server");
DEFINE_int32(n_io_threads, 3, "Number of io threads on RPC server");

namespace {

static inline
bool check_above_zero(const char* flagname, gflags::int32 value)
{
    if (value <= 0) {
        cerr << "value of " << flagname << " must be greater than 0" << endl;
        return false;
    } // if
    return true;
}

static inline
bool check_not_empty(const char* flagname, const std::string &value) 
{
    if (value.empty()) {
        cerr << "value of " << flagname << " cannot be empty" << endl;
        return false;
    } // if
    return true;
}

static bool validate_idata(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool idata_dummy = gflags::RegisterFlagValidator(&FLAGS_idata, &validate_idata);

static bool validate_nfields(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool nfields_dummy = gflags::RegisterFlagValidator(&FLAGS_nfields, &validate_nfields);

static bool validate_ntrees(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool ntrees_dummy = gflags::RegisterFlagValidator(&FLAGS_ntrees, &validate_ntrees);

static bool validate_idx(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool idx_dummy = gflags::RegisterFlagValidator(&FLAGS_idx, &validate_idx);

static bool validate_wt(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool wt_dummy = gflags::RegisterFlagValidator(&FLAGS_wt, &validate_wt);

static 
bool validate_algmgr(const char* flagname, const std::string &value) 
{
    using namespace std;

    if (FLAGS_build)
        return true;

    if (!check_not_empty(flagname, value))
        return false;

    string::size_type pos = value.find_last_of(':');
    if (string::npos == pos) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    g_strAlgMgrAddr = value.substr(0, pos);
    if (g_strAlgMgrAddr.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    string strPort = value.substr(pos + 1, string::npos);
    if (strPort.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!boost::conversion::try_lexical_convert(strPort, g_nAlgMgrPort)) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!g_nAlgMgrPort) {
        cerr << "Invalid port number specified by arg " << flagname << endl;
        return false;
    } // if

    return true;
}
static const bool algmgr_dummy = gflags::RegisterFlagValidator(&FLAGS_algmgr, &validate_algmgr);

static 
bool validate_svraddr(const char* flagname, const std::string &value) 
{
    using namespace std;

    if (FLAGS_build)
        return true;

    if (!check_not_empty(flagname, value))
        return false;

    string::size_type pos = value.find_last_of(':');
    if (string::npos == pos) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    g_strThisAddr = value.substr(0, pos);
    if (g_strThisAddr.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    string strPort = value.substr(pos + 1, string::npos);
    if (strPort.empty()) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!boost::conversion::try_lexical_convert(strPort, g_nThisPort)) {
        cerr << "Invalid addr format specified by arg " << flagname << endl;
        return false;
    } // if

    if (!g_nThisPort) {
        cerr << "Invalid port number specified by arg " << flagname << endl;
        return false;
    } // if

    return true;
}
static const bool svraddr_dummy = gflags::RegisterFlagValidator(&FLAGS_svraddr, &validate_svraddr);

static 
bool validate_n_work_threads(const char* flagname, gflags::int32 value) 
{
    if (FLAGS_build)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_work_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_work_threads, &validate_n_work_threads);

static 
bool validate_n_io_threads(const char* flagname, gflags::int32 value) 
{
    if (FLAGS_build)
        return true;
    return check_above_zero(flagname, value);
}
static const bool n_io_threads_dummy = gflags::RegisterFlagValidator(&FLAGS_n_io_threads, &validate_n_io_threads);

} // namespace

namespace KNN {

class KnnServiceHandler : virtual public KnnServiceIf {
public:
    virtual void queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n);
    virtual void queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n);
};

#define THROW_INVALID_REQUEST(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args; \
        InvalidRequest __input_request_err; \
        __input_request_err.reason = std::move(__err_stream.str()); \
        throw __input_request_err; \
    } while (0)

void KnnServiceHandler::queryByItem(std::vector<Result> & _return, 
            const std::string& item, const int32_t n)
{
    using namespace std;

    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value " << n);

    vector<string>    result;
    vector<double>    distances;

    g_pWordAnnDB->kNN_By_Word( item, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

void KnnServiceHandler::queryByVector(std::vector<Result> & _return, 
            const std::vector<double> & values, const int32_t n)
{
    using namespace std;

    if (n <= 0)
        THROW_INVALID_REQUEST("Invalid n value " << n);

    vector<string>    result;
    vector<double>    distances;

    g_pWordAnnDB->kNN_By_Vector( values, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

#undef THROW_INVALID_REQUEST

} // namespace KNN


typedef BigRLab::ThriftClient< BigRLab::AlgMgrServiceClient > AlgMgrClient;
typedef BigRLab::ThriftServer< KNN::KnnServiceIf, KNN::KnnServiceProcessor > KnnAlgServer;
static AlgMgrClient::Pointer    g_pAlgMgrClient;
static KnnAlgServer::Pointer    g_pKnnAlgServer;
static boost::shared_ptr<BigRLab::AlgSvrInfo>  g_pSvrInfo;

static
bool get_local_ip( std::string &result )
{
    using namespace boost::asio::ip;

    try {
        boost::asio::io_service netService;
        boost::asio::io_service::work work(netService);
        
        std::thread ioThr( [&]{ netService.run(); } );

        udp::resolver   resolver(netService);
        udp::resolver::query query(udp::v4(), "www.baidu.com", "");
        udp::resolver::iterator endpoints = resolver.resolve(query);
        udp::endpoint ep = *endpoints;
        udp::socket socket(netService);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
        result = std::move( addr.to_string() );

        netService.stop();
        ioThr.join();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "get_local_ip() error, Exception: " << e.what() << std::endl;
    } // try

    return false;
}


static
void start_rpc_service()
{
    using namespace std;

    // fill svrinfo
    string addr, line;
    if (get_local_ip(addr)) {
        if (addr != g_strThisAddr) {
            cout << "The local addr you provided is " << g_strThisAddr 
                << " which differs with that system detected " << addr
                << " Do you want to use the system detected addr? (y/n)y?" << endl;
            getline(cin, line);
            if (!line.empty() && tolower(line[0]) == 'n' )
                addr = g_strThisAddr;
        } // if
    } else {
        addr = g_strThisAddr;
    } // if

    g_pSvrInfo = boost::make_shared<BigRLab::AlgSvrInfo>();
    g_pSvrInfo->addr = addr;
    g_pSvrInfo->port = (int16_t)g_nThisPort;
    g_pSvrInfo->nWorkThread = FLAGS_n_work_threads;

    // start client to alg_mgr
}

static
void load_data_file( const char *filename )
{
    using namespace std;

    ifstream ifs( filename, ios::in );
    if (!ifs)
        throw runtime_error( string("Cannot open file ").append(filename) );

    string line;
    while (getline(ifs, line)) {
        try {
            g_pWordAnnDB->addRecord( line );

        } catch (const InvalidInput &errInput) {
            // cerr << errInput.what() << endl;
            LOG(ERROR) << errInput.what();
            continue;
        } // try
    } // while
}


static
void do_build_routine()
{
    cout << "Program working in BUILD mode." << endl;
    cout << "Loading data file..." << endl;
    load_data_file( FLAGS_idata.c_str() );
    cout << "Building annoy index..." << endl;
    g_pWordAnnDB->buildIndex( FLAGS_ntrees );
    g_pWordAnnDB->saveIndex( FLAGS_idx.c_str() );
    cout << "Saving word table..." << endl;
    g_pWordAnnDB->saveWordTable( FLAGS_wt.c_str() );

    cout << "Total " << g_pWordAnnDB->size() << " words in database." << endl;
}

static
void do_load_routine()
{
    cout << "Program working in LOAD mode." << endl;
    cout << "Loading annoy index..." << endl;
    g_pWordAnnDB->loadIndex( FLAGS_idx.c_str() );
    cout << "Loading word table..." << endl;
    g_pWordAnnDB->loadWordTable( FLAGS_wt.c_str() );

    cout << "Total " << g_pWordAnnDB->size() << " words in database." << endl;

    start_rpc_service();
}


int main( int argc, char **argv )
{
    using namespace std;

    try {
        google::InitGoogleLogging(argv[0]);
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        g_pWordAnnDB.reset( new WordAnnDB(FLAGS_nfields) );

        if (FLAGS_build)
            do_build_routine();
        else
            do_load_routine();

        // Test::test();
        // Test::test_kNN_By_Vector();
        // Test::handle_cmd();

    } catch (const std::exception &ex) {
        cerr << "main caught exception: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}






/*
 * namespace Test {
 *     void test_annoy_write()
 *     {
 *         using namespace std;
 *         ifstream ifs("vectors_10.txt", ios::in);
 *         string line, word;
 *         int nFields;
 * 
 *         getline( ifs, line );
 *         // getline( ifs, line );
 * 
 *         stringstream str(line);
 *         str >> word;
 *         cout << word << endl;
 * 
 *         vector<float> vec;
 *         read_into_container( str, vec );
 *         nFields = vec.size();
 *         cout << nFields << endl;
 *         Test::print_container( cout, vec );
 * 
 *         AnnoyIndex< uint32_t, float, Angular, RandRandom > aIndex( nFields );
 *         aIndex.add_item( 5, &vec[0] );
 *         aIndex.build( 10 );
 *         aIndex.save( "test.ann" );
 * 
 *         exit(0);
 *     }
 * 
 *     void test_annoy_read( int nFields )
 *     {
 *         using namespace std;
 * 
 *         AnnoyIndex< uint32_t, float, Angular, RandRandom > aIndex( nFields );
 *         aIndex.load( "test.ann" );
 * 
 *         vector<float> vec(nFields);
 *         // id 错了不会改变vec
 *         aIndex.get_item( 4, &vec[0] );
 *         Test::print_container( cout, vec );
 * 
 *         exit(0);
 *     }
 * } // namespace Test
 */

/*
 * static
 * void parse_args( int argc, char **argv )
 * {
 *     auto print_and_exit = [] {
 *         print_usage();
 *         exit(0);
 *     };
 * 
 *     if (argc < 4)
 *         print_and_exit();
 * 
 *     char    *parg = NULL;
 *     char    optc;
 *     int     i;
 * 
 *     if (strcmp(argv[1], "build") == 0) {
 *         g_eRunType = BUILD;
 *         for (i = 2; i < argc;) {
 *             parg = argv[i];
 *             if ( *parg++ != '-' )
 *                 print_and_exit();
 *             optc = *parg;
 *             if (optc == 'i') {
 *                 g_cstrInputDataFile = argv[++i];
 *             } else if (optc == 'f') {
 *                 if (sscanf(argv[++i], "%d", &g_nVecSize) != 1)
 *                     print_and_exit();
 *             } else if (strcmp(parg, "nt") == 0) {
 *                 if (sscanf(argv[++i], "%d", &g_nTrees) != 1)
 *                     print_and_exit();
 *             } else if (strcmp(parg, "oIdx") == 0) {
 *                 g_cstrOutputIdxFile = argv[++i];
 *             } else if (strcmp(parg, "oWt") == 0) {
 *                 g_cstrOutputWordFile = argv[++i];
 *             } else {
 *                 print_and_exit();
 *             } // if
 * 
 *             ++i;
 *         } // for
 * 
 *     } else if (strcmp(argv[1], "load") == 0) {
 *         g_eRunType = LOAD;
 *         for (i = 2; i < argc;) {
 *             parg = argv[i];
 *             if ( *parg++ != '-' )
 *                 print_and_exit();
 *             optc = *parg;
 *             if (strcmp(parg, "iIdx") == 0) {
 *                 g_cstrInputIdxFile = argv[++i];
 *             } else if (strcmp(parg, "iWt") == 0) {
 *                 g_cstrInputWordFile = argv[++i];
 *             } else if (optc == 'f') {
 *                 if (sscanf(argv[++i], "%d", &g_nVecSize) != 1)
 *                     print_and_exit();
 *             } else {
 *                 print_and_exit();
 *             } // if
 * 
 *             ++i;
 *         } // for
 *     } else {
 *         print_and_exit();
 *     } // if
 * }
 */


/*
 * static
 * void check_args()
 * {
 *     using namespace std;
 * 
 *     auto err_exit = [] (const char *msg) {
 *         cerr << msg << endl;
 *         exit(-1);
 *     };
 * 
 *     if (g_eRunType == BUILD) {
 *         if (!g_cstrInputDataFile)
 *             err_exit( "no input data file specified." );
 *         if (g_nVecSize <= 0)
 *             err_exit( "Invalid vector size." );
 *         if (g_nTrees <= 0)
 *             err_exit( "Invalid number of trees." );
 *         if (!g_cstrOutputIdxFile)
 *             err_exit( "no output index file specified." );
 *         if (!g_cstrOutputWordFile)
 *             err_exit( "no output word table file specified." );
 * 
 *     } else if (g_eRunType == LOAD) {
 *         if (!g_cstrInputIdxFile)
 *             err_exit( "no input index file specified." );
 *         if (!g_cstrInputWordFile)
 *             err_exit( "no input word table file specified." );
 *         if (g_nVecSize <= 0)
 *             err_exit( "Invalid vector size." );
 *     } // if
 * }
 */


/*
 * namespace Test {
 * 
 *     template < typename T >
 *     std::ostream& print_container( std::ostream &os, const T &c )
 *     {
 *         typedef typename T::value_type value_type;
 *         std::copy( c.begin(), c.end(), std::ostream_iterator<value_type>(os, " ") );
 *         os << std::endl;
 * 
 *         return os;
 *     }
 * 
 *     void handle_cmd()
 *     {
 *         using namespace std;
 * 
 *         cout << "Input command: " << endl;
 * 
 *         string line, cmd;
 *         while ( getline(cin, line) ) {
 *             stringstream str(line);
 *             str >> cmd;
 *             try {
 *                 if ("query" == cmd) {
 *                     string word;
 *                     vector<float> v;
 *                     str >> word;
 *                     g_pWordAnnDB->getVector(word, v);
 *                     cout << "values of word " << word << ":" << endl;
 *                     print_container(cout, v);
 *                 } else if ("knn" == cmd) {
 *                     string            word;
 *                     size_t            n;
 *                     vector<string>    result;
 *                     vector<float>     distances;
 * 
 *                     str >> word >> n;
 *                     g_pWordAnnDB->kNN_By_Word( word, n, result, distances );
 *                     cout << "got " << result.size() << " results:" << endl;
 *                     for (size_t i = 0; i < result.size(); ++i)
 *                         cout << result[i] << "\t" << distances[i] << endl;
 * 
 *                 }  else if ("dist" == cmd) {
 *                     string w1, w2;
 *                     str >> w1 >> w2;
 *                     float dist = g_pWordAnnDB->getDistance(w1, w2);
 *                     cout << "Distance between " << w1 << " and " << w2 << " is: " << dist << endl;
 * 
 *                 } else if ("quit" == cmd) {
 *                     break;
 *                 } else {
 *                     cout << "Invalid command!" << endl;
 *                     continue;
 *                 } // if
 * 
 *             } catch (const InvalidInput &err) {
 *                 cerr << err.what() << endl;
 *                 continue;
 *             } // try
 *         } // while
 * 
 *         cout << "handle_cmd() Done!" << endl;
 *     }
 * 
 *     void test_kNN_By_Vector()
 *     {
 *         using namespace std;
 * 
 *         size_t              n;
 *         vector<float>       inputVec;
 *         vector<string>      result;
 *         vector<uint32_t>    resultIds;
 *         vector<float>       distances;
 *         string              line;
 * 
 *         // read n
 *         {
 *             getline(cin, line);
 *             stringstream str(line);
 *             str >> n;
 *         }
 * 
 *         // read inputvec
 *         {
 *             getline(cin, line);
 *             stringstream str(line);
 *             read_into_container( str, inputVec );
 *         }
 * 
 * 
 *         g_pWordAnnDB->kNN_By_Vector( inputVec, n, resultIds, distances );
 * 
 *         result.resize( resultIds.size() );
 *         for (std::size_t i = 0; i != resultIds.size(); ++i) {
 *             g_pWordAnnDB->getWordById( resultIds[i], result[i] );
 *             // LOG_IF(WARNING, !ret) << "cannot find word with id " << resultIds[i];
 *         } // for
 * 
 *         cout << "got " << result.size() << " results:" << endl;
 *         for (size_t i = 0; i < result.size(); ++i)
 *             cout << result[i] << "\t" << distances[i] << endl;
 * 
 *         exit(0);
 *     }
 * 
 *     void print_args()
 *     {
 *         using namespace std;
 *         cout << "g_cstrInputDataFile = " << (g_cstrInputDataFile ? g_cstrInputDataFile : "NULL") << endl;
 *         cout << "g_cstrInputIdxFile = " << (g_cstrInputIdxFile ? g_cstrInputIdxFile : "NULL") << endl;
 *         cout << "g_cstrInputWordFile = " << (g_cstrInputWordFile ? g_cstrInputWordFile : "NULL") << endl;
 *         cout << "g_cstrOutputIdxFile = " << (g_cstrOutputIdxFile ? g_cstrOutputIdxFile : "NULL") << endl;
 *         cout << "g_cstrOutputWordFile = " << (g_cstrOutputWordFile ? g_cstrOutputWordFile : "NULL") << endl;
 *         cout << "g_nVecSize = " << g_nVecSize << endl;
 *         cout << "g_nTrees = " << g_nTrees << endl;
 *         cout << "g_eRunType = " << (g_eRunType == BUILD ? "BUILD" : "LOAD") << endl;
 *     }
 * 
 *     void test()
 *     {
 *         using namespace std;
 *         cout << "Total words in database: " << g_pWordAnnDB->size() << endl;    
 *         exit(0);
 *     }
 * 
 * } // namespace Test
 */

/*
 * static inline
 * void print_usage()
 * {
 *     using namespace std;
 * 
 *     cout << "Usage: " << endl;
 *     cout << "For building index from data file:" << endl;
 *     cout << "\t" << "./wordann build -i input_data_file -f n_vector_size "
 *          << "-nt n_trees " << "-oIdx output_index_file " 
 *          << "-oWt output_words_file" << endl;
 *     cout << "For loading index file from previous built:" << endl;
 *     cout << "\t" << "./wordann load -f n_vector_size -iIdx input_index_file " 
 *          << "-iWt input_words_file" << endl;
 * }
 */


