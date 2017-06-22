#include <glog/logging.h>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include "utils/str2time.hpp"
#include "utils/read_cmd.h"
#include "utils/read_sep.hpp"
#include "CommDef.h"
#include "FvFile.h"
#include "raw2fv.h"


#define SLEEP_MILLISECONDS(x) boost::this_thread::sleep_for(boost::chrono::milliseconds(x))


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Raw2Fv(name, mgr); }


void Raw2Fv::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;

    FeatureTask::init(conf);

    DLOG(INFO) << "Raw2Fv::init()";
    fs::path dataPath(m_pTaskMgr->dataDir());

    THROW_RUNTIME_ERROR_IF(input().empty(), "Raw2Fv::init() input file not specified!");
    THROW_RUNTIME_ERROR_IF(output().empty(), "Raw2Fv::init() output file not specified!");

    m_strDesc = conf["desc"].asString();
    THROW_RUNTIME_ERROR_IF(m_strDesc.empty(), "Raw2Fv::init() data desc file not specified!");
    m_strDesc = (dataPath / m_strDesc).c_str();

    m_nWorkers = conf["workers"].asUInt();
    if (!m_nWorkers) {
        m_nWorkers = std::thread::hardware_concurrency();
        LOG(INFO) << "Number of workers not set, auto set to " << m_nWorkers;
    } // if
    DLOG(INFO) << "nWorkers = " << m_nWorkers; 

    uint32_t iBufSz = conf["input_buf"].asUInt();
    if (!iBufSz) iBufSz = 15000;
    m_pInputBuffer.reset(new SharedQueue<StringPtr>(iBufSz));
    DLOG(INFO) << "Input buffer size = " << iBufSz;

    uint32_t oBufSz = conf["output_buf"].asUInt();
    if (!oBufSz) oBufSz = (uint32_t)(iBufSz * 0.75);
    m_pOutputBuffer.reset(new SharedQueue<FvPtr>(oBufSz));
    DLOG(INFO) << "Output buffer size = " << oBufSz;
}


void Raw2Fv::run()
{
    using namespace std;

    DLOG(INFO) << "Raw2Fv::run()";

    LOG(INFO) << "Loading data desc " << m_strDesc << "...";
    loadDesc();

    // start writer thread
    LOG(INFO) << "Start writer thread...";
    OFvFile ofile(m_strOutput);
    m_pThrWriter.reset(new std::thread([&, this]{
        FvPtr pData;
        while (pData = m_pOutputBuffer->pop()) {
            ofile.writeOne(*pData);
        } // while
    }));

    // start worker threads
    LOG(INFO) << "Start worker threads...";
    auto pfLoadData = m_bHasId ? std::bind(&Raw2Fv::loadDataWithId, this)
                               : std::bind(&Raw2Fv::loadDataWithoutId, this);
    for (uint32_t i = 0; i < m_nWorkers; ++i)
        m_ThrWorkers.create_thread(pfLoadData);

    // start reader thread
    LOG(INFO) << "Start reader thread...";
    ifstream ifs(m_strInput, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, 
            "Raw2Fv cannot open " << m_strInput << " for reading!");
    m_pThrReader.reset(new std::thread([&, this]{
        std::size_t lineno = 0;
        while (true) {
            auto pLine = std::make_shared<string>();
            if (!getline(ifs, *pLine))
                break;
            if (!pLine->empty()) {
                m_pInputBuffer->push(pLine);
            } // if
            ++lineno;
            LOG_IF(INFO, lineno % 100000 == 0) << "Processed " << lineno << " lines";
        } // while
    }));

    // join reader thread
    m_pThrReader->join();
    // push empty for end mark
    // join workers
    for (uint32_t i = 0; i < m_nWorkers; ++i)
        m_pInputBuffer->push(StringPtr());
    m_ThrWorkers.join_all();
    LOG(INFO) << "All workers done.";

    // join writter
    m_pOutputBuffer->push(nullptr);
    m_pThrWriter->join();
    LOG(INFO) << "Feature vector data has written into " << m_strOutput;

    m_pTaskMgr->setLastOutput(m_strOutput);
}


