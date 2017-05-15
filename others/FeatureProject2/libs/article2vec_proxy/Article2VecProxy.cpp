#include "Article2VecProxy.h"
#include <glog/logging.h>
#include <cassert>
#include <sstream>
#include <boost/filesystem.hpp>
#include "CommDef.h"
#include "utils/read_cmd.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Article2vecProxy(name, mgr); }


void Article2vecProxy::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "Article2vecProxy::init()";

    THROW_RUNTIME_ERROR_IF(input().empty(), "Article2vecProxy::init() input file not specified!");
    THROW_RUNTIME_ERROR_IF(output().empty(), "Article2vecProxy::init() output file not specified!");

    // hasid
    {
        const auto &jv = conf["hasid"];
        if (!!jv) m_bHasId = jv.asBool();
    } // hasid

    fs::path dataPath(m_pTaskMgr->dataDir());

    m_strRef = conf["ref"].asString();
    THROW_RUNTIME_ERROR_IF(m_strRef.empty(), "Article2vecProxy::init() ref file not specified!");
    m_strRef = (dataPath / m_strRef).c_str();

    m_strMethod = conf["method"].asString();
    THROW_RUNTIME_ERROR_IF(m_strMethod.empty(), "Article2vecProxy::init() method not specified!");
}


void Article2vecProxy::run()
{
    using namespace std;

    DLOG(INFO) << "Article2vecProxy::run()";

    LOG(INFO) << "Converting " << m_strInput << " to vectors...";
    
    ostringstream oss;
    oss << "GLOG_logtostderr=1 ./article2vec.bin -input " << m_strInput
        << " -output " << m_strOutput << " -ref " << m_strRef << " -method "
        << m_strMethod << " -hasid " << (m_bHasId ? "true" : "false") << flush;
    DLOG(INFO) << "cmd = " << oss.str();

    string output;
    int retcode = Utils::read_cmd(oss.str(), output);
    if (!output.empty()) cout << output << endl;

    if (retcode)
        LOG(ERROR) << "Run article2vec error!";
    else
        LOG(INFO) << "Converted vector data has written to " << m_strOutput;
}


