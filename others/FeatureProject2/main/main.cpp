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
#include "CommDef.h"
#include "FeatureTask.h"


DEFINE_string(conf, "", "Info about raw data in json format");


namespace Test {
} // namespace Test



int main(int argc, char **argv)
try {
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);


    auto pTaskMgr = std::make_shared<FeatureTaskMgr>();
    pTaskMgr->loadConf(FLAGS_conf);
    pTaskMgr->start();

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught by main: " << ex.what() << std::endl;
    return -1;
}

