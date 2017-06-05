/*
 * GLOG_logtostderr=1 ./semantic_tree.bin -f ../data/rules/entertainment.json
 */
#include "CommDef.h"
#include "SemanticTree.h"
#include <glog/logging.h>
#include <gflags/gflags.h>


DEFINE_string(f, "", "Tree file");


namespace Test {
using namespace std;
void count_node(SemanticTreeSet *pTreeSet)
{
    size_t sum = 0;
    for (const auto &kv : pTreeSet->trees())
        sum += kv.second->count();
    cout << sum << endl;
}
} // namespace Test


int main(int argc, char **argv)
try {
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    THROW_RUNTIME_ERROR_IF(FLAGS_f.empty(), "tree file -f not specified!");

    auto pTreeSet = std::make_shared<SemanticTreeSet>();
    pTreeSet->loadFromJson(FLAGS_f);

    // Test::count_node(pTreeSet.get());
    // exit(0);
    
    pTreeSet->print(cout);

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught by main: " << ex.what() << std::endl;
    return -1;
}



