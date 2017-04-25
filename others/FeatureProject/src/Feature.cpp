#include <fstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <glog/logging.h>
#include <json/json.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/smart_ptr.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TFileTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TZlibTransport.h>
#include "Feature.h"

FeatureInfoSet                       g_ftInfoSet;
std::string                          g_strSep = SPACES;


inline
bool str2time(const std::string &s, std::time_t &tt, 
              const std::string &format = "%Y-%m-%d %H:%M:%S")
{
    std::tm tm;
    char *success = ::strptime(s.c_str(), format.c_str(), &tm);
    if (!success) return false;
    tt = std::mktime(&tm);
    return true;
}


void load_feature_info(const std::string &fname, FeatureInfoSet &fiSet)
{
    using namespace std;

    Json::Value     root;

    // load json file
    {
        ifstream ifs(fname, ios::in);
        THROW_RUNTIME_ERROR_IF(!ifs, "load_feature_info cannot open " << fname << " for reading!");

        ostringstream oss;
        oss << ifs.rdbuf() << flush;

        Json::Reader    reader;
        THROW_RUNTIME_ERROR_IF(!reader.parse(oss.str(), root),
                    "Invalid json format!");
    } // end load json file

    uint32_t        nFeatures = 0;

    fiSet.clear();

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
            g_strSep = jv.asString();
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
            fiSet.add(pf);
        } // for
        // for (auto &jf : jsFeatures) {
            // auto pf = std::make_shared<FeatureInfo>(jf["name"].asString(), jf["type"].asString());
            // THROW_RUNTIME_ERROR_IF(!validTypes.count(pf->type()),
                    // "Feature " << pf->name() << " has invalid type " << pf->type());
            // fiSet.add(pf);

            // auto &jMulti = jf["multi"];
            // if (!!jMulti) pf->setMulti(jMulti.asBool());
            // auto &jSep = jf["sep"];
            // if (!!jSep) pf->sep() = jSep.asString();
        // } // for jf
    } // read features
}

static
void read_string_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno, FeatureInfoSet &fiSet)
{
    using namespace std;

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

static
void read_double_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno, FeatureInfoSet &fiSet)
{
    using namespace std;

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

static
void read_list_double_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno, FeatureInfoSet &fiSet)
{
    using namespace std;

    boost::trim_if(strField, boost::is_any_of(ftInfo.sep() + SPACES));
    THROW_RUNTIME_ERROR_IF(strField.empty(), "read_list_double_feature in line "
            << lineno << ", empty field of \"" << ftInfo.name() << "\"!");

    FeatureVectorHandle hFv(fv, fiSet);

    vector<string> strValues;
    const string sep = (ftInfo.sep().empty() ? SPACES : ftInfo.sep());
    boost::split(strValues, strField, boost::is_any_of(sep), boost::token_compress_on);
    vector<double> values(strValues.size(), 0.0);

    for (size_t i = 0; i < values.size(); ++i) {
        THROW_RUNTIME_ERROR_IF(!boost::conversion::try_lexical_convert(strValues[i], values[i]),
                "read_list_double_feature in line " << lineno << " for feature \""
                << ftInfo.name() << "\" cannot covert \"" << strValues[i] << "\" to double!");
    } // for

    hFv.setFeature(ftInfo.name(), values);
}

static
void read_datetime_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno, FeatureInfoSet &fiSet)
{
    using namespace std;

    boost::trim(strField);
    THROW_RUNTIME_ERROR_IF(strField.empty(), "read_datetime_feature in line "
            << lineno << ", empty field of \"" << ftInfo.name() << "\"!");
    
    FeatureVectorHandle hFv(fv, fiSet);

    string fmt = "%Y-%m-%d %H:%M:%S";       // TODO 日期时间格式
    time_t tm = 0;
    THROW_RUNTIME_ERROR_IF(!str2time(strField, tm, fmt),
            "read_datetime_feature in line " << lineno << " for feature \""
            << ftInfo.name() << "\" cannot covert \"" << strField << "\" to datetime!");

    hFv.setFeature((double)tm, ftInfo.name());
    ftInfo.setMinMax((double)tm);
}

