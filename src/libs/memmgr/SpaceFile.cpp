#include <iostream>
#include <fstream>
#include <fcntl.h>
#include "debuglog.h"

#include "SpaceFile.h"

SpaceFile::SpaceFile()
{
}

SpaceFile::~SpaceFile()
{
}

//��ʼ��:��ռ��С���ļ�����С
bool SpaceFile::initialize(const char* r_sfileName, const size_t& r_spaceSize)
{
    if (strlen(r_sfileName) >= sizeof(T_FILENAMEDEF)) 
    {
        log_error("r_sfileName=" << r_sfileName << ", ���� >= " << sizeof(T_FILENAMEDEF));
        return false;
    }

    if (r_spaceSize <= 0)
    {
        log_error("�Ƿ����� r_spaceSize = " << r_spaceSize);
        return false;
    }
    strcpy(m_shortname, r_sfileName);
    m_spaceSize = r_spaceSize;
    m_fileSize = FILEHEADER_LEN + r_spaceSize;
    return true;
}
//���ɹ̶���С���ļ�(�ն��ļ�),������ռ�ʱ����
bool SpaceFile::createFile(const char* r_path)
{
    sprintf(m_fileName, "%s/%s", r_path, m_shortname);
    //1.����ļ��Ƿ���ڣ������򱨴�
    if (access(m_fileName, F_OK) == 0)
    {
#ifdef _DEBUG_
        cout << m_fileName << "�����ļ��Ѿ�����!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //2.�����ļ�����СΪ:m_fileSize
    int t_fd;
    if ((t_fd = creat(m_fileName, S_IRUSR | S_IWUSR)) < 0)
    {
#ifdef _DEBUG_
        cout << "creat file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (lseek(t_fd, m_fileSize - 1, SEEK_SET) < 0)
    {
#ifdef _DEBUG_
        cout << "lseek file {" << m_fileName << "," << m_fileSize - 1 << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    char t_buf[2] = "0";
    if (write(t_fd, t_buf, 1) != 1)
    {
#ifdef _DEBUG_
        cout << "write file {" << m_fileName << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    close(t_fd);
    SpaceFileHeader t_fileHeader;
    t_fileHeader.m_fileStatus = INIT_FILE;
    t_fileHeader.m_fileSize   = m_fileSize;
    t_fileHeader.m_spaceSize  = m_spaceSize;
    if (writefilehead(t_fileHeader) == false)
    {
#ifdef _DEBUG_
        cout << "Writefilehead false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
//ɾ���ļ�
bool SpaceFile::deleteFile(const char* r_path)
{
    sprintf(m_fileName, "%s/%s", r_path, m_shortname);
    if (unlink(m_fileName) != 0)
    {
#ifdef _DEBUG_
        cout << "unlink(" << m_fileName << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}

bool SpaceFile::filetoshm(char* r_shmAddr, const char* r_path)
{
    sprintf(m_fileName, "%s/%s", r_path, m_shortname);
    struct stat t_fileAttr;       //ȡ�ļ�����
    SpaceFileHeader t_fileHeader; //�ļ�ͷ��Ϣ
    //1.�޶�Ӧ�ļ������ش���
    if (access(m_fileName, F_OK) != 0 ||
            stat(m_fileName, &t_fileAttr) != 0)
    {
#ifdef _DEBUG_
        cout << m_fileName << "�ļ������ڻ��޷�ȡ����,����ʧ��!" << __FILE__ << __LINE__ << endl;
        return false;
#endif
    }
    else if (t_fileAttr.st_size != m_fileSize)
    {
#ifdef _DEBUG_
        cout << "t_fileAttr.st_size:" << t_fileAttr.st_size << endl;
        cout << "m_fileSize:" << m_fileSize << endl;
        cout << m_fileName << "�ļ���Ч���ݿռ��С���ڴ�ռ��С��һ�£�����ʧ��!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //2.�ж�Ӧ�ļ�,�жϱ�ʶλ
    if (readFilehead(t_fileHeader) == false)
    {
#ifdef _DEBUG_
        cout << "��ȡ�ļ�ͷ��Ϣʧ��! " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    else
    {
        if (NORMAL_FILE == t_fileHeader.m_fileStatus)
        {
            //����
        }
        else if (INIT_FILE == t_fileHeader.m_fileStatus)
        {
            //�ļ�δ��ʼ��
#ifdef _DEBUG_
            cout << "�ļ�δ��ʼ��!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
        else  //if(BAD_FILE==t_fileHeader.m_fileStatus)
        {
#ifdef _DEBUG_
            cout << m_fileName << "�ļ������ѱ��ƻ���" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
    }
#ifdef _DEBUG_
    time_t t_startTime, t_endTime;
    time(&t_startTime);
#endif
    int t_threadNum = (int)(1 + m_spaceSize / MINFILESIZE);
    int t_i, t_j;
    ThreadParam t_threadParam[MAXTHREADNUM];
    pthread_t   t_threadId[MAXTHREADNUM]; //���߳�ID����
    bool        t_bRet = true;
    m_threadNum = (t_threadNum > MAXTHREADNUM) ? MAXTHREADNUM : t_threadNum;
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setstacksize(&t_attr, 20 * 1024 * 1024);
    for (t_i = 0; t_i < m_threadNum; t_i++)
    {
        t_threadParam[t_i].m_shmAddr = r_shmAddr;
        t_threadParam[t_i].m_startOff = (m_spaceSize / m_threadNum) * t_i;
        t_threadParam[t_i].m_endOff   = (m_spaceSize / m_threadNum) * (t_i + 1) - 1;
        t_threadParam[t_i].m_pSelf = this;
        t_threadParam[t_i].m_inoutFlag = 0;//filetoshm
        if (m_threadNum - 1 == t_i)
        {
            t_threadParam[t_i].m_endOff = m_spaceSize - 1;
        }
        if (pthread_create(&(t_threadParam[t_i].m_threadId), &t_attr,
                           (void * (*)(void*))threadRw_r,
                           &(t_threadParam[t_i])) != 0)
        {
#ifdef _DEBUG_
            cout << "Create Thread Error" << endl;
#endif
            for (t_j = 0; t_j < t_i; t_j++)
                pthread_join(t_threadParam[t_j].m_threadId, NULL);
            pthread_attr_destroy(&t_attr);
            return false;
        }
    }
    pthread_attr_destroy(&t_attr);
    //�ȴ������߳̽���
    for (t_j = 0; t_j < m_threadNum; t_j++)
    {
        pthread_join(t_threadParam[t_j].m_threadId, NULL);
        if (t_threadParam[t_j].m_bRet == false)
        {
#ifdef _DEBUG_
            cout << "���̴߳������ id=" << t_j << " " << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
        }
    }
#ifdef _DEBUG_
    time(&t_endTime);
    cout << "���̴߳����ʱΪ:" << difftime(t_endTime, t_startTime) << "��" << endl;
#endif
    return t_bRet;
}


bool SpaceFile::readFilehead(SpaceFileHeader& r_fileHeader)
{
    FILE*   t_fp;
    if ((t_fp = fopen(m_fileName, "rb")) == NULL)
    {
#ifdef _DEBUG_
        cout << "open file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fseek(t_fp, 0, SEEK_SET) < 0)
    {
        fclose(t_fp);
#ifdef _DEBUG_
        cout << "readFilehead.fseek false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fread(&r_fileHeader, sizeof(SpaceFileHeader), 1, t_fp) != 1)
    {
        fclose(t_fp);
#ifdef _DEBUG_
        cout << "fread file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    fclose(t_fp);
    return true;
}

bool SpaceFile::writefilehead(const SpaceFileHeader& r_fileHeader)
{
    FILE* t_fp;
    if ((t_fp = fopen(m_fileName, "rb+")) == NULL)
    {
#ifdef _DEBUG_
        cout << "open file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fseek(t_fp, 0, SEEK_SET) < 0)
    {
#ifdef _DEBUG_
        cout << "fseek false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fwrite(&r_fileHeader, sizeof(SpaceFileHeader), 1, t_fp) != 1)
    {
        fclose(t_fp);
#ifdef _DEBUG_
        cout << "fwrite file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    fclose(t_fp);
    return true;
}

bool SpaceFile::shmtofile(char* r_shmAddr, const char* r_path)
{
    sprintf(m_fileName, "%s/%s", r_path, m_shortname);
    //const int   t_writeOneBit=100*1024*1024;
    const int   t_writeOneBit = 10 * 1024 * 1024;
    struct stat r_fileAttr;   //ȡ�ļ�����
    SpaceFileHeader t_fileHeader;
    //1.�޶�Ӧ�ļ�,ֱ�ӱ��ݣ�дͷ�ļ�
    if (access(m_fileName, F_OK) != 0)
    {
        if (createFile(r_path) == false)
        {
#ifdef _DEBUG_
            cout << m_fileName << "�ļ�������(�޷�����)���޷�ȡ����,ֱ�ӱ���!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
    }
    stat(m_fileName, &r_fileAttr);
    if (r_fileAttr.st_size != m_fileSize)
    {
#ifdef _DEBUG_
        cout << m_fileName << "�ļ���С����!" << __FILE__ << __LINE__ << endl;
        cout << "r_fileAttr.st_size=" << r_fileAttr.st_size << endl;
        cout << "m_fileSize=" << m_fileSize << endl;
        cout << "m_shortname=" << m_shortname << endl;
#endif
        return false;
    }
    //3.�ж�Ӧ�ļ�,�жϱ�ʶλ
    if (readFilehead(t_fileHeader) == false)
    {
#ifdef _DEBUG_
        cout << "readFilehead false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_fileHeader.m_fileStatus = BAD_FILE;
    //1.���ļ��óɲ�����״̬�������쳣ʱ����
    if (writefilehead(t_fileHeader) == false)
    {
#ifdef _DEBUG_
        cout << "readFilehead false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //2.�����ռ�����
    int t_fp;
#ifndef __sun
    t_fp = open(m_fileName, O_WRONLY | O_DIRECT);
#else // for Sun Solaris
    t_fp = open(m_fileName, O_WRONLY);
    if (t_fp >= 0)
    {
        if (directio(t_fp, DIRECTIO_ON) != 0)
        {
            log_error_call("directio");
            // continue anyway without direct I/O
        }
    }
#endif
    if (t_fp < 0)
    {
        log_error_callp("open", m_fileName);
        return false;
    }
    if (lseek(t_fp, FILEHEADER_LEN, SEEK_SET) < 0)
    {
        close(t_fp);
#ifdef _DEBUG_
        cout << "readFilehead.fseek false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    size_t t_remainSize = m_spaceSize;
    size_t t_writeSize = 0;
    while (t_remainSize > 0)
    {
        t_writeSize = (t_remainSize > t_writeOneBit) ? t_writeOneBit : t_remainSize;
        if (write(t_fp, r_shmAddr + m_spaceSize - t_remainSize, t_writeSize) != t_writeSize)
        {
            close(t_fp);
#ifdef _DEBUG_
            cout << "write file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
        t_remainSize -= t_writeSize;
    }
    close(t_fp);
    //3.дͷ�ļ���������ɱ�־
    t_fileHeader.m_fileStatus = NORMAL_FILE;
    if (writefilehead(t_fileHeader) == false)
    {
#ifdef _DEBUG_
        cout << "readFilehead false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}


///�߳̽������ڴ������ݵ������ļ���
void* SpaceFile::threadRw_r(ThreadParam& r_param)
{
    r_param.m_bRet = true;
    int     t_fp;
    mode_t  t_mode;
    ssize_t t_opSize = r_param.m_endOff - r_param.m_startOff + 1;
    //const  ssize_t s_oneReadSize = 100*1024*1024;
    const  ssize_t s_oneReadSize = 10 * 1024 * 1024;
    ssize_t t_oneReadSize = 0;
    ssize_t t_alReadSize = 0;
    if (r_param.m_inoutFlag == 0)
    {
        //��
        t_mode = O_RDONLY;
    }
    else
    {
        //д
        t_mode = O_WRONLY;
    }
    if ((t_fp = open(r_param.m_pSelf->m_fileName, t_mode)) < 0)
    {
#ifdef _DEBUG_
        cout << "open file {" << r_param.m_pSelf->m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        r_param.m_bRet = false;
        pthread_exit(NULL);
        return NULL;
    }
    if (lseek(t_fp, r_param.m_startOff + SpaceFile::FILEHEADER_LEN, SEEK_SET) < 0)
    {
        close(t_fp);
#ifdef _DEBUG_
        cout << "readFilehead.fseek false!" << __FILE__ << __LINE__ << endl;
#endif
        r_param.m_bRet = false;
        pthread_exit(NULL);
        return NULL;
    }
    if (r_param.m_inoutFlag == 0)
    {
        //if(read(t_fp,r_param.m_shmAddr+r_param.m_startOff,t_opSize)!=t_opSize)
        //��Ϊÿ������ȡ100M
        while (t_opSize > 0)
        {
            t_oneReadSize = (t_opSize > s_oneReadSize) ? s_oneReadSize : t_opSize;
            if (read(t_fp, r_param.m_shmAddr + r_param.m_startOff + t_alReadSize, t_oneReadSize) != t_oneReadSize)
            {
                close(t_fp);
#ifdef _DEBUG_
                cout << "read file {" << r_param.m_pSelf->m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
                r_param.m_bRet = false;
                pthread_exit(NULL);
                return NULL;
            }
            t_opSize -= t_oneReadSize;
            t_alReadSize += t_oneReadSize;
        }
    }
    else
    {
        if (write(t_fp, r_param.m_shmAddr + r_param.m_startOff, t_opSize) != t_opSize)
        {
            close(t_fp);
#ifdef _DEBUG_
            cout << "write file {" << r_param.m_pSelf->m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
            r_param.m_bRet = false;
            pthread_exit(NULL);
            return NULL;
        }
    }
    close(t_fp);
    pthread_exit(NULL);
    return NULL;
}

