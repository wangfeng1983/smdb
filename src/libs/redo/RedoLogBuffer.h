#ifndef _LINKAGE_MDB_REDO_LOGBUFFER_H_
#define _LINKAGE_MDB_REDO_LOGBUFFER_H_

#include <ctime>
#include <unistd.h>
#include <limits.h>
#include <inttypes.h>

class Lock;
class LoopArray;

/** REDO日志缓存
 * 缓存包括头部和数据区。
 * 头部包含了缓冲区的一些相关信息。
 * 数据区用来保存redo log，用循环队列实现，用头部的位置head和数据大小size来指示数据的位置。
 */
class RedoLogBuffer
{
    public:
        /** REDO LOG 缓存的头部信息
         * 其中的 head 和 rear 表示的偏移量从真正的数据开始，不包括头部
         */
        typedef struct _REDOLOGBUFFER_HEADER
        {
            pid_t       lgwr_pid;           // LGWR 的进程号
            int         version;            // 变更版本号
            int64_t     scn_min;            // 最小 commit SCN
            int64_t     scn_max;            // 最大 commit SCN
            double      insert_time;        // 最后一次插入的时间
            double      write_time;         // 最后一次写入磁盘的时间
            off_t       capacity;           // 缓冲区容量，不包括头部
            off_t       head;               // 头指针，指向日志数据的第一个字节
            off_t       rear;               // 尾指针，指向下一个插入的位置
            off_t       size;               // 数据大小
        } Header;

    public:
        /**
         * @param addr   开始地址
         * @param length 长度(包括头部)
         */
        RedoLogBuffer(void* addr, Lock* lock);
        ~RedoLogBuffer(void);

        bool initialize();

        /// 设置头部的 LogWriter pid, 由 LGWR 在启动时设置
        bool setLGWRPID(pid_t pid);

    public:
        /** 将数据插入缓存
         * @param data      需要插入的数据
         * @param dataSize  数据大小
         * @param metainfo  元数据信息
         * @param header    需要接收的缓冲区头部指针，如果不需要设置为NULL
         * @return          true 表示成功； false 表示失败
         */
        bool insert(const void* data,
                    off_t data_size,
                    int64_t scn_min,
                    int64_t scn_max,
                    Header* header = 0);

        /** 获取缓存中的日志数据，必须一次取完, 如果 bufferSize 太小会失败
         * @param buffer        接收数据的缓冲区
         * @param bufferSize    缓冲区大小
         * @param header        需要接收的缓冲区头部指针，如果不需要设置为NULL
         * @return              获得的数据大小，=0表示没有数据, <0表示出错
         */
        int64_t get(void* buffer, off_t buffer_size, Header* header = 0);

        /** 删除缓存中的日志数据, 从头部开始删除 length 个字节
         * @param length        需要删除的数据量
         * @param header        需要接收的缓冲区头部指针，如果不需要设置为NULL
         * @return              true 表示成功； false 表示失败
         */
        bool erase(off_t length, Header* header = 0);

        // 通知 LogWriter 写日志
        void notifyWriter();

        /** 比较版本号
         * @return 1表示v1>v2(也考虑到版本号归零的情况), 0表示v1=v2, -1表示v1<v2
         */
        static int compareVersion(int v1, int v2);

    private:
        void getHeader(Header* header);

    private:
        void*       m_addr;         // 缓冲区的开始地址
        Lock*       m_lock;         // 锁
        Header*     m_header;       // 缓冲区头部指针, 等于m_addr
        int         m_header_size;  // 缓冲区头部结构的长度
        LoopArray*  m_loopArray;    // 循环队列
};

#endif
