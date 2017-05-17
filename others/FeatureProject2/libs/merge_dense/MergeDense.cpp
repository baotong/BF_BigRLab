#include "MergeDense.h"
#include <glog/logging.h>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "CommDef.h"
#include "FvFile.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new MergeDense(name, mgr); }


void MergeDense::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;
    using namespace std;

    FeatureTask::init(conf);

    DLOG(INFO) << "MergeDense::init()";
    THROW_RUNTIME_ERROR_IF(input().empty(), "MergeDense::init() input file not specified!");

    fs::path dataPath(m_pTaskMgr->dataDir());

    m_strDense = conf["dense"].asString();
    boost::trim(m_strDense);
    THROW_RUNTIME_ERROR_IF(m_strDense.empty(), "MergeDense::init() dense file not specified!");
    m_strDense = (dataPath / m_strDense).c_str();

    m_strFeature = conf["feature"].asString();
    boost::trim(m_strFeature);
    THROW_RUNTIME_ERROR_IF(m_strFeature.empty(), "MergeDense::init() feature name not specified!");
}


void MergeDense::run()
{
    DLOG(INFO) << "MergeDense::run()";

    LOG(INFO) << "Merging " << m_strInput << " and " << m_strDense;
    if (hasId())
        mergeWithId();
    else
        mergeWithoutId();
    LOG(INFO) << "Merged data has written to " << m_strOutput;
    m_pTaskMgr->setLastOutput(m_strOutput);

    if (autoRemove()) {
        LOG(INFO) << "Removing " << m_strInput;
        remove_file(m_strInput);
    } // if
}


void MergeDense::mergeWithId()
{
    using namespace std;

    ifstream ifs(m_strDense, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "MergeDense cannot open " << m_strDense << " for reading!");

    auto readLine = [&](string &id, vector<double> &vec)->bool {
        while (true) {
            string line;
            if (!getline(ifs, line))
                return false;
            istringstream iss(line);
            iss >> id;
            if (!iss) continue;
            vec.clear();
            std::copy(istream_iterator<double>(iss), istream_iterator<double>(), 
                    back_inserter(vec));
            if (iss.fail()) continue;
        } // while
        return true;
    };

    IFvFile ifv(m_strInput);
    OFvFile ofv(m_strOutput);

    FeatureVector fv;
    while (ifv.readOne(fv)) {
        FeatureVectorHandle hfv(fv);
        string          id;
        vector<double>  vec;
        do {
            if (!readLine(id, vec))
                break;
            
        } while (hfv.id() < id);
    } // while
}


void MergeDense::mergeWithoutId()
{
    THROW_RUNTIME_ERROR("MergeDense::mergeWithoutId() TODO");
}


