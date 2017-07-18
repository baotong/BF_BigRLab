#ifndef _WORK_INFO_H_
#define _WORK_INFO_H_

#include <memory>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <json/json.h>


class WorkInfo {
public:
    enum Status {READY, RUNNING, FINISH};
    typedef std::shared_ptr<WorkInfo>      pointer;
public:
    WorkInfo(const std::string &_Name) 
            : m_strName(_Name), m_nPid(0), m_nStatus(READY) {}
    virtual ~WorkInfo() = default;

    const std::string& name() const { return m_strName; }
    int status() const { return m_nStatus; }
    int retcode() const { return m_nAppRetval; }
    const std::string& output() const { return m_strAppOutput; }
    uint64_t duration() const { return m_nDuration; }

    virtual void init(const Json::Value &conf) = 0;
    virtual void run() = 0;
    void runCmd();
    void kill();
protected:
    std::string     m_strName;
    std::string     m_strCmd;
    std::string     m_strAppOutput;
    pid_t           m_nPid;
    int             m_nAppRetval;
    uint64_t        m_nDuration = 0;
    std::atomic_int                 m_nStatus;
    std::unique_ptr<std::thread>    m_pWorkThr;
};


class FeatureWorkInfo : public WorkInfo {
public:
    FeatureWorkInfo(const std::string &_Name)
            : WorkInfo(_Name) {}
    virtual void init(const Json::Value &conf);
    virtual void run();
    void workThreadRoutine();

private:
    std::string     m_strInput;
};

#endif /* _WORK_INFO_H_ */

