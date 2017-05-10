#include <fstream>
#include <glog/logging.h>
#include <dlfcn.h>
#include "FeatureTask.h"


using namespace std;

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
    } // for i
}


void FeatureTask::init(const Json::Value &conf)
{
    DLOG(INFO) << "FeatureTask " << name() << " init...";
    m_strInput = conf["input"].asString();
    if (m_strInput.empty())
        m_strInput = m_pTaskMgr->lastOutput();
    m_strOutput = conf["output"].asString();
}
