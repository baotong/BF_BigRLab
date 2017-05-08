#ifndef _FEATURE_TASK_H_
#define _FEATURE_TASK_H_

#include "Feature.h"

/*
 * TODO
 * autoremove
 * update global FeatureInfoSet
 */


class FeatureTaskMgr;

class FeatureTask {
public:
    typedef std::shared_ptr<FeatureTask>    pointer;
public:
    enum TaskType {OTHER, RAW2FV};

public:
    FeatureTask( const std::string &_Name, FeatureTaskMgr *_Mgr )
            : m_pTaskMgr(_Mgr), m_enumType(OTHER), m_strName(_Name) {}
    virtual ~FeatureTask() = default;

    virtual void init(const Json::Value &conf);
    virtual void run() = 0;

    void setType(TaskType tp)
    { m_enumType = tp; }
    TaskType type() const
    { return m_enumType; }

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

protected:  
    FeatureTaskMgr      *m_pTaskMgr;
    TaskType            m_enumType;
    std::string         m_strName, m_strInput, m_strOutput;
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
    FeatureTaskMgr() : m_strDataDir(".") {}

    void loadConf(const std::string &fname);
    void start();

    // void setDataDir(const std::string &val)
    // { m_strDataDir = val; }
    const std::string& dataDir() const
    { return m_strDataDir; }

    void setLastOutput(const std::string &val)
    { m_strLastOutput = val; }
    const std::string& lastOutput() const
    { return m_strLastOutput; }

    void setFeatureInfoSet(const std::shared_ptr<FeatureInfoSet> &ptr)
    { m_pFeatureInfoSet = ptr; }
    std::shared_ptr<FeatureInfoSet> featureInfoSet() const
    { return m_pFeatureInfoSet; }

private:
    Json::Value         m_jsConf;
    std::string         m_strDataDir;
    std::string         m_strLastOutput;    // 上一个task的输出文件
    std::shared_ptr<FeatureInfoSet>     m_pFeatureInfoSet;
};



#endif /* _FEATURE_TASK_H_ */

