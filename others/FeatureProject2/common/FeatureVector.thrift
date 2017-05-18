/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators FeatureVector.thrift
 */

namespace * FeatureProject

struct FeatureVector {
    1: optional string                            id;
    2: optional map<string, set<string>>          stringFeatures;
    3: optional map<string, map<string, double>>  floatFeatures;
    4: optional map<string, list<double>>         denseFeatures;
}


/*
 * NOTE!!! After gen src code, must change the constructor as:
 *   FloatInfo() : index(0), 
 *         minVal(std::numeric_limits<double>::max()), 
 *         maxVal(std::numeric_limits<double>::min()) {
 *   }
 * in XX_types.h
 */
struct FloatInfo {
    1: i32       index;
    2: double    minVal;
    3: double    maxVal;
}

struct DenseInfo {
    1: i32      startIdx;
    2: i32      len;
}

struct FeatureIndex {
    1: optional map<string, map<string, i32>>           stringIndices;
    2: optional map<string, map<string, FloatInfo>>     floatInfo;
    3: optional map<string, DenseInfo>                  denseInfo;
}

