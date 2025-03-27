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
        //��ʼ��:��ռ��С���ļ�����С r_sfileNameΪ���ļ���
        bool initialize(const char* r_sfileName, const size_t& r_spaceSize);
        //�����ݴ��ļ��е��뵽�����ڴ�
        bool filetoshm(char* r_shmAddr, const char* r_path);
        //�����ݴӹ����ڴ浼�����ļ���
        bool shmtofile(char* r_shmAddr, const char* r_path);
        static size_t getFileHdLen()
        {
            return FILEHEADER_LEN;
        }
    protected:
        enum ConstValue
        {
            FILEHEADER_LEN = 1024 * 4, ///< �ļ�ͷ���� by chenm �ĳ�4k��С ����ߴ���д��Ч��
            INIT_FILE   = 0  ,  ///< �´����ļ�
            NORMAL_FILE = 1  ,  ///< �ļ�״̬������״̬
            BAD_FILE    = 2  ,  ///< �ļ�״̬���쳣���ļ����ܱ��ƻ���
            /*
            #ifdef IBMAIX_OS_CONFIG
            MAXTHREADNUM= 2  ,  ///< ����߳���2
            #else
            MAXTHREADNUM= 5  ,  ///< ����߳���5
            #endif
            */
            MAXTHREADNUM = 10 ,
            MINFILESIZE = 1024 * 1024 * 512 ///< ���ö��̵߳���С�ļ���С. 512M
        };
        class SpaceFileHeader
        {
            public:
                char    m_testInfo[10];
                int     m_fileStatus; ///<�ļ�״̬
                size_t  m_fileSize;   ///<�ļ���С
                size_t  m_spaceSize;  ///<��ռ��С
        };
        /// ÿ���߳�ʹ�õĲ�������.
        typedef struct
        {
            pthread_t  m_threadId;	///< �߳�ID.
            bool       m_bRet;		  ///< ����ֵ.
            char*      m_shmAddr;   ///< д�ڴ��ַ.
            off_t      m_startOff;	///< ��ʼƫ����(�����ڴ�).
            off_t      m_endOff;		///< ����ƫ����(�����ڴ�).
            SpaceFile* m_pSelf;	    ///< ָ������ָ��.
            int        m_inoutFlag; ///< 0:fileToshm,>0:shmToFile
        } ThreadParam;

    protected:
        //���ɹ̶���С���ļ�(�ն��ļ�),������ռ�ʱ����
        bool createFile(const char* r_path);
        //ɾ���ļ�
        bool deleteFile(const char* r_path);
        ///д�ļ�ͷ
        bool writefilehead(const SpaceFileHeader& r_fileHeader);
        ///���ļ�ͷ
        bool readFilehead(SpaceFileHeader& r_fileHeader);
        ///�̶߳�ȡ�ļ����ݵ������ڴ���
        static void* threadRw_r(ThreadParam& r_param);
    private:
        T_FILENAMEDEF   m_shortname;///<���ļ���
        T_FILENAMEDEF   m_fileName; ///<�ļ���
        size_t          m_spaceSize;///<��ռ��С
        size_t          m_fileSize; ///<�ļ���С
        int             m_threadNum;///<�߳���
};
#endif //_SPACEFILE_H_INCLUDE_20080415_

