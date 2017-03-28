#include <fstream>
#include <glog/logging.h>
#include <json/json.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include "CommDef.h"
#include "Feature.h"

std::vector<FeatureInfo::pointer>    g_arrFeatureInfo;
std::string                          g_strSep = SPACES;


void load_feature_info(const std::string &fname)
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

    g_arrFeatureInfo.clear();

    // read nFeatures
    {
        Json::Value &jv = root["nfeatures"];
        THROW_RUNTIME_ERROR_IF(!jv, "No attr \"nFeatures\" found in config!");
        nFeatures = jv.asUInt();
        THROW_RUNTIME_ERROR_IF(!nFeatures, "Value of attr \"nFeatures\" " << nFeatures << " is invalid!");
    } // read nFeatures

    g_arrFeatureInfo.reserve(nFeatures);

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
            auto pf = std::make_shared<FeatureInfo>(jf["name"].asString(), jf["type"].asString());
            THROW_RUNTIME_ERROR_IF(!validTypes.count(pf->type),
                    "Feature " << pf->name << " has invalid type " << pf->type);
            g_arrFeatureInfo.emplace_back(pf);

            auto &jMulti = jf["multi"];
            if (!!jMulti) pf->multi = jMulti.asBool();
            // DLOG(INFO) << pf->name << " " << pf->multi;
            auto &jSep = jf["sep"];
            if (!!jSep) pf->sep = jSep.asString();

            auto &jValues = jf["values"];
            if (!!jValues) {
                for (auto &jv : jValues)
                    pf->addValue(jv.asString());
            } // if
        } // for jf
    } // read features
}

static
void read_string_feature(FeatureVector &fv, std::string &strField, 
            const FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    boost::trim_if(strField, boost::is_any_of(ftInfo.sep + SPACES));
    LOG_IF(WARNING, strField.empty()) << "read_string_feature in line " << lineno
            << " empty feature " << ftInfo.name;

    FeatureVectorHandle hFv(fv);

    if (ftInfo.multi) {
        vector<string> strValues;
        const string sep = (ftInfo.sep.empty() ? SPACES : ftInfo.sep);
        boost::split(strValues, strField, boost::is_any_of(sep), boost::token_compress_on);
        for (auto &v : strValues) {
            boost::trim(v);
            THROW_RUNTIME_ERROR_IF(!ftInfo.values.empty() && !ftInfo.values.count(v),
                    "read_string_feature in line " << lineno << " for feature \""
                    << ftInfo.name << "\", " << 
                    boost::format("\"%s\" is not a valid value!") % v);
            hFv.addFeature(ftInfo.name, v);
        } // for
    } else {
        THROW_RUNTIME_ERROR_IF(!ftInfo.values.empty() && !ftInfo.values.count(strField),
                "read_string_feature in line " << lineno << " for feature \""
                << ftInfo.name << "\", " << 
                boost::format("\"%s\" is not a valid value!") % strField);
        hFv.setFeature(ftInfo.name, strField);
    } // if
}

static
void read_double_feature(FeatureVector &fv, std::string &strField, 
            const FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    boost::trim(strField);
    THROW_RUNTIME_ERROR_IF(strField.empty(), "read_double_feature in line "
            << lineno << ", empty field of \"" << ftInfo.name << "\"!");
    
    FeatureVectorHandle hFv(fv);

    if (ftInfo.multi) {
        vector<string> strValues;
        const string sep = (ftInfo.sep.empty() ? SPACES : ftInfo.sep);
        boost::split(strValues, strField, boost::is_any_of(sep), boost::token_compress_on);
        for (auto &v : strValues) {
            boost::trim(v);
            auto pos = v.rfind('=');
            THROW_RUNTIME_ERROR_IF(string::npos == pos,
                    "read_double_feature in line " << lineno << " for feature \""
                    << ftInfo.name << "\" wrong format!");
            string key(v, 0, pos);
            string value(v, pos+1);
            boost::trim(key); boost::trim(value);
            THROW_RUNTIME_ERROR_IF(!ftInfo.values.empty() && !ftInfo.values.count(key),
                    "read_double_feature in line " << lineno << " for feature \""
                    << ftInfo.name << "\", " << 
                    boost::format("\"%s\" is not a valid value!") % key);
            double val = 0.0;
            THROW_RUNTIME_ERROR_IF(!boost::conversion::try_lexical_convert(value, val),
                    "read_double_feature in line " << lineno << " for feature \""
                    << ftInfo.name << "\" cannot covert \"" << value << "\" to double!");
            hFv.setFeature(val, ftInfo.name, key);
        } // for
    } else {
        double val = 0.0;
        THROW_RUNTIME_ERROR_IF(!boost::conversion::try_lexical_convert(strField, val),
                "read_double_feature in line " << lineno << " for feature \""
                << ftInfo.name << "\" cannot covert \"" << strField << "\" to double!");
        hFv.setFeature(val, ftInfo.name);
    } // if
}

static
void read_list_double_feature(FeatureVector &fv, std::string &strField, 
            const FeatureInfo &ftInfo, const std::size_t lineno)
{
    using namespace std;

    boost::trim_if(strField, boost::is_any_of(ftInfo.sep + SPACES));
    THROW_RUNTIME_ERROR_IF(strField.empty(), "read_list_double_feature in line "
            << lineno << ", empty field of \"" << ftInfo.name << "\"!");

    FeatureVectorHandle hFv(fv);

    vector<string> strValues;
    const string sep = (ftInfo.sep.empty() ? SPACES : ftInfo.sep);
    boost::split(strValues, strField, boost::is_any_of(sep), boost::token_compress_on);
    vector<double> values(strValues.size(), 0.0);

    for (size_t i = 0; i < values.size(); ++i) {
        THROW_RUNTIME_ERROR_IF(!boost::conversion::try_lexical_convert(strValues[i], values[i]),
                "read_list_double_feature in line " << lineno << " for feature \""
                << ftInfo.name << "\" cannot covert \"" << strValues[i] << "\" to double!");
    } // for

    hFv.setFeature(ftInfo.name, values);
}

static
void read_feature(FeatureVector &fv, std::string &strField, 
            const FeatureInfo &ftInfo, const std::size_t lineno)
{
    if (ftInfo.type == "string") {
        read_string_feature(fv, strField, ftInfo, lineno);
    } else if (ftInfo.type == "double") {
        read_double_feature(fv, strField, ftInfo, lineno);
    } else if (ftInfo.type == "list_double") {
        read_list_double_feature(fv, strField, ftInfo, lineno);
    } else {
        THROW_RUNTIME_ERROR("read_feature in line " << lineno << 
                ", feature type \"" << ftInfo.type << "\" is invalid!");
    } // if
}


void load_data(const std::string &fname, Example &exp)
{
    using namespace std;

    ifstream ifs(fname, ios::in);
    THROW_RUNTIME_ERROR_IF(!ifs, "load_data cannot open " << fname << " for reading!");

    const size_t nFeatures = g_arrFeatureInfo.size();

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
            read_feature(fv, strValues[i], *g_arrFeatureInfo[i], lineCnt);
        hExp.addVector(std::move(fv));
    } // while
}


