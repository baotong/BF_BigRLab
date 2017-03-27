/*
 * c++ -o /tmp/test test.cpp -lglog -ljsoncpp -std=c++11 -g
 * ./test.bin ../data/adult.data ../data/adult_conf.json
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <json/json.h>
#include <glog/logging.h>
#include "FeatureHandle.hpp"
#include "FeatureInfo.h"

#define SPACES                " \t\f\r\v\n"

#define THROW_RUNTIME_ERROR(args) \
    do { \
        std::ostringstream __err_stream; \
        __err_stream << args << std::flush; \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

#define THROW_RUNTIME_ERROR_IF(cond, args) \
    do { \
        if (cond) THROW_RUNTIME_ERROR(args); \
    } while (0)

#define RET_MSG(args) \
    do { \
        std::ostringstream __err_stream; \
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
        std::ostringstream __err_stream; \
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

    void test_feature_op()
    {
        FeatureVector fv;
        FeatureVectorHandle fop(fv);
        fop.setFeature("name", "Jhonason");
        fop.setFeature("gender", "Male");
        fop.setFeature(30.0, "age");
        fop.addFeature("skill", "Java");
        fop.addFeature("skill", "Python");
        fop.addFeature("skill", "Database");
        cout << fv << endl;

        fop.setFeature("name", "Lucy");
        fop.setFeature("gender", "female");
        fop.setFeature(26.0, "age");
        fop.setFeature(5.0, "score", "Math");
        fop.setFeature(4.0, "score", "Computer");
        fop.setFeature(3.5, "score", "Art");
        fop.setFeature(4.3, "score", "Spanish");
        cout << fv << endl;
        fop.setFeature(4.5, "score", "Spanish");
        cout << fv << endl;

        bool ret = false;
        string strVal;
        double fVal = 0.0;
        ret = fop.getFeatureValue("name", strVal);
        cout << (ret ? strVal : "Not found!") << endl;
        ret = fop.getFeatureValue(fVal, "age");
        if (ret) cout << fVal << endl;
        else cout << "Not found!" << endl;
        ret = fop.getFeatureValue(fVal, "score", "Computer");
        if (ret) cout << fVal << endl;
        else cout << "Not found!" << endl;
        ret = fop.getFeatureValue("Location", strVal);
        cout << (ret ? strVal : "Not found!") << endl;
        ret = fop.getFeatureValue(fVal, "score", "Chinese");
        if (ret) cout << fVal << endl;
        else cout << "Not found!" << endl;
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

static
void read_feature(FeatureVector &fv, std::string &strField, const FeatureInfo &ftInfo)
{

}

static
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
        THROW_RUNTIME_ERROR_IF(strValues.size() != nFeatures,
                "Error when processing data file in line " << lineCnt 
                << ", nFeatures not match! " << nFeatures << " expected but " 
                << strValues.size() << " detected.");
        FeatureVector fv;
        for (size_t i = 0; i < nFeatures; ++i)
            read_feature(fv, strValues[i], *g_arrFeatureInfo[i]);
        hExp.addVector(std::move(fv));
    } // while
}


int main(int argc, char **argv)
try {
    RET_MSG_VAL_IF(argc < 3, -1,
            "Usage: " << argv[0] << " dataFile confFile");

    const char *dataFilename = argv[1];
    const char *confFilename = argv[2];

    load_feature_info(confFilename);
    Test::print_feature_info();
    Test::test_feature_op();

    Example exp;
    load_data(dataFilename, exp);

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught in main: " << ex.what() << std::endl;
    return -1;
}




