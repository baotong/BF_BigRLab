/*
 * thrift --gen cpp:templates,pure_enums,moveable_types,no_default_operators example.thrift
 */

struct FeatureVector {
    1: optional string                            id;
    2: optional map<string, set<string>>          stringFeatures;
    3: optional map<string, map<string, double>>  floatFeatures;
    4: optional map<string, list<double>>         denseFeatures;
}


struct Example {
    1: optional list<FeatureVector>       example;
    2: optional FeatureVector             context;
    3: optional map<string, string>       metadata;
}

