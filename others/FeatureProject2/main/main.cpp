/*
 * trim sep+SPACES, split only sep
 */
// # <fstream>
// # <boost/smart_ptr.hpp>
// # <thrift/protocol/TBinaryProtocol.h>
// # <thrift/protocol/TCompactProtocol.h>
// # <thrift/transport/TFileTransport.h>
// # <thrift/transport/TFDTransport.h>
// # <thrift/transport/TBufferTransports.h>
// # <thrift/transport/TZlibTransport.h>
// # <boost/format.hpp>
// # <boost/lexical_cast.hpp>
// # <glog/logging.h>
// # "Feature.h"
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <climits>
#include "CommDef.h"
#include "FeatureTask.h"
#include "FvFile.h"


DEFINE_string(conf, "", "Info about raw data in json format");
DEFINE_string(dump, "", "Print fv file");
DEFINE_int32(top, 0, "dump top k lines");


namespace Test {
} // namespace Test


static
void do_dump(const std::string &fname, int _TopK)
{
    using namespace std;

    uint32_t topk = _TopK ? (uint32_t)_TopK : std::numeric_limits<uint32_t>::max();

    IFvFile ifv(fname);
    for (uint32_t i = 0; i < topk; ++i) {
        FeatureVector fv;
        if (!ifv.readOne(fv))
            break;
        cout << fv << endl;
    } // for i
}


int main(int argc, char **argv)
try {
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (!FLAGS_conf.empty()) {
        auto pTaskMgr = std::make_shared<FeatureTaskMgr>();
        pTaskMgr->loadConf(FLAGS_conf);
        pTaskMgr->start();

    } else if (!FLAGS_dump.empty()) {
        do_dump(FLAGS_dump, FLAGS_top);

    } else {
        cerr << "Wrong command!" << endl;
        return -1;
    } // if

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught by main: " << ex.what() << std::endl;
    return -1;
}

