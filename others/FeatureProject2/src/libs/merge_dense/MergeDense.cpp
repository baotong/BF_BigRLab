#include "MergeDense.h"
#include <glog/logging.h>
#include <cassert>
#include <iterator>
#include <fstream>
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
            if (!getline(ifs, line)) {
                // DLOG(INFO) << "getline fail";
                return false;
            } // if
            istringstream iss(line);
            iss >> id;
            if (!iss) continue;
            vec.clear();
            std::copy(istream_iterator<double>(iss), istream_iterator<double>(), 
                    back_inserter(vec));
            // DLOG(INFO) << "vec size = " << vec.size();
            if (!vec.size()) { /* DLOG(INFO) << "read fail"; */ continue; }
            else { /* DLOG(INFO) << "read success"; */ return true; }
        } // while
        return true;
    };

    string          id;
    vector<double>  vec;
    if (!readLine(id, vec)) {
        LOG(ERROR) << m_strDense << " does not contains any valid dense data!";
        return;
    } // if

    IFvFile ifv(m_strInput);
    OFvFile ofv(m_strOutput);

    FeatureVector fv;
    while (ifv.readOne(fv)) {
        FeatureVectorHandle hfv(fv);

        while (id < hfv.id()) {
            if (!readLine(id, vec))
                break;    
        } // while id

        if (id == hfv.id())
            hfv.setFeature(m_strFeature, vec);

        ofv.writeOne(fv);
    } // while fv
}


void MergeDense::mergeWithoutId()
{
    THROW_RUNTIME_ERROR("MergeDense::mergeWithoutId() TODO");
}


