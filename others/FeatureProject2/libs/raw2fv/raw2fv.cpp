#include <glog/logging.h>
#include <fstream>
#include <cassert>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/smart_ptr.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TFileTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TZlibTransport.h>
#include "utils/str2time.hpp"
#include "utils/read_cmd.h"
#include "CommDef.h"
#include "raw2fv.h"


FeatureTask* create_instance(const std::string &name, FeatureTaskMgr *mgr)
{ return new Raw2Fv(name, mgr); }


void Raw2Fv::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;

    FeatureTask::init(conf);

    DLOG(INFO) << "Raw2Fv::init()";
    fs::path dataPath(m_pTaskMgr->dataDir());

    THROW_RUNTIME_ERROR_IF(input().empty(), "Raw2Fv::init() input file not specified!");
    m_strInput = (dataPath / m_strInput).c_str();

    THROW_RUNTIME_ERROR_IF(output().empty(), "Raw2Fv::init() output file not specified!");
    m_strOutput = (dataPath / m_strOutput).c_str();

    m_strDesc = conf["desc"].asString();
    THROW_RUNTIME_ERROR_IF(m_strDesc.empty(), "Raw2Fv::init() data desc file not specified!");
    m_strNewDesc = conf["new_desc"].asString();
    if (m_strNewDesc.empty()) m_strNewDesc = m_strDesc;
    m_strDesc = (dataPath / m_strDesc).c_str();
    m_strNewDesc = (dataPath / m_strNewDesc).c_str();

    m_bHasId = conf["hasid"].asBool();
    m_bSetGlobalDesc = conf["set_global_desc"].asBool();
}


void Raw2Fv::run()
{
    DLOG(INFO) << "Raw2Fv::run()";

    LOG(INFO) << "Loading data desc " << m_strDesc << "...";
    loadDesc();

    LOG(INFO) << "Loading data " << m_strInput << 
            " and converting to feature vectors " << m_strOutput << "...";
    if (m_bHasId)
        loadDataWithId();
    else
        loadDataWithoutId();
}


void Raw2Fv::loadDesc()
{
    namespace fs = boost::filesystem;
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
    // read gSep
    {
        Json::Value &jv = root["sep"];
        if (!jv) {
            LOG(WARNING) << "Attr \"sep\" not set, use default blank chars.";
        } else {
            m_strSep = jv.asString();
        } // jv
    } // read gSep

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
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    DLOG(INFO) << "Raw2Fv::loadDataWithId()";

    // s_strDenseId.clear();

    ifstream ifs(m_strInput, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "Raw2Fv::loadDataWithId() cannot open " << m_strInput << " for reading!");

    // check & trunc out file
    {
        ofstream ofs(m_strOutput, ios::out | ios::trunc);
        THROW_RUNTIME_ERROR_IF(!ofs, "Raw2Fv::loadDataWithId() cannot open " << m_strOutput << " for writting!");
    }

    auto _transport1 = boost::make_shared<TFileTransport>(m_strOutput);
    auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
    auto transport = boost::make_shared<TZlibTransport>(_transport2,
            128, 1024,
            128, 1024,
            Z_BEST_COMPRESSION);
    auto protocol = boost::make_shared<TBinaryProtocol>(transport);

    assert(m_pFeatureInfoSet);
    
    auto &fiSet = *m_pFeatureInfoSet;
    const size_t nFeatures = fiSet.size();

    string line;
    size_t lineCnt = 0;
    while (getline(ifs, line)) {
        ++lineCnt;
        boost::trim_if(line, boost::is_any_of(m_strSep + SPACES));  // NOTE!!! 整行trim应该去掉分割符和默认trim空白字符
        if (line.empty()) continue;
        vector<string> strValues;
        boost::split(strValues, line, boost::is_any_of(m_strSep), boost::token_compress_on);
        FeatureVector fv;
        FeatureVectorHandle hFv(fv, fiSet);
        hFv.setId(strValues[0]);
        uint32_t i = 1, j = 0;
        while (i < strValues.size() && j < nFeatures) {
            FeatureInfo &fi = *fiSet[j];
            if (fi.type() == "list_double") {
                read_list_double_feature_id(fv, fi, lineCnt);
                ++j;
            } else {
                read_feature(fv, strValues[i], fi, lineCnt);
                ++i; ++j;
            } // if
        } // while
        // DLOG(INFO) << fv;
        fv.write(protocol.get());
    } // while

    transport->finish();
}