static
void read_feature(FeatureVector &fv, std::string &strField, 
            FeatureInfo &ftInfo, const std::size_t lineno, FeatureInfoSet &fiSet)
{
    if (!ftInfo.isKeep()) return;
    if (ftInfo.type() == "string") {
        read_string_feature(fv, strField, ftInfo, lineno, fiSet);
    } else if (ftInfo.type() == "double") {
        read_double_feature(fv, strField, ftInfo, lineno, fiSet);
    } else if (ftInfo.type() == "list_double") {
        read_list_double_feature(fv, strField, ftInfo, lineno, fiSet);
    } else if (ftInfo.type() == "datetime") {
        read_datetime_feature(fv, strField, ftInfo, lineno, fiSet);
    } else {
        THROW_RUNTIME_ERROR("read_feature in line " << lineno << 
                ", feature type \"" << ftInfo.type() << "\" is invalid!");
    } // if
}


void load_data(const std::string &ifname, const std::string &ofname, FeatureInfoSet &fiSet)
{
    using namespace std;
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    ifstream ifs(ifname, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "load_data cannot open " << ifname << " for reading!");

    // check & trunc out file
    {
        ofstream ofs(ofname, ios::out | ios::trunc);
        THROW_RUNTIME_ERROR_IF(!ofs, "load_data cannot open " << ofname << " for writting!");
    }

    auto _transport1 = boost::make_shared<TFileTransport>(ofname);
    auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
    auto transport = boost::make_shared<TZlibTransport>(_transport2,
            128, 1024,
            128, 1024,
            Z_BEST_COMPRESSION);
    auto protocol = boost::make_shared<TBinaryProtocol>(transport);

    const size_t nFeatures = fiSet.size();

    string line;
    size_t lineCnt = 0;
    while (getline(ifs, line)) {
        ++lineCnt;
        boost::trim_if(line, boost::is_any_of(g_strSep + SPACES));  // NOTE!!! 整行trim应该去掉分割符和默认trim空白字符
        if (line.empty()) continue;
        vector<string> strValues;
        boost::split(strValues, line, boost::is_any_of(g_strSep), boost::token_compress_on);
        // for (auto &s : strValues)
            // cout << s << endl;
        THROW_RUNTIME_ERROR_IF(strValues.size() != nFeatures,
                "Error when processing data file in line " << lineCnt 
                << ", nFeatures not match! " << nFeatures << " expected but " 
                << strValues.size() << " detected.");
        FeatureVector fv;
        for (size_t i = 0; i < nFeatures; ++i)
            read_feature(fv, strValues[i], *fiSet[i], lineCnt, fiSet);
        fv.write(protocol.get());
    } // while

    transport->finish();

    // build index
    uint32_t idx = 0;
    for (auto &pf : fiSet.arrFeature()) {
        if (!pf->isKeep()) continue;
        for (auto &subftKv : pf->subFeatures())
            subftKv.second.setIndex(idx++);
    } // for
}


#if 0
void load_data(const std::string &fname, Example &exp)
{
    using namespace std;

    ifstream ifs(fname, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "load_data cannot open " << fname << " for reading!");

    const size_t nFeatures = g_ftInfoSet.size();

    ExampleHandle hExp(exp);
    hExp.clearVector();

    string line;
    size_t lineCnt = 0;
    while (getline(ifs, line)) {
        ++lineCnt;
        boost::trim_if(line, boost::is_any_of(g_strSep + SPACES));  // NOTE!!! 整行trim应该去掉分割符和默认trim空白字符
        if (line.empty()) continue;
        vector<string> strValues;
        boost::split(strValues, line, boost::is_any_of(g_strSep), boost::token_compress_on);
        // for (auto &s : strValues)
            // cout << s << endl;
        THROW_RUNTIME_ERROR_IF(strValues.size() != nFeatures,
                "Error when processing data file in line " << lineCnt 
                << ", nFeatures not match! " << nFeatures << " expected but " 
                << strValues.size() << " detected.");
        FeatureVector fv;
        for (size_t i = 0; i < nFeatures; ++i)
            read_feature(fv, strValues[i], *g_ftInfoSet[i], lineCnt);
        hExp.addVector(std::move(fv));
    } // while

    uint32_t idx = 0;
    for (auto &pf : fiSet.arrFeature()) {
        for (auto &subft : pf->subFeatures())
            subft.setIndex(idx++);
    } // for
}
#endif