void Raw2Fv::loadDesc()
{
    using namespace std;

    DLOG(INFO) << "Raw2Fv::loadDesc() " << m_strDesc;

    Json::Value     root;

    // load json file
    {
        ifstream ifs(m_strDesc, ios::in);
        THROW_RUNTIME_ERROR_IF(!ifs,
                "Raw2Fv::loadDesc() cannot open " << m_strDesc << " for reading!");

        ostringstream oss;
        oss << ifs.rdbuf() << flush;

        Json::Reader    reader;
        THROW_RUNTIME_ERROR_IF(!reader.parse(oss.str(), root),
                "Raw2Fv::loadDesc() Invalid json format!");
    } // end load json file

    uint32_t        nFeatures = 0;

    m_pFeatureInfoSet.reset(new FeatureInfoSet);

    // read nFeatures
    {
        Json::Value &jv = root["nfeatures"];
        THROW_RUNTIME_ERROR_IF(!jv, "No attr \"nFeatures\" found in config!");
        nFeatures = jv.asUInt();
        THROW_RUNTIME_ERROR_IF(!nFeatures, "Value of attr \"nFeatures\" " << nFeatures << " is invalid!");
    } // read nFeatures

    // split 用 sep, trim 用 SPACES
    // read Sep
    {
        Json::Value &jv = root["sep"];
        if (!jv) {
            LOG(WARNING) << "Attr \"sep\" not set, use default blank chars.";
        } else {
            m_strSep = jv.asString();
        } // jv
    } // read Sep

    Utils::read_sep(m_strSep);

    // read features
    {
        std::set<string> validTypes VALID_TYPES;

        Json::Value &jsFeatures = root["features"];
        THROW_RUNTIME_ERROR_IF(!jsFeatures, "No feature info found in config!");
        // DLOG(INFO) << jsFeatures.size();
        THROW_RUNTIME_ERROR_IF(jsFeatures.size() != nFeatures,
                "Num of feature info detected " << jsFeatures.size()
                << " not equal to value set in attr \"nFeatures\" " << nFeatures);
        for (auto &jf : jsFeatures) {
            auto pf = std::make_shared<FeatureInfo>();
            pf->fromJson(jf);
            THROW_RUNTIME_ERROR_IF(pf->isKeep() && pf->name().empty(), "Feature name cannot be empty!");
            THROW_RUNTIME_ERROR_IF(pf->isKeep() && !validTypes.count(pf->type()),
                    "Feature " << pf->name() << " has invalid type " << pf->type());
            m_pFeatureInfoSet->add(pf);
        } // for
    } // read features
}


void Raw2Fv::loadDataWithId()
{
    using namespace std;

    auto &fiSet = *m_pFeatureInfoSet;

    StringPtr pInput;
    while (pInput = m_pInputBuffer->pop()) {
        string &line = *pInput;
        vector<string> strValues;
        // boost::split(strValues, line, boost::is_any_of(m_strSep), boost::token_compress_on);
        boost::split(strValues, line, boost::is_any_of(m_strSep));
        if (strValues.size() - 1 != m_pFeatureInfoSet->size()) {
            LOG(ERROR) << "size mismatch! line size "
                << strValues.size() << " != nFeatures " << m_pFeatureInfoSet->size();
            m_pOutputBuffer->push(nullptr);
            continue;
        } // if
        std::for_each(strValues.begin(), strValues.end(), [](string &s){
            boost::trim(s);
        });
        auto pfv = std::make_shared<FeatureVector>();
        FeatureVectorHandle hFv(*pfv);
        hFv.setId(strValues[0]);
        bool success = true;
        for (size_t i = 0; i < m_pFeatureInfoSet->size(); ++i) {
            FeatureInfo &fi = *fiSet[i];
            if (!read_feature(*pfv, strValues[i+1], fi, 0)) {
                success = false;
                break;
            } // if
        } // for
        // DLOG(INFO) << fv;
        if (success) {
            m_pOutputBuffer->push(pfv);
        } else {
            m_pOutputBuffer->push(nullptr);
        } // if success
    } // while
}


void Raw2Fv::loadDataWithoutId()
{}


