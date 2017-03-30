/*
 * ./test.bin ../data/adult.data ../data/adult_conf.json ../data/out.txt
 * ./test.bin ../data/search_no_id.data ../data/search_conf.json ../data/out.txt
 */
/*
 * trim sep+SPACES, split only sep
 */
#include <fstream>
#include <boost/format.hpp>
#include <glog/logging.h>
#include "CommDef.h"
#include "Feature.h"


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


// print one cell
static
void print_xgboost(std::ostream &os, FeatureVector &fv, FeatureInfo &fi)
{
    if (fi.type == "string") {
        for (auto &s : fv.stringFeatures[fi.name]) {
            auto it = fi.index.find(s);
            THROW_RUNTIME_ERROR_IF(it == fi.index.end(), 
                    "value \"" << s << "\" is invalid!");
            auto idx = it->second;
            os << idx << ":1 ";
        } // for
    } else if (fi.type == "double" || fi.type == "datetime") {
        for (auto &kv : fv.floatFeatures[fi.name]) {
            auto it = fi.index.find(kv.first);
            THROW_RUNTIME_ERROR_IF(it == fi.index.end(), 
                    "value \"" << kv.first << "\" is invalid!");
            auto idx = it->second;
            os << idx << ":" << kv.second << " "; 
        } // for
    } else {
        THROW_RUNTIME_ERROR("print_xgboost Invalid type!");
    } // if
}

static
void convert_xgboost(Example &exp, const std::string &fname)
{
    using namespace std;

    ofstream ofs(fname, ios::out);
    THROW_RUNTIME_ERROR_IF(!ofs, "Cannot open file " << fname << " for writting!");

    for (size_t i = 0; i < exp.example.size(); ++i) {
        FeatureVector &fv = exp.example[i];
        for (auto &pfi : g_arrFeatureInfo)
            print_xgboost(ofs, fv, *pfi);
        ofs << endl;
    } // for
}



int main(int argc, char **argv)
try {
    using namespace std;

    RET_MSG_VAL_IF(argc < 4, -1,
            "Usage: " << argv[0] << " dataFile confFile outFile");

    const char *dataFilename = argv[1];
    const char *confFilename = argv[2];
    const char *outFilename = argv[3];

    load_feature_info(confFilename);
    // Test::print_feature_info();
    // Test::test_feature_op();

    Example exp;
    load_data(dataFilename, exp);
    // cout << exp << endl;
    convert_xgboost(exp, outFilename);

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught in main: " << ex.what() << std::endl;
    return -1;
}




