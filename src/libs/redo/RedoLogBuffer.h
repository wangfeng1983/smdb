#ifndef _LINKAGE_MDB_REDO_LOGBUFFER_H_
#define _LINKAGE_MDB_REDO_LOGBUFFER_H_

#include <ctime>
#include <unistd.h>
#include <limits.h>
#include <inttypes.h>

class Lock;
class LoopArray;

/** REDO��־����
 * �������ͷ������������
 * ͷ�������˻�������һЩ�����Ϣ��
 * ��������������redo log����ѭ������ʵ�֣���ͷ����λ��head�����ݴ�Сsize��ָʾ���ݵ�λ�á�
 */
class RedoLogBuffer
{
    public:
        /** REDO LOG �����ͷ����Ϣ
         * ���е� head �� rear ��ʾ��ƫ���������������ݿ�ʼ��������ͷ��
         */
        typedef struct _REDOLOGBUFFER_HEADER
        {
            pid_t       lgwr_pid;           // LGWR �Ľ��̺�
            int         version;            // ����汾��
            int64_t     scn_min;            // ��С commit SCN
            int64_t     scn_max;            // ��� commit SCN
            double      insert_time;        // ���һ�β����ʱ��
            double      write_time;         // ���һ��д����̵�ʱ��
            off_t       capacity;           // ������������������ͷ��
            off_t       head;               // ͷָ�룬ָ����־���ݵĵ�һ���ֽ�
            off_t       rear;               // βָ�룬ָ����һ�������λ��
            off_t       size;               // ���ݴ�С
        } Header;

    public:
        /**
         * @param addr   ��ʼ��ַ
         * @param length ����(����ͷ��)
         */
        RedoLogBuffer(void* addr, Lock* lock);
        ~RedoLogBuffer(void);

        bool initialize();

        /// ����ͷ���� LogWriter pid, �� LGWR ������ʱ����
        bool setLGWRPID(pid_t pid);

    public:
        /** �����ݲ��뻺��
         * @param data      ��Ҫ���������
         * @param dataSize  ���ݴ�С
         * @param metainfo  Ԫ������Ϣ
         * @param header    ��Ҫ���յĻ�����ͷ��ָ�룬�������Ҫ����ΪNULL
         * @return          true ��ʾ�ɹ��� false ��ʾʧ��
         */
        bool insert(const void* data,
                    off_t data_size,
                    int64_t scn_min,
                    int64_t scn_max,
                    Header* header = 0);

        /** ��ȡ�����е���־���ݣ�����һ��ȡ��, ��� bufferSize ̫С��ʧ��
         * @param buffer        �������ݵĻ�����
         * @param bufferSize    ��������С
         * @param header        ��Ҫ���յĻ�����ͷ��ָ�룬�������Ҫ����ΪNULL
         * @return              ��õ����ݴ�С��=0��ʾû������, <0��ʾ����
         */
        int64_t get(void* buffer, off_t buffer_size, Header* header = 0);

        /** ɾ�������е���־����, ��ͷ����ʼɾ�� length ���ֽ�
         * @param length        ��Ҫɾ����������
         * @param header        ��Ҫ���յĻ�����ͷ��ָ�룬�������Ҫ����ΪNULL
         * @return              true ��ʾ�ɹ��� false ��ʾʧ��
         */
        bool erase(off_t length, Header* header = 0);

        // ֪ͨ LogWriter д��־
        void notifyWriter();

        /** �Ƚϰ汾��
         * @return 1��ʾv1>v2(Ҳ���ǵ��汾�Ź�������), 0��ʾv1=v2, -1��ʾv1<v2
         */
        static int compareVersion(int v1, int v2);

    private:
        void getHeader(Header* header);

    private:
        void*       m_addr;         // �������Ŀ�ʼ��ַ
        Lock*       m_lock;         // ��
        Header*     m_header;       // ������ͷ��ָ��, ����m_addr
        int         m_header_size;  // ������ͷ���ṹ�ĳ���
        LoopArray*  m_loopArray;    // ѭ������
};

#endif
