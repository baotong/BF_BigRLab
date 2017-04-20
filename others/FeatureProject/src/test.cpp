/*
 * # just storing feature vector data
 * GLOG_logtostderr=1 ./test.bin -op store -raw ../data/adult.data -conf ../data/adult_conf.json -fv ../data/adult.fv
 *
 * # convert feature vector into xgboost
 * GLOG_logtostderr=1 ./test.bin -op fmt:xgboost:../data/adult.data.xgboost -conf ../data/adult_conf.json -fv ../data/adult.fv
 *
 * ## normalize
 * # only one feature
 * GLOG_logtostderr=1 ./test.bin -op normalize:age -conf ../data/adult_conf.json -fv ../data/adult.fv -o ../data/adult.normalized
 * # all float features
 * GLOG_logtostderr=1 ./test.bin -op normalize -conf ../data/adult_conf.json -fv ../data/adult.fv -o ../data/adult.normalized
 *
 * ## dump fv
 * GLOG_logtostderr=1 ./test.bin -op dump -fv ../data/adult.normalized -o /tmp/out.txt
 * # first 100 lines
 * GLOG_logtostderr=1 ./test.bin -op dump:100 -fv ../data/adult.normalized -o /tmp/out.txt
 *
 * ./test.bin ../data/adult.data ../data/adult_conf.json ../data/out.txt
 * ./test.bin ../data/search_no_id.data ../data/search_conf.json ../data/out.txt
 */
/*
 * trim sep+SPACES, split only sep
 */
#include <fstream>
#include <boost/smart_ptr.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/transport/TFileTransport.h>
#include <thrift/transport/TFDTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TZlibTransport.h>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <json/json.h>
#include "Feature.h"


DEFINE_string(op, "", "Operation to do on data");
DEFINE_string(raw, "", "Raw data file to input");
DEFINE_string(conf, "", "Info about raw data in json format");
DEFINE_string(fv, "", "Serialized feature vector file");
DEFINE_string(o, "", "output file");

Json::Value         g_jsConf;


namespace Test {
    using namespace std;
    void print_feature_info()
    {
        cout << "Totally " << g_ftInfoSet.size() << " features." << endl;
        cout << "Global seperator = " << g_strSep << endl;
        cout << endl;
        for (auto &pf : g_ftInfoSet.arrFeature())
            cout << *pf << endl;
    }

    void test_serial2file(const std::string &fname, const Example &exp)
    {
        using namespace apache::thrift;
        using namespace apache::thrift::protocol;
        using namespace apache::thrift::transport;

        // open & trunc
        {
            ofstream ofs(fname, ios::out | ios::trunc);
            THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open file \"" << fname << "\" for writting!");
        }

        // 默认是每次追加，所以要先trunc
        auto _transport1 = boost::make_shared<TFileTransport>(fname);
        auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
        auto transport = boost::make_shared<TZlibTransport>(_transport2,
                128, 1024,
                128, 1024,
                Z_BEST_COMPRESSION);
        auto protocol = boost::make_shared<TBinaryProtocol>(transport);

        for (const auto &f : exp.example)
            f.write(protocol.get());

        // transport->flush();     // NOTE!!! must do this if using buffered transport
        transport->finish();
    }

    void test_read_serial(const std::string &fname, Example &exp)
    {
        using namespace apache::thrift;
        using namespace apache::thrift::protocol;
        using namespace apache::thrift::transport;

        auto _transport1 = boost::make_shared<TFileTransport>(fname, true); // true read-only
        auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
        auto transport = boost::make_shared<TZlibTransport>(_transport2);
        auto protocol = boost::make_shared<TBinaryProtocol>(transport);

        // transport->open();

        auto &arr = exp.example;

        FeatureVector fv;
        while (true) {
            try {
                fv.read(protocol.get());
                arr.emplace_back(std::move(fv));
            } catch (const apache::thrift::transport::TTransportException&) {
                break;
            } // try
        } // while

        DLOG(INFO) << arr.size();
    }
} // namespace Test


