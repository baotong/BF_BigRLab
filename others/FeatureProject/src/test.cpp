/*
 * c++ -o /tmp/test test.cpp -lglog -ljsoncpp -std=c++11 -g
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <example_types.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <json/json.h>
#include <glog/logging.h>
#include "FeatureInfo.h"

#define SPACES                " \t\f\r\v\n"

#define THROW_RUNTIME_ERROR(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args << std::flush; \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

#define THROW_RUNTIME_ERROR_IF(cond, args) \
    do { \
        if (cond) THROW_RUNTIME_ERROR(args); \
    } while (0)

#define RET_MSG(args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args << std::flush; \
        std::cerr << __err_stream.str() << std::endl; \
        return; \
    } while (0)

#define RET_MSG_IF(cond, args) \
    do { \
        if (cond) RET_MSG(args); \
    } while (0)

#define RET_MSG_VAL(val, args) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << args << std::flush; \
        std::cerr << __err_stream.str() << std::endl; \
        return val; \
    } while (0)

#define RET_MSG_VAL_IF(cond, val, args) \
    do { \
        if (cond) RET_MSG_VAL(val, args); \
    } while (0)


#define VALID_TYPES     {"string", "double", "list_double"}


static std::vector<FeatureInfo::pointer>    g_arrFeatureInfo;
static std::string                          g_strSep = SPACES;

std::ostream& operator << (std::ostream &os, const FeatureInfo &fi)
{
    os << "name = " << fi.name << std::endl;
    os << "type = " << fi.type << std::endl;
    os << "multi = " << fi.multi << std::endl;
    if (!fi.sep.empty())
        os << "sep = " << fi.sep << std::endl;
    if (!fi.values.empty()) {
        os << "values = ";
        for (auto &v : fi.values)
            os << v << " ";
        os << std::endl;
    } // if
    return os;
}

namespace Test {
    using namespace std;
    void print_feature_info()
    {
        cout << "Totally " << g_arrFeatureInfo.size() << " features." << endl;
        cout << "Global seperator = " << g_strSep << endl;
        cout << endl;
        for (auto &pf : g_arrFeatureInfo)
            cout << *pf << endl;
    }
} // namespace Test


static
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


int main(int argc, char **argv)
try {
    RET_MSG_VAL_IF(argc < 3, -1,
            "Usage: " << argv[0] << " dataFile confFile");

    const char *dataFilename = argv[1];
    const char *confFilename = argv[2];

    load_feature_info(confFilename);
    Test::print_feature_info();

    (void)dataFilename;

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught in main: " << ex.what() << std::endl;
    return -1;
}




