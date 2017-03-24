/*
 * c++ -o /tmp/test gen_json.cpp -lglog -ljsoncpp -std=c++11 -g
 */
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <json/json.h>
#include <glog/logging.h>

#define SPACES                " \t\f\r\v\n"

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x << std::flush; \
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


struct FeatureInfo {
    typedef std::shared_ptr<FeatureInfo>  pointer;
    std::string                 name;
    std::string                 type;
    std::vector<std::string>    values;
};

std::ostream& operator << (std::ostream &os, const FeatureInfo &f)
{
    os << "name: " << f.name << std::endl;
    os << "type: " << f.type << std::endl;
    if (f.values.size()) {
        os << "values: ";
        for (auto &v : f.values)
            os << v << " ";
        os << std::endl;
    } // for
    return os;
}

static
void read_conf(const std::string &confFile, std::vector<FeatureInfo::pointer> &fields)
{
    using namespace std;

    ifstream ifs(confFile, ios::in);

    RET_MSG_IF(!ifs, "Cannot open " << confFile << " for reading!");

    fields.clear();

    string line, name, type;
    FeatureInfo::pointer pField;
    size_t lineCnt = 0;

    while (getline(ifs, line)) {
        ++lineCnt;
        istringstream iss(line);
        iss >> name >> type;
        THROW_RUNTIME_ERROR_IF(iss.fail() || iss.bad(), 
                "Read name or type error in line " << lineCnt);
        boost::trim_if(name, boost::is_any_of(":," SPACES));
        boost::trim_if(type, boost::is_any_of(":," SPACES));
        pField = std::make_shared<FeatureInfo>();
        pField->name = name; pField->type = type;
        fields.emplace_back(pField);

        string strValues;
        vector<string> values;
        getline(iss, strValues);
        boost::trim_if(strValues, boost::is_any_of(SPACES));
        if (strValues.empty()) continue;
        boost::split(values, strValues, boost::is_any_of("," SPACES), boost::token_compress_on);
        pField->values.swap(values);
    } // while
}


static
void gen_json(const std::string &filename, const std::vector<FeatureInfo::pointer> &fields)
{
    using namespace std;

    Json::Value root;
    root["nfeatures"] = (Json::UInt)(fields.size());
    root["sep"] = "," SPACES;

    for (const auto &pf : fields) {
        Json::Value fItem;
        fItem["name"] = pf->name;
        fItem["type"] = pf->type;
        for (const string &v : pf->values)
            fItem["values"].append(v);
        auto &back = root["features"].append(Json::Value());
        back.swap(fItem);
    } // for

    Json::StyledWriter writer;  
    string outStr = writer.write(root);

    ofstream ofs(filename, ios::out);
    THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open " << filename << " for writting!");
    ofs << outStr << flush;
}


int main(int argc, char **argv)
{
    using namespace std;

    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " confFile jsonFile" << endl;
        return -1;
    } // if

    try {
        vector<FeatureInfo::pointer>  fields;
        read_conf(argv[1], fields);
        // DLOG(INFO) << fields.size();
        // for (auto &v : fields)
            // cout << *v << endl;
        gen_json(argv[2], fields);

    } catch (const std::exception &ex) {
        cerr << ex.what() << endl;
    } // try

    return 0;
}

