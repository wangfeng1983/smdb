#ifndef _SPACEMGRBASE_MANAGER_H_20080415_
#define _SPACEMGRBASE_MANAGER_H_20080415_
#include <sys/shm.h>
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include "SpaceFile.h"
#include "SpaceInfo.h"
#include "MDBLatchMgr.h"
#include "Mdb_Exception.h"

class SpaceMgrBase
{
    protected:
        //������Ϣ�ڱ�ռ䴴��֮ǰ����
        SpaceInfo    m_spaceHeader;
        SpaceFile    m_file;

        //��ռ䴴��֮����Ϣ
        SpaceInfo*    m_pSpHeader;  ///<��ռ�ͷ
    public:
        SpaceMgrBase()
        {
            m_pdbLock = NULL;
        }
        virtual ~SpaceMgrBase()
        {
            if (m_spaceHeader.m_shmAddr != NULL)
            {
                shmdt(m_spaceHeader.m_shmAddr);
                m_spaceHeader.m_shmAddr = NULL;
            }
        }
    public:
        bool dumpDataIntoFile(const char* r_path);
        bool loadDataFromFile(const char* r_path);
    public:
        //���Խӿ�
        virtual bool dumpSpaceInfo(ostream& r_os);
        // �ڴ�ʹ���� add by chenm
        // Ĭ�Ϸ������б�ռ�ʹ����
        virtual float getTableSpaceUsedPercent()
        {
            return 0;
        }
    public:
        /**
         *initialize ��ʼ��.
         *@param   r_SpaceInfo:  ��ռ䶨�����
         *@return  true �ɹ�,false ʧ��
         */
        bool  initialize(const SpaceInfo&  r_SpaceInfo);
        bool  attach();
        bool  detach();
        SpaceInfo* getSpaceInfo();
        unsigned int getSpaceCode()
        {
            return m_spaceHeader.m_spaceCode;
        }
        char* getSpaceAddr()
        {
            return m_spaceHeader.m_shmAddr;
        }
        char* getSpaceName()
        {
            return m_spaceHeader.m_spaceName;
        }
    public:
        /**
         *createSpace ������ռ�.
         *@param   r_SpaceInfo:  ��ռ䶨�����
         *@param   r_shmKey    :  ��ռ��ShmKey
         *@param   r_shmId     :  ��ռ�ShmId
         *@return  true �����ɹ�,false ʧ��
         */
        bool createSpace(SpaceInfo&  r_SpaceInfo);
        static bool deleteSpace(SpaceInfo&  r_SpaceInfo);
        /**
         *deleteSpace ɾ����ռ�.
         *@param   r_shmId :  ��ռ�ShmId
         *@return  true �ɹ�,false ʧ��
         */
        static bool deleteSpace(const int& r_shmId);

        /**
         *attachSpace ���ӱ�ռ�
         *@param   r_shmKey:  ��ռ�ShmKey
         *@param   r_size  :  ��ռ��С
         *@param   r_shmId :  ��ռ�ShmId
         *@param   r_pShm  :  ���Ӻ�ĵ�ַ
         *@return  true �ɹ�,false ʧ��
         */
        static bool attachSpace(const key_t& r_shmKey, const size_t& r_size,
                                int& r_shmId, char* &r_pShm);

        /**
         *attachSpace ���ӱ�ռ�
         *@param   r_shmId :  ��ռ�ShmId
         *@param   r_pShm  :  ���Ӻ�ĵ�ַ
         *@return  true �ɹ�,false ʧ��
         */
        static bool attachSpace(const int& r_shmId, char * &r_pShm);

        /**
         *detachSpace ���ӱ�ռ�
         *@param   r_pShm  :  ��ַ
         *@return  true �ɹ�,false ʧ��
         */
        static bool detachSpace(char * &r_pShm);
    public:
        void lock(const int r_pos = 0) throw(Mdb_Exception);
        bool trylock(const int r_pos = 0) throw(Mdb_Exception);
        void unlock(const int r_pos = 0);
    public:
        MDBLatchMgr*  m_pdbLock;
        SrcLatchInfo* m_srclatchinfo[MDB_PARL_NUM];
};


#endif //_SPACEMGRBASE_MANAGER_H_20080415_