void Raw2Fv::read_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    if (!ftInfo.isKeep()) return;
    if (ftInfo.type() == "string") {
        read_string_feature(fv, strField, ftInfo, lineno);
    } else if (ftInfo.type() == "double") {
        read_double_feature(fv, strField, ftInfo, lineno);
    }  else if (ftInfo.type() == "datetime") {
        read_datetime_feature(fv, strField, ftInfo, lineno);
    } else {
        THROW_RUNTIME_ERROR("read_feature in line " << lineno << 
                ", feature type \"" << ftInfo.type() << "\" is invalid!");
    } // if
}


void Raw2Fv::read_string_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    auto &fiSet = *m_pFeatureInfoSet;

    boost::trim_if(strField, boost::is_any_of(ftInfo.sep() + SPACES));
    LOG_IF(WARNING, strField.empty()) << "read_string_feature in line " << lineno
            << " empty feature " << ftInfo.name();

    FeatureVectorHandle hFv(fv, fiSet);

    if (ftInfo.isMulti()) {
        vector<string> strValues;
        const string sep = (ftInfo.sep().empty() ? SPACES : ftInfo.sep());
        boost::split(strValues, strField, boost::is_any_of(sep), boost::token_compress_on);
        for (auto &v : strValues) {
            boost::trim(v);
            hFv.addFeature(ftInfo.name(), v);
        } // for
    } else {
        hFv.setFeature(ftInfo.name(), strField);
    } // if
}


void Raw2Fv::read_double_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    auto &fiSet = *m_pFeatureInfoSet;

    boost::trim(strField);
    THROW_RUNTIME_ERROR_IF(strField.empty(), "read_double_feature in line "
            << lineno << ", empty field of \"" << ftInfo.name() << "\"!");
    
    FeatureVectorHandle hFv(fv, fiSet);

    if (ftInfo.isMulti()) {
        vector<string> strValues;
        const string sep = (ftInfo.sep().empty() ? SPACES : ftInfo.sep());
        boost::split(strValues, strField, boost::is_any_of(sep), boost::token_compress_on);
        for (auto &v : strValues) {
            boost::trim(v);
            auto pos = v.rfind('=');
            THROW_RUNTIME_ERROR_IF(string::npos == pos,
                    "read_double_feature in line " << lineno << " for feature \""
                    << ftInfo.name() << "\" wrong format!");
            string key(v, 0, pos);
            string value(v, pos+1);
            boost::trim(key); boost::trim(value);
            double val = 0.0;
            THROW_RUNTIME_ERROR_IF(!boost::conversion::try_lexical_convert(value, val),
                    "read_double_feature in line " << lineno << " for feature \""
                    << ftInfo.name() << "\" cannot covert \"" << value << "\" to double!");
            hFv.setFeature(val, ftInfo.name(), key);
            ftInfo.setMinMax(val, key);
        } // for
    } else {
        double val = 0.0;
        THROW_RUNTIME_ERROR_IF(!boost::conversion::try_lexical_convert(strField, val),
                "read_double_feature in line " << lineno << " for feature \""
                << ftInfo.name() << "\" cannot covert \"" << strField << "\" to double!");
        hFv.setFeature(val, ftInfo.name());
        ftInfo.setMinMax(val);
    } // if
}


void Raw2Fv::read_datetime_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    auto &fiSet = *m_pFeatureInfoSet;

    boost::trim(strField);
    THROW_RUNTIME_ERROR_IF(strField.empty(), "read_datetime_feature in line "
            << lineno << ", empty field of \"" << ftInfo.name() << "\"!");
    
    FeatureVectorHandle hFv(fv, fiSet);

    string fmt = "%Y-%m-%d %H:%M:%S";       // TODO 日期时间格式
    time_t tm = 0;
    THROW_RUNTIME_ERROR_IF(!Utils::str2time(strField, tm, fmt),
            "read_datetime_feature in line " << lineno << " for feature \""
            << ftInfo.name() << "\" cannot covert \"" << strField << "\" to datetime!");

    hFv.setFeature((double)tm, ftInfo.name());
    ftInfo.setMinMax((double)tm);
}