// print one cell
static
void print_xgboost(std::ostream &os, FeatureVector &fv, FeatureInfo &fi)
{
    if (fi.type() == "string") {
        for (auto &s : fv.stringFeatures[fi.name]) {
            auto it = fi.index.find(s);
            THROW_RUNTIME_ERROR_IF(it == fi.index.end(),
                    "string value \"" << s << "\" is invalid!");
            auto idx = it->second;
            os << idx << ":1 ";
        } // for
    } else if (fi.type() == "double" || fi.type() == "datetime") {
        for (auto &kv : fv.floatFeatures[fi.name]) {
            auto it = fi.index.find(kv.first);
            THROW_RUNTIME_ERROR_IF(it == fi.index.end(),
                    "float value \"" << kv.first << "\" is invalid! " << fi.index.size());
            auto idx = it->second;
            os << idx << ":" << kv.second << " ";
        } // for
    } else {
        THROW_RUNTIME_ERROR("print_xgboost Invalid type!");
    } // if
}


void do_store(std::istringstream&)
{
    THROW_RUNTIME_ERROR_IF(FLAGS_raw.empty(), "No raw data file specified!");
    THROW_RUNTIME_ERROR_IF(FLAGS_conf.empty(), "No conf file specified!");
    THROW_RUNTIME_ERROR_IF(FLAGS_fv.empty(), "No fv data file specified!");

    LOG(INFO) << "Loading data...";
    load_feature_info(FLAGS_conf, g_jsConf, g_ftInfoSet);
    load_data(FLAGS_raw, FLAGS_fv, g_ftInfoSet);
    LOG(INFO) << "Feature vectors stored in " << FLAGS_fv;
    // Test::print_feature_info();
}


static
void load_fv(const std::string &fname)
{
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    auto _transport1 = boost::make_shared<TFileTransport>(fname, true); // true read-only
    auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
    auto transport = boost::make_shared<TZlibTransport>(_transport2);
    auto protocol = boost::make_shared<TBinaryProtocol>(transport);

    FeatureVector fv;
    while (true) {
        try {
            fv.read(protocol.get());
            // find min max values only for floatfeatures
            for (auto &kv : fv.floatFeatures) {
                auto pFtInfo = g_ftInfoSet.get(kv.first);
                THROW_RUNTIME_ERROR_IF(!pFtInfo,
                        "load_fv() data inconsistency!, no feature info \"" << kv.first << "\" found!");
                THROW_RUNTIME_ERROR_IF(kv.second.empty(), "load_fv() invalid data!");
                if (pFtInfo->multi) {
                    for (auto &kv2 : kv.second)
                        pFtInfo->setMinMax(kv2.second, kv2.first);
                } else {
                    auto it = kv.second.begin();
                    pFtInfo->setMinMax(it->second);
                } // if
            } // for kv
            // build index
            for (auto &pfi : g_ftInfoSet.arrFeature()) {
                if (pfi->type == "string") {
                    auto it = fv.stringFeatures.find(pfi->name);
                    THROW_RUNTIME_ERROR_IF(it == fv.stringFeatures.end(),
                            "load_fv() build index fail! data mismatch, cannot find feature " << pfi->name << " in data");
                    for (auto &s : it->second)
                        pfi->index[s] = 0;
                } else if (pfi->type == "double" || pfi->type == "datetime") {
                    auto it = fv.floatFeatures.find(pfi->name);
                    THROW_RUNTIME_ERROR_IF(it == fv.floatFeatures.end(),
                            "load_fv() build index fail! data mismatch, cannot find feature " << pfi->name << " in data");
                    for (auto &kv : it->second)
                        pfi->index[kv.first] = 0;
                } else if (pfi->type == "list_double") {
                    auto it = fv.denseFeatures.find(pfi->name);
                    THROW_RUNTIME_ERROR_IF(it == fv.denseFeatures.end(),
                            "load_fv() build index fail! data mismatch, cannot find feature " << pfi->name << " in data");
                    pfi->index[""] = 0;
                } // if
            } // for pfi
        } catch (const apache::thrift::transport::TTransportException&) {
            break;
        } // try
    } // while

    // build index
    uint32_t idx = 0;
    for (auto &pf : g_ftInfoSet.arrFeature()) {
        for (auto &kv : pf->index) {
            kv.second = idx++;
            // DLOG(INFO) << kv.second << "\t" << kv.first;
        } // for kv
    } // for
}


