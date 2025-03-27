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

/// ������־�ӿ�
class RedoLog
{
    public:
        /// REDO��־����
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

        /// REDO ��־��������Ϣ
        typedef struct _REDO_CONFIG
        {
            std::string         dbname;             // MDB name
            std::string         path;               // ���redo�ļ���·��

            int                 logFileSize;        // ���� log �ļ���С, ��λΪM
            int                 logGroupCount;      // log�ļ��������
            int                 logBufferSize;      // redo log buffer ��С, ��λΪK
            int                 logBufferCount;     // redo log buffer ����

            int                 writeMode;          // д����־�ķ�ʽ
            void*               redoLogAddress;     // UNDO ��ռ��� RedoLog ���Ŀ�ʼ��ַ
        } RedoConfig;

    public:
        RedoLog();
        ~RedoLog();

        /** ��ʼ��RedoLog����
         * @param config REDO log ��������Ϣ
         * @return ��ʼ���ɹ�����true; ���򷵻�false
         */
        bool initialize(const RedoConfig* config);

        /** ��ʼ�� log �ļ�.
         * ��� log �ļ������ڣ��򴴽����� log �ļ�
         * ��� log �ļ��Ѿ����ڣ���������� log �ļ�
         * @param channel_list - LogWriter��ͨ���б�
         */
        bool initializeLogFiles(const std::vector<int>& channel_list);

        /// ��� RedoLog ����Ҫ�Ŀռ��С
        off_t getRedoLogAreaSize();

        /// ��ʼ�� UNDO ��ռ��е� RedoLog �����ڴ�����ʱ�����
        bool initializeRedoLogArea();

        /** �ύ��־.
         * @param log ��־����
         * @param type ��־����
         * @param scn  ����commitʱ��SCN
         */
        bool add(const char* log, LogType type, T_SCN scn = 0);

        /// �ύ����־�Ƿ��Ѿ�д�����
        bool hasFinished();

    private:
        const RedoConfig*           m_config;

        RedoLogBuffer*              m_redoLogBuffer;
        RedoLogBuffer::Header       m_bufferHeader;

        LGWRRunStatus*              m_lgwrRunStatus;
        LGWRRunStatus::RunStatus    m_lgwrStatus;

        LocalRedoBuffer*            m_localRedoBuffer;

        int64_t                     m_scn;

        std::vector<Lock*>          m_locksNeedRelease; // ��Ҫ�ͷŵ�������

    private:
        // �ύ���ػ���������־
        bool submit();
};

#endif
