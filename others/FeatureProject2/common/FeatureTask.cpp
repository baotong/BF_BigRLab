#include <fstream>
#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <dlfcn.h>
#include "FeatureTask.h"


// 要求函数指针名和库中的函数sym名一样
#define LOAD_FUNC( handle, name ) \
    do { \
        char *__load_func_error_str = 0; \
        *(void **) (&(name)) = ::dlsym(handle, #name); \
        if ((__load_func_error_str = ::dlerror()) != NULL) { \
            ::dlclose(handle); \
            handle = NULL; \
            THROW_RUNTIME_ERROR( "Cannot find symbol " << #name << " in lib file!" ); \
        } \
    } while(0) 


namespace FeatureProject {

using namespace std;

FeatureTaskMgr::TaskLib::~TaskLib()
{
    if (pHandle) {
        DLOG(INFO) << "Unload lib " << path;
        ::dlclose(pHandle);
        pHandle = NULL;
    } // if
}


void FeatureTaskMgr::TaskLib::loadLib()
{
    DLOG(INFO) << "Loading lib " << path;

    ::dlerror();
    pHandle = ::dlopen(path.c_str(), RTLD_LAZY);
    THROW_RUNTIME_ERROR_IF(!pHandle, "FeatureTaskMgr cannot load lib " << ::dlerror());
    ::dlerror();

    NewInstFunc create_instance;
    LOAD_FUNC(pHandle, create_instance);
    pNewInstFn = create_instance;
}


FeatureTask::pointer FeatureTaskMgr::TaskLib::newInstance( const std::string &name, FeatureTaskMgr *pMgr )
{
    FeatureTask::pointer ret;
    if (pNewInstFn)
        ret.reset(pNewInstFn(name, pMgr)); 
    return ret;
}


void FeatureTaskMgr::loadConf(const std::string &fname)
{
    DLOG(INFO) << "FeatureTaskMgr::loadConf() " << fname;

    // read json file
    {
        ifstream ifs(fname, ios::in);
        THROW_RUNTIME_ERROR_IF(!ifs, "FeatureTaskMgr::loadConf() cannot open " << fname << " for reading!");

        ostringstream oss;
        oss << ifs.rdbuf() << flush;

        Json::Reader    reader;
        THROW_RUNTIME_ERROR_IF(!reader.parse(oss.str(), m_jsConf),
                "FeatureTaskMgr::loadConf() Invalid json format!");
    } // read json file

    // datadir
    {
        const string &str = m_jsConf["datadir"].asString();
        if (!str.empty())
            m_strDataDir = str;
    } // datadir

    // autoremove default false
    m_bGlobalAutoRemove = m_jsConf["autoremove"].asBool();

    // hasId default true
    {
        auto &jv = m_jsConf["hasid"];
        if (!!jv) m_bGlobalHasId = jv.asBool();
    } // hasid
}


void FeatureTaskMgr::start()
{
    DLOG(INFO) << "FeatureTaskMgr::start()";

    const auto &jsTaskArr = m_jsConf["tasks"];
    THROW_RUNTIME_ERROR_IF(!jsTaskArr, "No task assigned!");

    for (Json::ArrayIndex i = 0; i < jsTaskArr.size(); ++i) {
        const auto &jsTask = jsTaskArr[i];
        const string &strName = jsTask["name"].asString();
        THROW_RUNTIME_ERROR_IF(strName.empty(), "name not specified in task " << (i+1));
        const string &strLib = jsTask["lib"].asString();
        THROW_RUNTIME_ERROR_IF(strLib.empty(), "lib not specified in task " << (i+1));

        TaskLib tLib(strLib);
        tLib.loadLib();
        auto pTask = tLib.newInstance(strName, this);
        pTask->init( jsTask );
        LOG(INFO) << "Processing task " << pTask->name() << "...";
        pTask->run();
        LOG(INFO) << "Task " << pTask->name() << " done!";
        LOG(INFO) << endl;
    } // for i

    LOG(INFO) << "All tasks done successfully!";
}


std::string FeatureTask::gen_tmp_output(const std::string &baseName)
{
    using namespace std;

    static uint32_t     no = 0;

    string name = baseName;
    std::replace_if(name.begin(), name.end(), boost::is_any_of(SPACES), '_');
    ostringstream oss;
    oss << name << "_" << ++no << ".out" << flush;

    return oss.str();
}


void FeatureTask::remove_file(const std::string &path)
{
    namespace fs = boost::filesystem;

    boost::system::error_code ec;
    try {
        fs::remove(path, ec);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Cannot remove " << path << " " << ec;
    } // try
}


void FeatureTask::init(const Json::Value &conf)
{
    namespace fs = boost::filesystem;

    DLOG(INFO) << "FeatureTask " << name() << " init...";

    fs::path dataPath(m_pTaskMgr->dataDir());

    m_strInput = conf["input"].asString();
    if (m_strInput.empty())
        m_strInput = m_pTaskMgr->lastOutput();
    else
        m_strInput = (dataPath / m_strInput).c_str();

    m_strOutput = conf["output"].asString();
    if (m_strOutput.empty())
        m_strOutput = gen_tmp_output(name());
    m_strOutput = (dataPath / m_strOutput).c_str();

    // auto remove
    {
        const auto &jv = conf["autoremove"];
        if (!jv) m_bAutoRemove = m_pTaskMgr->globalAutoRemove();
        else m_bAutoRemove = jv.asBool();
    } // auto remove

    // hasid
    {
        const auto &jv = conf["hasid"];
        if (!jv) m_bHasId = m_pTaskMgr->globalHasId();
        else m_bHasId = jv.asBool();
    } // hasid

    m_pFeatureInfoSet = m_pTaskMgr->globalDesc();
}


} // namespace FeatureProject

