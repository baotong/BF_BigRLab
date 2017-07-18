#include "common.hpp"
#include "WorkInfo.h"
#include <csignal>
#include <cstdio>
#include <chrono>
#include <cstdlib>
#include <sys/wait.h>
#include <errno.h>
#include <glog/logging.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>


#define READ   0
#define WRITE  1

static
FILE* popen2(const std::string &command, const std::string &type, int &pid)
{
    pid_t child_pid;
    int fd[2];

    if (pipe(fd) == -1)
        THROW_RUNTIME_ERROR("Create pipe error " << strerror(errno));

    if((child_pid = fork()) == -1)
        THROW_RUNTIME_ERROR("Fork child process error " << strerror(errno));

    /* child process */
    if (child_pid == 0)
    {
        if (type == "r")
        {
            close(fd[READ]);    //Close the READ end of the pipe since the child's fd is write-only
            dup2(fd[WRITE], 1); //Redirect stdout to pipe
        }
        else
        {
            close(fd[WRITE]);    //Close the WRITE end of the pipe since the child's fd is read-only
            dup2(fd[READ], 0);   //Redirect stdin to pipe
        }

        setpgid(child_pid, child_pid); //Needed so negative PIDs can kill children of /bin/sh
        execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
        exit(0);
    }
    else
    {
        if (type == "r")
        {
            close(fd[WRITE]); //Close the WRITE end of the pipe since parent's fd is read-only
        }
        else
        {
            close(fd[READ]); //Close the READ end of the pipe since parent's fd is write-only
        }
    }

    pid = child_pid;

    if (type == "r")
    {
        return fdopen(fd[READ], "r");
    }

    return fdopen(fd[WRITE], "w");
}


static
int pclose2(FILE * fp, pid_t pid)
{
    int stat;

    fclose(fp);
    while (waitpid(pid, &stat, 0) == -1)
    {
        if (errno != EINTR)
        {
            stat = -1;
            break;
        }
    }

    return stat;
}


void WorkInfo::runCmd()
{
    DLOG(INFO) << "Running cmd: " << m_strCmd;

    FILE *fp = popen2(m_strCmd, "r", m_nPid);
    ::setvbuf(fp, NULL, _IONBF, 0);

    typedef boost::iostreams::stream< boost::iostreams::file_descriptor_source >
        FDRdStream;
    FDRdStream ppStream( fileno(fp), boost::iostreams::never_close_handle );

    std::ostringstream ss;
    ss << ppStream.rdbuf();

    m_strAppOutput = ss.str();

    m_nAppRetval = pclose2(fp, m_nPid);
    m_nAppRetval = WEXITSTATUS(m_nAppRetval);
}


void FeatureWorkInfo::init(const Json::Value &conf)
{
    m_strInput = conf["infile"].asString();
    THROW_RUNTIME_ERROR_IF(m_strInput.empty(), "Input file not specified!");
}


void FeatureWorkInfo::run()
{
    // for debug
    m_strCmd = "./test.sh 2>&1";
    m_pWorkThr.reset(new std::thread(&FeatureWorkInfo::workThreadRoutine, this));
    m_pWorkThr->detach();
    m_nStatus = RUNNING;
}


void FeatureWorkInfo::workThreadRoutine()
try {
    auto tp1 = std::chrono::high_resolution_clock::now();
    runCmd();
    auto tp2 = std::chrono::high_resolution_clock::now();
    m_nDuration = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1).count();
    m_nStatus = FINISH;
} catch (const std::exception &ex) {
    m_strAppOutput = "Exception caught: ";
    m_strAppOutput.append(ex.what());
    m_nAppRetval = -1;
    m_nStatus = FINISH;
}


