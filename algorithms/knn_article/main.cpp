#include "AnnDB.h"
#include <fstream>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <boost/shared_ptr.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>

DEFINE_bool(build, false, "work mode build or nobuild");
DEFINE_string(input, "", "input data filename");
DEFINE_int32(nfields, 0, "num of fields in input data file.");
DEFINE_int32(ntrees, 0, "num of trees to build.");
DEFINE_string(idx, "", "filename to save the tree.");

namespace {

using namespace std;

static inline
bool check_above_zero(const char* flagname, gflags::int32 value)
{
    COND_RET_VAL(value <= 0, false, "value of " << flagname << " must be greater than 0");
    return true;
}

static inline
bool check_not_empty(const char *flagname, const std::string &value)
{
    COND_RET_VAL(value.empty(), false, "You have to specify " << flagname);
    return true;
}

static bool validate_input(const char* flagname, const std::string &value) 
{ 
    if (!FLAGS_build)
        return true;
    return check_not_empty(flagname, value); 
}
static const bool input_dummy = gflags::RegisterFlagValidator(&FLAGS_input, &validate_input);

static bool validate_nfields(const char* flagname, gflags::int32 value) 
{ return check_above_zero(flagname, value); }
static const bool nfields_dummy = gflags::RegisterFlagValidator(&FLAGS_nfields, &validate_nfields);

static bool validate_ntrees(const char* flagname, gflags::int32 value) 
{ 
    if (!FLAGS_build)
        return true;
    return check_above_zero(flagname, value); 
}
static const bool ntrees_dummy = gflags::RegisterFlagValidator(&FLAGS_ntrees, &validate_ntrees);

static bool validate_idx(const char* flagname, const std::string &value) 
{ return check_not_empty(flagname, value); }
static const bool idx_dummy = gflags::RegisterFlagValidator(&FLAGS_idx, &validate_idx);

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
        if ( !ifs || !*ifs)
            THROW_RUNTIME_ERROR("Cannot open " << FLAGS_input << " for reading!");
    } // if

    return ifs;
}

} // namespace

typedef uint32_t IdType;
typedef float    ValueType;
typedef AnnDB<IdType, ValueType>    AnnDbType;
static boost::shared_ptr<AnnDbType> g_pAnnDB;

static
void do_build_routine()
{
    using namespace std;

    cout << "Building AnnDB..." << endl;

    boost::shared_ptr<istream> ifs = open_input();

    string line;
    vector<ValueType> vec;
    
    vec.reserve(FLAGS_nfields);
    size_t lineno = 0;
    while (getline(*ifs, line)) {
        ++lineno;
        vec.clear();
        stringstream stream(line);
        std::copy( istream_iterator<ValueType>(stream),
                   istream_iterator<ValueType>(),
                   back_inserter(vec) );
        try {
            g_pAnnDB->addItem( vec );
        } catch (const exception &ex) {
            LOG(ERROR) << "addItem error on line:" << lineno;
        } // try
    } // while

    cout << "Building Ann index..." << endl;
    g_pAnnDB->buildIndex( FLAGS_ntrees );
    cout << "Saving AnnDB to file..." << endl;
    g_pAnnDB->saveIndex( FLAGS_idx.c_str() );

    cout << "Build AnnDB job done!" << endl;
}

static
void do_serve_routine()
{
    using namespace std;
    
    cout << "Loading AnnDB from file..." << endl;
    g_pAnnDB->loadIndex( FLAGS_idx.c_str() );

    typedef boost::tuple<IdType&, ValueType&> IterType;
    IdType id;
    size_t k;
    vector<IdType>    likeIds;
    vector<ValueType> likeDistances;

    while (true) {
        cout << "\nInput id and k: " << flush;
        cin >> id >> k;
        if (!cin)
            break;
        likeIds.clear(); likeDistances.clear();
        g_pAnnDB->kNN_By_Id(id, k, likeIds, likeDistances);
        BOOST_FOREACH( IterType v, boost::combine(likeIds, likeDistances) )
            cout << v.get<0>() << "\t\t" << v.get<1>() << endl;
    } // while

    cout << "serve routine done!" << endl;
}

int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        g_pAnnDB.reset( new AnnDB<IdType, ValueType>(FLAGS_nfields) );

        if (FLAGS_build)
            do_build_routine();
        else
            do_serve_routine();

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
    } // try

    return 0;
}
