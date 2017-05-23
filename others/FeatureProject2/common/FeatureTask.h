#ifndef _FEATURE_TASK_H_
#define _FEATURE_TASK_H_

#include "Feature.h"

namespace FeatureProject {

class FeatureTaskMgr;

class FeatureTask {
public:
    typedef std::shared_ptr<FeatureTask>    pointer;

public:
    FeatureTask( const std::string &_Name, FeatureTaskMgr *_Mgr )
            : m_pTaskMgr(_Mgr), m_strName(_Name), m_bAutoRemove(false), m_bHasId(true) {}
    virtual ~FeatureTask() = default;

    virtual void init(const Json::Value &conf);
    virtual void run() = 0;

    const std::string& name() const
    { return m_strName; }

    void setInput(const std::string &val)
    { m_strInput = val; }
    const std::string& input() const
    { return m_strInput; }

    void setOutput(const std::string &val)
    { m_strOutput = val; }
    const std::string& output() const
    { return m_strOutput; }

    bool autoRemove() const { return m_bAutoRemove; }
    void setAutoRemove(bool flag)
    { m_bAutoRemove = flag; }

    bool hasId() const { return m_bHasId; }

protected:
    static std::string gen_tmp_output(const std::string &baseName);
    static void remove_file(const std::string &path);
    static std::shared_ptr<FeatureIndex> load_info(const std::string &fname);

protected:  
    FeatureTaskMgr      *m_pTaskMgr;
    std::string         m_strName, m_strInput, m_strOutput;
    // std::shared_ptr<FeatureInfoSet>     m_pFeatureInfoSet;
    bool                m_bAutoRemove, m_bHasId;
};


class FeatureTaskMgr {
public:
    struct TaskLib {
        typedef FeatureTask* (*NewInstFunc)(const std::string&, FeatureTaskMgr*);

        TaskLib( const std::string &_Path )
                : path(_Path), pHandle(NULL), pNewInstFn(NULL) {}
        ~TaskLib();

        void loadLib();
        FeatureTask::pointer newInstance( const std::string &name, FeatureTaskMgr *pMgr );

        std::string    path;
        void           *pHandle;
        NewInstFunc    pNewInstFn;
    }; // TaskLib

public:
    FeatureTaskMgr() : m_strDataDir("."), 
                m_bGlobalAutoRemove(false), m_bGlobalHasId(true) {}

    void loadConf(const std::string &fname);
    void start();

    const std::string& dataDir() const
    { return m_strDataDir; }

    void setLastOutput(const std::string &val)
    { m_strLastOutput = val; }
    const std::string& lastOutput() const
    { return m_strLastOutput; }

    // void setGlobalDesc(const std::shared_ptr<FeatureInfoSet> &ptr)
    // { m_pFeatureInfoSet = ptr; }
    // std::shared_ptr<FeatureInfoSet> globalDesc() const
    // { return m_pFeatureInfoSet; }

    bool globalAutoRemove() const { return m_bGlobalAutoRemove; }
    bool globalHasId() const { return m_bGlobalHasId; }

private:
    Json::Value         m_jsConf;
    std::string         m_strDataDir;
    std::string         m_strLastOutput;    // 上一个task的输出文件
    bool                m_bGlobalAutoRemove, m_bGlobalHasId;
    // std::shared_ptr<FeatureInfoSet>     m_pFeatureInfoSet;
};


} // namespace FeatureProject


#endif /* _FEATURE_TASK_H_ */

