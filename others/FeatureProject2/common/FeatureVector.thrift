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


struct FeatureIndex {
    1: map<string, map<string, i32>>    index;
}