void do_format(std::istringstream &iss)
{
    using namespace std;
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    string      fmt, ofname;

    getline(iss, fmt, ':');
    THROW_RUNTIME_ERROR_IF(iss.fail() || iss.bad() || fmt.empty(),
            "do_format() cannot read specified format!");
    getline(iss, ofname, ':');
    THROW_RUNTIME_ERROR_IF(iss.fail() || iss.bad() || ofname.empty(),
            "do_format() cannot read specified output file name!");

    THROW_RUNTIME_ERROR_IF(FLAGS_conf.empty(), "No conf file specified!");
    THROW_RUNTIME_ERROR_IF(FLAGS_fv.empty(), "No fv data file specified!");

    LOG(INFO) << "Loading data...";
    load_feature_info(FLAGS_conf, g_jsConf, g_ftInfoSet);
    load_fv(FLAGS_fv);

    ofstream ofs(ofname, ios::out | ios::trunc);
    THROW_RUNTIME_ERROR_IF(!ofs, "do_format() cannot open " << ofname << " for writtingt!");

    auto _transport1 = boost::make_shared<TFileTransport>(FLAGS_fv, true);
    auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
    auto transport = boost::make_shared<TZlibTransport>(_transport2);
    auto protocol = boost::make_shared<TBinaryProtocol>(transport);

    LOG(INFO) << "Formatting data into " << fmt;

    FeatureVector fv;
    while (true) {
        try {
            fv.read(protocol.get());
            for (auto &pfi : g_ftInfoSet.arrFeature())
                print_xgboost(ofs, fv, *pfi);           // TODO only support xgboost now
            ofs << endl;
        } catch (const apache::thrift::transport::TTransportException&) {
            break;
        } // try
    } // while

    LOG(INFO) << "Formatted data stored in " << ofname;
}


void do_normalize(std::istringstream &iss)
{
    using namespace std;
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    THROW_RUNTIME_ERROR_IF(FLAGS_conf.empty(), "No conf file specified!");
    THROW_RUNTIME_ERROR_IF(FLAGS_fv.empty(), "No fv data file specified!");
    THROW_RUNTIME_ERROR_IF(FLAGS_o.empty(), "No output file specified!");

    // open & trunc
    {
        ofstream ofs(FLAGS_o, ios::out | ios::trunc);
        THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open file \"" << FLAGS_o << "\" for writting!");
    }

    load_feature_info(FLAGS_conf, g_jsConf, g_ftInfoSet);
    load_fv(FLAGS_fv);

    string segment;
    typedef map<string, set<string> >    FtSet;
    FtSet               ftSet;
    while (getline(iss, segment, ':')) {
        string ft, subft;
        auto pos = segment.find('.');
        if (pos == string::npos) {
            ft = segment;
        } else {
            ft = segment.substr(0, pos);
            subft = segment.substr(pos + 1);
        } // if
        auto ret = ftSet.insert(std::make_pair(ft, FtSet::mapped_type()));
        if (!subft.empty()) {
            auto &ftSetVal = ret.first->second;
            ftSetVal.insert(subft);
        } // if
    } // while

    auto _rdTransport1 = boost::make_shared<TFileTransport>(FLAGS_fv, true);
    auto _rdTransport2 = boost::make_shared<TBufferedTransport>(_rdTransport1);
    auto rdTransport = boost::make_shared<TZlibTransport>(_rdTransport2);
    auto rdProtocol = boost::make_shared<TBinaryProtocol>(rdTransport);

    auto _wrTransport1 = boost::make_shared<TFileTransport>(FLAGS_o);
    auto _wrTransport2 = boost::make_shared<TBufferedTransport>(_wrTransport1);
    auto wrTransport = boost::make_shared<TZlibTransport>(_wrTransport2,
            128, 1024,
            128, 1024,
            Z_BEST_COMPRESSION);
    auto wrProtocol = boost::make_shared<TBinaryProtocol>(wrTransport);

    LOG(INFO) << "Processing data...";
    FeatureVector fv;
    while (true) {
        try {
            fv.read(rdProtocol.get());
            for (auto &kv : fv.floatFeatures) {
                auto pFtInfo = g_ftInfoSet.get(kv.first);
                if (!ftSet.empty() && !ftSet.count(kv.first))
                    continue;
                THROW_RUNTIME_ERROR_IF(!pFtInfo,
                        "load_fv() data inconsistency!, no feature info \"" << kv.first << "\" found!");
                THROW_RUNTIME_ERROR_IF(kv.second.empty(), "load_fv() invalid data!");
                if (pFtInfo->multi) {
                    for (auto &kv2 : kv.second) {
                        if (!ftSet.empty() && !ftSet[kv.first].empty() && !ftSet[kv.first].count(kv2.first))
                            continue;
                        auto minmax = pFtInfo->getMinMax(kv2.first);
                        double range = minmax.second - minmax.first;
                        THROW_RUNTIME_ERROR_IF(range < 0.0,
                                "do_normalize() invalid min max value for feature " << kv.first);
                        double &val = kv2.second;
                        if (range == 0.0) {
                            val = 0.0;
                        } else {
                            val -= minmax.first;
                            val /= range;
                        } // if range
                    } // for kv2
                } else {
                    auto minmax = pFtInfo->getMinMax();
                    double range = minmax.second - minmax.first;
                    THROW_RUNTIME_ERROR_IF(range < 0.0,
                            "do_normalize() invalid min max value for feature " << kv.first);
                    auto it = kv.second.begin();
                    double &val = it->second;
                    if (range == 0.0) {
                        val = 0.0;
                    } else {
                        val -= minmax.first;
                        val /= range;
                    } // if range
                } // if
            } // for kv
            fv.write(wrProtocol.get());
        } catch (const apache::thrift::transport::TTransportException&) {
            break;
        } // try
    } // while

    wrTransport->finish();
    LOG(INFO) << "Normalized data kept in " << FLAGS_o;
}