bool Raw2Fv::read_feature(FeatureVector &fv, std::string &strField,
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    if (!ftInfo.isKeep()) return true;
    if (ftInfo.type() == "string") {
        return read_string_feature(fv, strField, ftInfo, lineno);
    } else if (ftInfo.type() == "double") {
        return read_double_feature(fv, strField, ftInfo, lineno);
    } else if (ftInfo.type() == "datetime") {
        return read_datetime_feature(fv, strField, ftInfo, lineno);
    } // if 
    // invalid type
    LOG(ERROR) << "read_feature in line " << lineno <<
            ", feature type \"" << ftInfo.type() << "\" is invalid!";
    return false;
}


bool Raw2Fv::read_string_feature(FeatureVector &fv, std::string &strField,
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    // boost::trim_if(strField, boost::is_any_of(ftInfo.sep() + SPACES));
    if (strField.empty()) {
        return true;
    } // if

    FeatureVectorHandle hFv(fv);

    if (ftInfo.isMulti()) {
        vector<string> strValues;
        // const string sep = (ftInfo.sep().empty() ? SPACES : ftInfo.sep());
        string strSep = ftInfo.sep();
        Utils::read_sep(strSep);
        boost::split(strValues, strField, boost::is_any_of(strSep));
        for (auto &v : strValues) {
            boost::trim(v);
            if (!v.empty())
                hFv.addFeature(ftInfo.name(), v);
        } // for
    } else {
        hFv.setFeature(ftInfo.name(), strField);
    } // if

    return true;
}


bool Raw2Fv::read_double_feature(FeatureVector &fv, std::string &strField,
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    // boost::trim(strField);
    // boost::trim_if(strField, boost::is_any_of(ftInfo.sep() + SPACES));
    if (strField.empty()) {
        return true;
    } // if

    FeatureVectorHandle hFv(fv);

    if (ftInfo.isMulti()) {
        vector<string> strValues;
        // const string sep = (ftInfo.sep().empty() ? SPACES : ftInfo.sep());
        string strSep = ftInfo.sep();
        Utils::read_sep(strSep);
        boost::split(strValues, strField, boost::is_any_of(strSep));
        for (auto &v : strValues) {
            boost::trim(v);
            if (v.empty()) continue;
            auto pos = v.rfind('=');
            THROW_RUNTIME_ERROR_IF(string::npos == pos,
                    "read_double_feature in line " << lineno << " for feature \""
                    << ftInfo.name() << "\" wrong format!");
            string key(v, 0, pos);
            string value(v, pos+1);
            boost::trim(key); boost::trim(value);
            double val = 0.0;
            if (!boost::conversion::try_lexical_convert(value, val)) {
                LOG(ERROR) << "read_double_feature in line " << lineno << " for feature \""
                        << ftInfo.name() << "\" cannot covert \"" << value << "\" to double!";
                return false;
            } // if
            hFv.setFeature(val, ftInfo.name(), key);
        } // for
    } else {
        double val = 0.0;
        if (!boost::conversion::try_lexical_convert(strField, val)) {
            LOG(ERROR) << "read_double_feature in line " << lineno << " for feature \""
                    << ftInfo.name() << "\" cannot covert \"" << strField << "\" to double!";
            return false;
        } // if
        hFv.setFeature(val, ftInfo.name());
    } // if

    return true;
}


bool Raw2Fv::read_datetime_feature(FeatureVector &fv, std::string &strField,
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    // boost::trim(strField);
    if (strField.empty()) {
        LOG(ERROR) << "read_datetime_feature in line "
            << lineno << ", empty field of \"" << ftInfo.name() << "\"!";
        return false;
    } // if

    FeatureVectorHandle hFv(fv);

    string fmt = "%Y-%m-%d %H:%M:%S";       // TODO 日期时间格式
    time_t tm = 0;
    if (!Utils::str2time(strField, tm, fmt)) {
        LOG(ERROR) << "read_datetime_feature in line " << lineno << " for feature \""
                << ftInfo.name() << "\" cannot covert \"" << strField << "\" to datetime!";
        return false;
    } // if

    hFv.setFeature((double)tm, ftInfo.name());

    return true;
}

