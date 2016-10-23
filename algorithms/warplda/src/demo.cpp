#define STRIP_FLAG_HELP 0
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sstream>
#include <fstream>
#include <memory>
#include "Vocab.hpp"
#include "Bigraph.hpp"
#include "Utils.hpp"
#include "warplda.hpp"

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

DEFINE_int32(niter, 10, "number of iterations");
DEFINE_int32(k, 1000, "number of topics");
DEFINE_int32(mh, 1, "number of Metropolis-Hastings steps");
DEFINE_int32(ntop, 10, "num top words per each topic");
DEFINE_string(bin, "", "binary file");
DEFINE_string(model, "", "model file");
DEFINE_string(info, "", "info");
DEFINE_string(vocab, "", "vocabulary file");
DEFINE_string(topics, "", "topic assignment file");
DEFINE_string(z, "", "Z file name");
DEFINE_bool(estimate, false, "estimate model parameters");
DEFINE_bool(inference, false, "inference latent topic assignments");
DEFINE_bool(writeinfo, true, "write info");
DEFINE_bool(dumpmodel, true, "dump model");
DEFINE_bool(dumpz, true, "dump Z");
DEFINE_int32(perplexity, -1, "Interval to evaluate perplexity. -1 for don't evaluate.");
DEFINE_int32(skip, 2, "skip num of words at first of each line (only for text)");


static std::unique_ptr<Vocab>   g_pVocab;
static std::unique_ptr<LDA>     g_pLda;

static
void service_init()
{
    using namespace std;

    // check vocab
    {
        ifstream ifs(FLAGS_vocab, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open vocab file " << FLAGS_vocab);
    }
    g_pVocab.reset(new Vocab);
    g_pVocab->load(FLAGS_vocab);

    // check model
    {
        ifstream ifs(FLAGS_model, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open model file " << FLAGS_model);
    }
    LOG(INFO) << "Loading model " << FLAGS_model;
    g_pLda.reset(new WarpLDA<1>());
    g_pLda->loadModel(FLAGS_model);
}


static
void parse_document(const std::string &line, std::vector<TVID> &v)
{
    std::istringstream sin(line);
    std::string w;
    v.clear();
    for (int i = 0; sin >> w; i++) {
        if (i >= FLAGS_skip) {
            int vid = g_pVocab->getIdByWord(w);
            if (vid != -1) v.push_back(vid);
        } // if
    } // for
}

void text_to_bin(const std::string &text, 
                 std::ostream &uIdxStream,
                 std::ostream &vIdxStream,
                 std::ostream &uLnkStream,
                 std::ostream &vLnkStream)
{
    int doc_id = 0;     // TODO meaningless
    std::vector<std::pair<TUID, TVID>>  edge_list;   // TUID TVID all uint32_t
    std::vector<TVID>                   vlist;

    parse_document(text, vlist);
    for (auto word_id : vlist)
        edge_list.emplace_back(doc_id, word_id);
    ++doc_id;

    // Shuffle tokens
    std::vector<TVID> new_vid(g_pVocab->nWords());
    for (unsigned i = 0; i < new_vid.size(); ++i)
        new_vid[i] = i;
    g_pVocab->RearrangeId(new_vid.data()); // TODO NOTE!!! useful? 要改变vocab的, not thread-safe

    for (auto &e : edge_list)
        e.second = new_vid[e.second];

    Bigraph::Generate(uIdxStream, vIdxStream, 
            uLnkStream, vLnkStream, edge_list, g_pVocab->nWords());      // 生成输出文件
}

namespace Test {
using namespace std;
void test1()
{
    string line;
    while (getline(cin, line)) {
        // TODO ios::binary
        stringstream uIdxStream(ios::in | ios::out | ios::binary), 
                     vIdxStream(ios::in | ios::out | ios::binary), 
                     uLnkStream(ios::in | ios::out | ios::binary), 
                     vLnkStream(ios::in | ios::out | ios::binary);
        text_to_bin(line, uIdxStream, vIdxStream, uLnkStream, vLnkStream);
        g_pLda->loadBinary(uIdxStream, vIdxStream, uLnkStream, vLnkStream);
        g_pLda->inference(FLAGS_niter, FLAGS_perplexity);
        g_pLda->storeZ(cout);
    } // while
}
} // namespace Test


int main(int argc, char **argv)
{
    using namespace std;

    try {
        google::InitGoogleLogging(argv[0]);
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        LOG(INFO) << argv[0] << " started...";

        service_init();

        Test::test1();
        return 0;

        
    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        return -1;
    } // try


    return 0;
}



#if 0
int main(int argc, char** argv)
{
    using namespace std;

    // google::InitGoogleLogging(argv[0]);
    gflags::SetUsageMessage("Usage : ./warplda [ flags... ]");
	gflags::ParseCommandLineFlags(&argc, &argv, true);

    if ((FLAGS_inference || FLAGS_estimate) == false)
        FLAGS_estimate = true;
    if (!FLAGS_z.empty())
        FLAGS_dumpz = true;

    SetIfEmpty(FLAGS_bin, FLAGS_prefix + ".bin");
    SetIfEmpty(FLAGS_model, FLAGS_prefix + ".model");
    SetIfEmpty(FLAGS_info, FLAGS_prefix + ".info");
    SetIfEmpty(FLAGS_vocab, FLAGS_prefix + ".vocab");
    SetIfEmpty(FLAGS_topics, FLAGS_prefix + ".topics");

    print_args();

    LDA *lda = new WarpLDA<1>();
    lda->loadBinary(FLAGS_bin);     // 加载数据
    if (FLAGS_estimate)
    {
        cerr << "DBG Doing estimate..." << endl; // dbg 预测不做
        lda->estimate(FLAGS_k, FLAGS_alpha / FLAGS_k, FLAGS_beta, FLAGS_niter, FLAGS_perplexity);
        if (FLAGS_dumpmodel)
        {
            std::cout << "Dump model " << FLAGS_model << std::endl;
            lda->storeModel(FLAGS_model);
        }
        if (FLAGS_writeinfo)
        {
            std::cout << "Write Info " << FLAGS_info << " ntop " << FLAGS_ntop << std::endl;
            lda->writeInfo(FLAGS_vocab, FLAGS_info, FLAGS_ntop);
        }
        if (FLAGS_dumpz)
        {
            SetIfEmpty(FLAGS_z, FLAGS_prefix + ".z.estimate");
            std::cout << "Dump Z " << FLAGS_z << std::endl;
            lda->storeZ(FLAGS_z);
        }
    }
    else if(FLAGS_inference)
    {
        cerr << "DBG Doing inference..." << endl; // dbg  predict run this
        lda->loadModel(FLAGS_model);
        lda->inference(FLAGS_niter, FLAGS_perplexity);
        if (FLAGS_dumpz)
        {
            SetIfEmpty(FLAGS_z, FLAGS_prefix + ".z.inference");
            std::cout << "Dump Z " << FLAGS_z << std::endl;
            lda->storeZ(FLAGS_z);
        }
    }
	return 0;
}
#endif