void do_dump(std::istringstream &iss)
{
    using namespace std;
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    uint32_t cnt = std::numeric_limits<uint32_t>::max();

    string strCnt;
    getline(iss, strCnt, ':');
    if (!strCnt.empty())
        cnt = boost::lexical_cast<uint32_t>(strCnt);

    THROW_RUNTIME_ERROR_IF(FLAGS_fv.empty(), "No fv data file specified!");
    THROW_RUNTIME_ERROR_IF(FLAGS_o.empty(), "No output file specified!");
    ofstream ofs(FLAGS_o, ios::out | ios::trunc);
    THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open file \"" << FLAGS_o << "\" for writting!");

    auto _transport1 = boost::make_shared<TFileTransport>(FLAGS_fv, true);
    auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
    auto transport = boost::make_shared<TZlibTransport>(_transport2);
    auto protocol = boost::make_shared<TBinaryProtocol>(transport);

    FeatureVector fv;
    for (uint32_t i = 0; i < cnt; ++i) {
        try {
            fv.read(protocol.get());
            ofs << fv << endl;
        } catch (const apache::thrift::transport::TTransportException&) {
            break;
        } // try
    } // while
}


int main(int argc, char **argv)
try {
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    typedef std::function<void(istringstream&)>     OpFunc;
    typedef std::map<std::string, OpFunc>           OpTable;
    OpTable     opTable;
    opTable["store"] = do_store;
    opTable["fmt"] = do_format;
    opTable["normalize"] = do_normalize;
    opTable["dump"] = do_dump;

    RET_MSG_VAL_IF(FLAGS_op.empty(), -1, "Operation must be specified by \"-op\"");

    string opCmd;
    istringstream iss(FLAGS_op);
    getline(iss, opCmd, ':');
    RET_MSG_VAL_IF(iss.fail() || iss.bad(), -1, "Invalid operation format!");
    auto it = opTable.find(opCmd);
    RET_MSG_VAL_IF(it == opTable.end(), -1, "Invalid operation cmd!");
    it->second(iss);

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught by main: " << ex.what() << std::endl;
    return -1;
}



#if 0
static
void convert_xgboost(Example &exp, const std::string &fname)
{
    using namespace std;

    ofstream ofs(fname, ios::out);
    THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open file " << fname << " for writting!");

    for (size_t i = 0; i < exp.example.size(); ++i) {
        FeatureVector &fv = exp.example[i];
        for (auto &pfi : g_ftInfoSet.arrFeature())
            print_xgboost(ofs, fv, *pfi);
        ofs << endl;
    } // for
}

int main(int argc, char **argv)
try {
    using namespace std;

    RET_MSG_VAL_IF(argc < 4, -1,
            "Usage: " << argv[0] << " dataFile confFile outFile");

    const char *dataFilename = argv[1];
    const char *confFilename = argv[2];
    const char *outFilename = argv[3];

    load_feature_info(confFilename);
    // Test::print_feature_info();
    // Test::test_feature_op();

    Example exp;
    load_data(dataFilename, exp);
    DLOG(INFO) << exp.example.size();
    // cout << exp << endl;
    convert_xgboost(exp, outFilename);

    Test::test_serial2file("test.data", exp);
    Example exp2;
    Test::test_read_serial("test.data", exp2);
    convert_xgboost(exp2, "test.out");

    return 0;

} catch (const std::exception &ex) {
    cerr << "Exception caught by main: " << ex.what() << endl;
    return -1;
}
#endif



