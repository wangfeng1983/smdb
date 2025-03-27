#ifndef _SPACEFILE_H_INCLUDE_20080415_
#define _SPACEFILE_H_INCLUDE_20080415_

#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#include "MdbConstDef.h"

class SpaceFile
{
    public:
        SpaceFile();
        virtual ~SpaceFile();
    public:
        //初始化:表空间大小，文件名大小 r_sfileName为短文件名
        bool initialize(const char* r_sfileName, const size_t& r_spaceSize);
        //将数据从文件中导入到共享内存
        bool filetoshm(char* r_shmAddr, const char* r_path);
        //将数据从共享内存导出到文件中
        bool shmtofile(char* r_shmAddr, const char* r_path);
        static size_t getFileHdLen()
        {
            return FILEHEADER_LEN;
        }
    protected:
        enum ConstValue
        {
            FILEHEADER_LEN = 1024 * 4, ///< 文件头长度 by chenm 改成4k大小 可提高磁盘写入效率
            INIT_FILE   = 0  ,  ///< 新创建文件
            NORMAL_FILE = 1  ,  ///< 文件状态：正常状态
            BAD_FILE    = 2  ,  ///< 文件状态：异常（文件可能被破坏）
            /*
            #ifdef IBMAIX_OS_CONFIG
            MAXTHREADNUM= 2  ,  ///< 最大线程数2
            #else
            MAXTHREADNUM= 5  ,  ///< 最大线程数5
            #endif
            */
            MAXTHREADNUM = 10 ,
            MINFILESIZE = 1024 * 1024 * 512 ///< 采用多线程的最小文件大小. 512M
        };
        class SpaceFileHeader
        {
            public:
                char    m_testInfo[10];
                int     m_fileStatus; ///<文件状态
                size_t  m_fileSize;   ///<文件大小
                size_t  m_spaceSize;  ///<表空间大小
        };
        /// 每个线程使用的参数参数.
        typedef struct
        {
            pthread_t  m_threadId;	///< 线程ID.
            bool       m_bRet;		  ///< 返回值.
            char*      m_shmAddr;   ///< 写内存地址.
            off_t      m_startOff;	///< 起始偏移量(共享内存).
            off_t      m_endOff;		///< 结束偏移量(共享内存).
            SpaceFile* m_pSelf;	    ///< 指向自身指针.
            int        m_inoutFlag; ///< 0:fileToshm,>0:shmToFile
        } ThreadParam;

    protected:
        //生成固定大小的文件(空洞文件),创建表空间时调用
        bool createFile(const char* r_path);
        //删除文件
        bool deleteFile(const char* r_path);
        ///写文件头
        bool writefilehead(const SpaceFileHeader& r_fileHeader);
        ///读文件头
        bool readFilehead(SpaceFileHeader& r_fileHeader);
        ///线程读取文件内容到共享内存中
        static void* threadRw_r(ThreadParam& r_param);
    private:
        T_FILENAMEDEF   m_shortname;///<短文件名
        T_FILENAMEDEF   m_fileName; ///<文件名
        size_t          m_spaceSize;///<表空间大小
        size_t          m_fileSize; ///<文件大小
        int             m_threadNum;///<线程数
};
#endif //_SPACEFILE_H_INCLUDE_20080415_

