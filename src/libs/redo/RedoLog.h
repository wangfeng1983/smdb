#ifndef _LINKAGE_MDB_REDO_REDOLOG_H_
#define _LINKAGE_MDB_REDO_REDOLOG_H_

#include <unistd.h>
#include <inttypes.h>

#include <string>
#include <vector>

#include "MdbConstDef2.h"
#include "LGWRRunStatus.h"
#include "RedoLogBuffer.h"

class LocalRedoBuffer;
class Lock;

/// 重做日志接口
class RedoLog
{
    public:
        /// REDO日志类型
        typedef enum _REDOLOG_TYPE
        {
            DDL = 1,
            TRANS_BEGIN,
            TRANS_COMMIT,
            TRANS_ROLLBACK,
            INSERT,
            UPDATE,
            DELETE
        } LogType;

        /// REDO 日志的配置信息
        typedef struct _REDO_CONFIG
        {
            std::string         dbname;             // MDB name
            std::string         path;               // 存放redo文件的路径

            int                 logFileSize;        // 单个 log 文件大小, 单位为M
            int                 logGroupCount;      // log文件组的数量
            int                 logBufferSize;      // redo log buffer 大小, 单位为K
            int                 logBufferCount;     // redo log buffer 数量

            int                 writeMode;          // 写入日志的方式
            void*               redoLogAddress;     // UNDO 表空间中 RedoLog 区的开始地址
        } RedoConfig;

    public:
        RedoLog();
        ~RedoLog();

        /** 初始化RedoLog对象
         * @param config REDO log 的配置信息
         * @return 初始化成功返回true; 否则返回false
         */
        bool initialize(const RedoConfig* config);

        /** 初始化 log 文件.
         * 如果 log 文件不存在，则创建所有 log 文件
         * 如果 log 文件已经存在，则清空所有 log 文件
         * @param channel_list - LogWriter的通道列表
         */
        bool initializeLogFiles(const std::vector<int>& channel_list);

        /// 获得 RedoLog 区需要的空间大小
        off_t getRedoLogAreaSize();

        /// 初始化 UNDO 表空间中的 RedoLog 区，在创建的时候调用
        bool initializeRedoLogArea();

        /** 提交日志.
         * @param log 日志内容
         * @param type 日志类型
         * @param scn  事务commit时的SCN
         */
        bool add(const char* log, LogType type, T_SCN scn = 0);

        /// 提交的日志是否已经写入磁盘
        bool hasFinished();

    private:
        const RedoConfig*           m_config;

        RedoLogBuffer*              m_redoLogBuffer;
        RedoLogBuffer::Header       m_bufferHeader;

        LGWRRunStatus*              m_lgwrRunStatus;
        LGWRRunStatus::RunStatus    m_lgwrStatus;

        LocalRedoBuffer*            m_localRedoBuffer;

        int64_t                     m_scn;

        std::vector<Lock*>          m_locksNeedRelease; // 需要释放的锁对象

    private:
        // 提交本地缓冲区的日志
        bool submit();
};

#endif