void Raw2Fv::read_list_double_feature_id(FeatureVector &fv, 
            FeatureInfo &fi, const std::size_t lineno)
{
    using namespace std;

    auto &fiSet = *m_pFeatureInfoSet;
    FeatureVectorHandle hFv(fv, fiSet);

    if (!fi.denseLen()) parseDense(fi);
}


void Raw2Fv::parseDense(FeatureInfo &fi)
{
    namespace fs = boost::filesystem;
    using namespace std;

    fs::path dataPath(m_pTaskMgr->dataDir());
    dataPath /= fi.densePath();
    fi.pathDense().swap(dataPath);

    DLOG(INFO) << "Parsing dense file: " << fi.pathDense();

    std::ostringstream oss;
    oss << "tail -1 " << fi.pathDense() << " | awk \'{print NF}\'" << std::flush;

    string output;
    int retcode = Utils::read_cmd(oss.str(), output);
    THROW_RUNTIME_ERROR_IF(retcode, "Parse dense file fail!");

    std::istringstream iss(output);
    iss >> fi.denseLen();
    THROW_RUNTIME_ERROR_IF(!iss, "Read dense len fail!");

    if (m_bHasId) --fi.denseLen();

    DLOG(INFO) << "Dense length is: " << fi.denseLen();
}


bool Raw2Fv::readDense(std::vector<double> &vec, FeatureInfo &fi)
{
    namespace fs = boost::filesystem;
    using namespace std;

    if (!fi.denseFile()) {
        if (fi.pathDense().empty()) {
            fs::path dataPath(m_pTaskMgr->dataDir());
            dataPath /= fi.densePath();
            fi.pathDense().swap(dataPath);
        } // if
        fi.denseFile().reset(new ifstream(fi.pathDense().c_str(), ios::in));
        THROW_RUNTIME_ERROR_IF(!fi.denseFile(), 
                "Raw2Fv::readDense() cannot open " << fi.pathDense() << " for reading!");
    } // if

    // DLOG(INFO) << "readDense() loading " << m_pathDense;

    istream& ifs = *fi.denseFile();
    
    vec.clear();
    vec.reserve(fi.denseLen());

    string line;
    if (!getline(ifs, line))
        return false;

    istringstream iss(line);
    std::copy(istream_iterator<double>(iss), istream_iterator<double>(),
            std::back_inserter(vec));

    LOG_IF(WARNING, vec.size() != fi.denseLen()) << 
        "read vector size " << vec.size() << " not equal to parsed size " << fi.denseLen();

    return true;
}


bool Raw2Fv::readDenseId(std::string &id, std::vector<double> &vec, FeatureInfo &fi)
{
    namespace fs = boost::filesystem;
    using namespace std;

    DLOG(INFO) << "Raw2Fv::readDenseId()";

    if (!fi.denseFile()) {
        if (fi.pathDense().empty()) {
            fs::path dataPath(m_pTaskMgr->dataDir());
            dataPath /= fi.densePath();
            fi.pathDense().swap(dataPath);
        } // if
        fi.denseFile().reset(new ifstream(fi.pathDense().c_str(), ios::in));
        THROW_RUNTIME_ERROR_IF(!fi.denseFile(), 
                "Raw2Fv::readDense() cannot open " << fi.pathDense() << " for reading!");
    } // if

    // DLOG(INFO) << "readDense() loading " << m_pathDense;

    istream& ifs = *fi.denseFile();
    
    vec.clear();
    vec.reserve(fi.denseLen());

    string line;
    if (!getline(ifs, line))
        return false;

    DLOG(INFO) << "Read line: " << line;
    istringstream iss(line);
    iss >> id;
    std::copy(istream_iterator<double>(iss), istream_iterator<double>(),
            std::back_inserter(vec));

    LOG_IF(WARNING, vec.size() != fi.denseLen()) << 
        "read vector size " << vec.size() << " not equal to parsed size " << fi.denseLen();
    return true;
}


