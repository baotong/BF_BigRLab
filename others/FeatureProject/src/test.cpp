/*
 * c++ -o /tmp/test test.cpp -lglog -ljsoncpp -std=c++11 -g
 * ./test.bin ../data/adult.data ../data/adult_conf.json
 */
/*
 * trim sep+SPACES, split only sep
 */
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



int main(int argc, char **argv)
try {
    using namespace std;

    RET_MSG_VAL_IF(argc < 3, -1,
            "Usage: " << argv[0] << " dataFile confFile");

    const char *dataFilename = argv[1];
    const char *confFilename = argv[2];

    load_feature_info(confFilename);
    // Test::print_feature_info();
    // Test::test_feature_op();

    Example exp;
    load_data(dataFilename, exp);
    cout << exp << endl;

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught in main: " << ex.what() << std::endl;
    return -1;
}




