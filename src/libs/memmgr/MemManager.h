/**
*   MDB�ڴ������.
*   �ṩ�ڴ�����Ľӿ�
*   @author �߽��� <gaojf@lianchuang.com>
*   @version 1.0
*   @date 2008-04-12
*/
#ifndef _MEMMANAGER_H_INCLUDE_200080411_
#define _MEMMANAGER_H_INCLUDE_200080411_

#include "MdbConstDef.h"
#include "Mdb_Config.h"
#include "ControlFile.h"
#include "SpaceInfo.h"
#include "ControlMemMgr.h"
#include "MdbAddress.h"
#include "TableMemMgr.h"
#include "IndexMemMgr.h"
#include "TbMemInfo.h"
#include "Mdb_Exception.h"
#include "UndoMemMgr.h"
#include "MdbStatusMgr.h"

class ThreadParam
{
    public:
        bool           m_result;
        SpaceMgrBase*  m_pSpaceMgr;
        MdbStatus*     m_pstatus;
        pthread_t      m_threadId;
};

class MemManager
{
    protected:
        class InnerPageInfo
        {
            public:
                PageInfo*    m_page;
                ShmPosition  m_pos;
        };
    public:
        MemManager(const char* r_dbName);
        virtual ~MemManager();
    public: //�ṩ��Db�ӿ�:���ݿ⼶���������ʽ��̣�
        /**
         *openMdb �����ݿ�����.
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool openMdb()  ;
        /**
         *openMdb �Ͽ����ݿ�����.
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool closeMdb()  ;
    public://�ṩ��Db�ӿ�:���ݿ⼶����(�������)
        /**
         *creatMdb ���ݿⴴ��.
         *         ֻ�����������Ĵ���(�����ݿ�)
         *         ��������Ϣ�ݲ��������ļ���ʽ�ṩ
         *         �������ݾ�δ�����������ļ���ռ�
         *@return  true �����ɹ�,ʧ�� �׳��쳣
         */
        bool creatMdb() ;

        /**��ʱ��ʵ��
         *dropMdb ���ݿ�ɾ��.
         *         ��������ڴ����ݿ���Ϣ
         *@return  true ɾ���ɹ�,ʧ�� �׳��쳣
         */
        bool dropMdb()  ;

        /**
         *startMdb ���ݿ�����.
         *@return  true �����ɹ�,ʧ�� �׳��쳣
         */
        bool startMdb()  ;
        /**
         *afterStartMdb ���ݿ�������,����scn�Ͳ�λ��Ϣ��.
         * ʧ�� �׳��쳣
         */
        void afterStartMdb();
        /**
         *stopMdb ���ݿ�ֹͣ.
         *@return  true ֹͣ�ɹ�,ʧ�� �׳��쳣
         */
        bool stopMdb()  ;
        ///////////////////////////////////////////
        bool backupMdb()  ;
        void lockddl();
        bool tryLockDDL(); // add by chenm 2010-09-09
        void unlockddl();
        MdbStatus* getMdbStatus();
        bool writeStatus();
        static void* writeOneTBSpace(ThreadParam* arg);//add by chenm 2010-09-10
    public:
        //��ַת���ӿ�
        /**
         *getPhAddr �����ڲ�λ����Ϣ��ȡ�����ַ��Ϣ.
         *@param   r_mdbAddr: ����Ϊ�ڲ�λ����Ϣ�����Ϊ�����ַ��Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getPhAddr(MdbAddress& r_mdbAddr)  ;
        /**
         *getPhAddr �����ڲ�λ����Ϣ��ȡ�����ַ��Ϣ.
         *@param   r_shmPos: ����Ϊ�ڲ�λ����Ϣ
         *@param   r_phAddr: ���Ϊ�����ַ��Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getPhAddr(const ShmPosition& r_shmPos, char * &r_phAddr)  ;
        /**
         *getShmPos ���������ַ��Ϣ��ȡ�ڲ�λ����Ϣ.
         *@param   r_mdbAddr: ����Ϊ�����ַ��Ϣ�����Ϊ�ڲ�λ����Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getShmPos(MdbAddress& r_mdbAddr)  ;
        /**
         *memcopy �ڴ�Copy.
         *@param   r_desAddr: Դ�����ַ
         *@param   r_srcAddr: Ŀ�������ַ
         *@param   r_size   : ���Ƶ��ֽ���
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool memcopy(void* r_desAddr, const void* r_srcAddr, const size_t& r_size)  ;
        /**
         *memcopy �ڴ�Copy.
         *@param   r_desAddr: Դ�����ַ
         *@param   r_shmPos : Ŀ���ڲ���ַ
         *@param   r_size   : ���Ƶ��ֽ���
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool memcopy(void* r_desAddr, const ShmPosition& r_shmPos, const size_t& r_size)  ;
    public:
        void getlastckpscn(T_SCN& r_scn);
        void setlastckpscn(const T_SCN& r_scn);

        MDbGlobalInfo* getMdbGInfo() {
            return m_ctlMemMgr.getMdbGInfo();
        };

        /**
         *addGlobalParam ����ȫ�ֲ�����Ϣ.
         *@param   r_gParam: ȫ�ֲ�����Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool addGlobalParam(const GlobalParam& r_gParam)  ;
        /**
         *getGlobalParam ��ȡȫ�ֲ�����Ϣ.
         *@param   r_paramname: ��������
         *@param   r_gParam   : ��Ż�ȡ�Ĳ�����Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getGlobalParam(const char* r_paramname, GlobalParam& r_gParam)  ;
        /**
         *getGlobalParams ��ȡ����ȫ�ֲ�����Ϣ.
         *@param   r_gparamList   : ��Ż�ȡ�Ĳ�����Ϣ
         *@return  true �ɹ�,false ʧ��
         */
        bool getGlobalParams(vector<GlobalParam>& r_gparamList)  ;
        /**
         *updateGlobalParam ����ȫ�ֲ�����Ϣ.
         *@param   r_paramname   : ��������
         *@param   r_value       : ֵ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool updateGlobalParam(const char* r_paramname, const char* r_value)  ;
        /**
         *deleteGlobalParam ɾ��ȫ�ֲ�����Ϣ.
         *@param   r_paramname   : ��������
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool deleteGlobalParam(const char* r_paramname)  ;
        /**
         *registerSession �Ǽ�Session��Ϣ.
         *@param   r_sessionInfo   : SESSION��Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool registerSession(SessionInfo& r_sessionInfo)  ;
        /**
         *unRegisterSession ע��Session��Ϣ.
         *@param   r_sessionInfo   : SESSION��Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool unRegisterSession(const SessionInfo& r_sessionInfo)  ;
        /**
         *getSessionInfos ��ȡ����Session��Ϣ.
         *@param   r_sessionInfoList   : ��Ż�ȡ����Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getSessionInfos(vector<SessionInfo> &r_sessionInfoList)  ;
        bool clearSessionInfos(); //add by gaojf 2009-3-2 4:05

    public:
        /**
         *createTbSpace ���ݱ�ռ������Ϣ,�������ݱ�ռ�.
         *@param   r_spaceInfo   : ��ռ���Ϣ,��Ҫ���ñ�ռ�����
         *                          ��С�����͵���Ϣ
         *@param   t_flag        : 0 ��һ�δ���, ��0 �ǵ�һ�δ���
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool createTbSpace(SpaceInfo& r_spaceInfo, const int& r_flag = 0)  ;

        /**
         *getSpaceInfo ���ݱ�ռ�����ȡ��Ӧ�ı�ռ���Ϣ.
         *@param   r_spaceName   : ��ռ�����
         *@param   r_spaceInfo   : ��ռ���Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getSpaceInfo(const char* r_spaceName, SpaceInfo& r_spaceInfo)  ;

        /**
         *getSpaceInfoList ȡ��������������������ռ���Ϣ.
         *@param   r_spaceInfoList   : ���ȡ�õı�ռ���Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getSpaceInfoList(vector<SpaceInfo> &r_spaceInfoList)  ;

        /**
         *addTableSpace ����������󶨱�ռ�.
         *@param   r_spaceName   : ��ռ�����
         *@param   r_tableName   : ������������
         *@param   r_tableType   : ������
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool addTableSpace(const char* r_spaceName, const char* r_tableName, const T_TABLETYPE& r_tableType)  ;

        /**
         *getSpaceListByTable ���ݱ�ȡ��󶨵ı�ռ�����.
         *@param   r_tableName   : ����
         *@param   r_spaceList   : ��ű�����ı�ռ�����
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getSpaceListByTable(const char* r_tableName, vector<string> &r_spaceList)  ;
        /**
         *getSpaceListByIndex ��������ȡ��󶨵ı�ռ�����.
         *@param   r_indexName   : ������
         *@param   r_spaceList   : ��ű�����ı�ռ�����
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getSpaceListByIndex(const char* r_indexName, vector<string> &r_spaceList)  ;

        /**
         *updateMgrInfo ���±�ռ������(��ռ������������Ҫ����).
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool updateMgrInfo()  ; //�������ݱ�ռ������Ϣ
        /**
         *getPageInfo ����ҳ���ڲ�λ����Ϣȡ������ָ��.
         *@param r_pagePos:ҳ���ڲ�λ����Ϣ
         *@param r_pPage  :ҳ������ָ��
         *@return  true �ɹ���false ʧ��
         */
        bool getPageInfo(const ShmPosition& r_pagePos, PageInfo *&r_pPage);
        //����Slot��ַ��ȡ��Ӧ��ҳ����Ϣ��ҳ���ַ
        bool getPageBySlot(const ShmPosition& r_slot, ShmPosition& r_pagePos, PageInfo* &r_pPage);
    public:
        /**
         *createTable ���ݱ��崴����������(�����������������ڴ�����).
         *@param r_tableDef : ����
         *@param r_tableDesc: ��������ָ��
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool createTable(const TableDef& r_tableDef, TableDesc* &r_tableDesc)  ;
        /**
         *dropTable ɾ��������ɾ����������������ͷ��ڴ棩.
         *@param r_tableName : ������
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool dropTable(const char* r_tableName)  ;
        /**
         *truncateTable ��ձ�,�ͷ��ڴ棨������������գ�.
         *@param r_tableName : ������
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool truncateTable(const char* r_tableName)  ;
        /**
         *allocateTableMem �������ڴ�.
         *@param r_tableDesc : ��������ָ��(ָ�����ڴ�)
         *@param r_num       : �ڵ���
         *@param r_slotAddrList: ������뵽���ڴ��ַ��Ϣ(���������ַ��ƫ����)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool allocateTableMem(TableDesc* &r_tableDesc, const int& r_num,
                              vector<MdbAddress>& r_slotAddrList);


        /**
         *createIndex ����������������������(�������ݵĴ����������������й���).
         *@param r_idxDef : ��������(ָ�����ڴ�)
         *@param r_idxDesc: ����������(ָ�����ڴ�)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool createIndex(const IndexDef& r_idxDef, IndexDesc* &r_idxDesc)  ;
        /**
         *dropIndex ɾ������.
         *@param r_idxName : ��������
         *@param r_tableName: ������(��ȱʡ)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool dropIndex(const char* r_idxName, const char* r_tableName = 0)  ;
        /**
         *truncateIndex �������������������������.
         *@param r_idxName : ��������
         *@param r_tableName: ������(��ȱʡ)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool truncateIndex(const char* r_idxName, const char* r_tableName = 0)  ;
        /**
         *allocateIdxMem ���������ڴ�.
         *@param r_indexDesc : ����������
         *@param r_num       : �����ڵ����
         *@param r_addrList  : ������뵽���ڴ��ַ��Ϣ(���������ַ��ƫ����)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool allocateIdxMem(IndexDesc& r_indexDesc, const int& r_num, vector<MdbAddress> &r_addrList)  ;
        /**
         *allocateIdxMem ����һ�������ڵ��ڴ�.
         *@param r_indexDesc : ����������
         *@param r_addr      : ������뵽���ڴ��ַ��Ϣ(���������ַ��ƫ����)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool allocateIdxMem(IndexDesc* &r_pIndexDesc, MdbAddress& r_addr)  ;

        /**
         *getTableDescList ȡ���б���������Ϣ.
         *@param r_tableDescList : ��ű���������Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getTableDescList(vector<TableDesc>& r_tableDescList)  ;
        /**
         *getTableDefList ȡ���б�����Ϣ���������������壩.
         *@param r_tableDefList : ��ű���������Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getTableDefList(vector<TableDef>& r_tableDefList)  ;

        /**
         *getTableDescByName ���ݱ���ȡ����������Ϣ.
         *@param r_tableName : ����
         *@param r_pTableDesc: ��������ָ��(ָ�����ڴ�)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getTableDescByName(const char* r_tableName, TableDesc* &r_pTableDesc)  ;
        /**
         *getIndexListByTable ���ݱ�������ȡ������Ϣ.
         *@param r_tablDesc : ��������
         *@param r_pIndexList: ��Ÿñ��Ӧ����������������ָ���б�
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getIndexListByTable(const TableDesc& r_tablDesc, vector<IndexDesc*> &r_pIndexList)  ;
        /**
         *getIndexListByTable ���ݱ�������ȡ������Ϣ.
         *@param r_tablDesc : ��������
         *@param r_pIndexList: ��Ÿñ��Ӧ����������������Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getIndexListByTable(const char* r_tableName, vector<IndexDef> &r_pIndexList)  ;
        /**
         *getIndexDescByName ������������ȡ������Ϣ.
         *@param r_indexName : ��������
         *@param r_pIndexDesc: ������������ָ�����ڴ棩
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getIndexDescByName(const char* r_indexName, IndexDesc* &r_pIndexDesc)  ;
        /**
         *getTableMemInfo ���ݱ���/������ȡ��Ӧ���ڴ�ռ����Ϣ.
         *@param r_tableName : ����/������
         *@param r_tbMemInfo : �ڴ�ռ����Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getTableMemInfo(const char* r_tableName, TbMemInfo& r_tbMemInfo)  ;
        /**
         *getSlotByShmPos ����ƫ����ȡSlotָ��.
         *@param r_shmPos : �ڴ�ƫ����
         *@param r_pSlot  : Slotָ�루ָ�����ڴ棩
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool getSlotByShmPos(const ShmPosition& r_shmPos, UsedSlot* &r_pSlot)  ;
        UndoMemMgr* getUndoMemMgr()
        {
            return &m_undoMemMgr;
        }
        //r_flag Ϊ1 SCN �Զ�+1  0����
        void        getscn(T_SCN& r_scn, const int r_flag = 1)
        {
            m_ctlMemMgr.getscn(r_scn, r_flag);
        }
        void        gettid(long& transid)
        {
            m_ctlMemMgr.gettid(transid);
        }

        /**
         *freeIdxMem �ͷ������ڴ�.
         *@param r_indexDesc : ����������
         *@param r_addrList  : �����Ҫ�ͷŵ��ڴ��ַ��Ϣ(������ƫ����)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool freeIdxMem(IndexDesc& r_indexDesc, const vector<ShmPosition> &r_addrList, const T_SCN& r_scn)  ;
        /**
         *freeIdxMem �ͷ�һ�������ڵ��ڴ�.
         *@param r_indexDesc : ����������
         *@param r_addr  : �����Ҫ�ͷŵ��ڴ��ַ��Ϣ(������ƫ����)
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool freeIdxMem(IndexDesc& r_indexDesc, const ShmPosition& r_addr, const T_SCN& r_scn)  ;

        bool addRedoConfig(const char* key, const size_t& value);
        bool addRedoConfig(const char* key, const char* value);
    protected:
        DescPageInfos* getdescinfo(const char& r_type, const size_t& r_offset);
        size_t getdescOffset(const char& r_type, const DescPageInfos* r_desc);

        /**
         *freeTableMem ���ͷ��ڴ�.
         *@param r_tableDesc : ��������ָ��(ָ�����ڴ�)
         *@param r_slotAddrList: �����Ҫ�ͷŵ��ڴ��ַ��Ϣ
         *                     : r_slotAddrList������ShmPosition��Ϣ
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool freeTableMem(TableDesc* r_tableDesc, const vector<MdbAddress>& r_slotAddrList, const T_SCN& r_scn);
        bool freeTableMem(TableDesc* r_tableDesc, const MdbAddress& r_slotAddr, const T_SCN& r_scn);
        /**
         *getSpAddrList ȡ��ռ��ַ��Ϣ(��������ռ����).
         *r_refetch  0-�����»�ȡ��ַ��1-���»�ȡ��ַ //modified by gaojf 2012/5/10 9:13:11
         *@return  ��
         */
        void getSpAddrList(const int& r_refetch = 0);
        /**
         *createSpace ����һ����ռ�.
         *@param r_spaceInfo : ��ռ䶨����Ϣ
         *@param r_flag  : ��һ�δ�����ǣ�0��һ�Σ�1��
         *@return  true �ɹ�,false ʧ��
         */
        bool createSpace(SpaceInfo& r_spaceInfo, const int& r_flag);
        /**
         *createSpace ����������ռ�.
         *@param r_spaceInfo : ��ռ䶨����Ϣ�б�
         *@param r_flag  : ��һ�δ�����ǣ�0��һ�Σ�1��
         *@return  true �ɹ�,false ʧ��
         */
        bool createSpace(const int& r_flag, vector<SpaceInfo> &r_spaceInfo);
        /**
         *attachSpace attach������ռ�(����������).
         *@return  true �ɹ�,false ʧ��
         */
        bool attachSpace();
        /**
         *detachSpace detach������ռ�(����������)
         *@return  true �ɹ�,false ʧ��
         */
        bool detachSpace();
        /**
         *initSpace ��ʼ��������ռ�0��һ�δ�����1�ǵ�һ��
         *@param r_flag  : 0��һ�Σ�1��
         *@return  true �ɹ�,false ʧ��
         */
        bool initSpace(const int r_flag);

        /**
         *allocateMem �����ڴ棨r_num���ڵ㣩
         *@param r_spaceNum: ��ռ����
         *@param r_spaceList:��ռ��б�
         *@param r_descPageInfo:��������Ӧ���ڴ�ҳ����Ϣ
         *@param r_num   : ����Ľڵ���
         *@param r_slotAddrList  : ������뵽�Ľڵ��ַ(�����ڲ���ַ��Ϣ�������ַ��Ϣ)
         *@param r_flag          : 0 ��,��Ҫ��Slot������, 1���������贮
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool allocateMem(const int& r_spaceNum, const T_NAMEDEF r_spaceList[MAX_SPACE_NUM],
                         DescPageInfos* r_descpageinfogs, const int& r_num,
                         vector<MdbAddress>& r_slotAddrList,
                         const int& r_flag);
        /**
         *freeMem �ͷ�һ���ڵ���ڴ�
         *@param r_descPageInfo:��������Ӧ���ڴ�ҳ����Ϣ
         *@param r_addr  : �ڵ���ڲ���ַ��Ϣ
         *@param r_falg          : 0 ��,��Ҫ��Slot������, 1���������贮
         *@return  true �ɹ�,ʧ�� �׳��쳣
         */
        bool freeMem(DescPageInfos* r_descpageinfogs, const ShmPosition& r_addr,
                     const int& r_flag, const T_SCN& r_scn)  ;
        /**
         *getSessionId ��ȡSESSIONID.
         *@return  ��ȡ����Id,-1��ʾʧ��
         */
        void  getSessionId(int& sessionid, short int& semmark); //��ȡSessionId
        /**
         *realseSid �ͷ�SESSIONID.
         *@param   r_sid: ��Ҫ�ͷŵ�SESSIONID
         *@return  true �ɹ�,false ʧ��
         */
        bool realseSid(const int& r_sid);

    protected:
        T_NAMEDEF         m_mdbName;      ///<���ݿ�����
        MdbCtlInfo*       m_pMdbCtlInfo;  ///<������Ϣ
        ControlFile       m_ctlFile;      ///<���ݿ�����ļ�
        vector<SpaceInfo> m_spaceInfoList;///<��ռ���Ϣ
        /////////////////////////////////////////////////////
        ControlMemMgr     m_ctlMemMgr;    ///<�������ڴ����
        TableMemMgr       m_tableMemMgr;  ///<�������ڴ����
        IndexMemMgr       m_indexMemMgr;  ///<�������ڴ����
        UndoMemMgr        m_undoMemMgr;   ///<Undo���ڴ����
        /////////////////////////////////////////////////////
        MdbAddressMgr     m_addressMgr;   ///<���ݿ��ַ������(ת��)
        MdbStatusMgr      m_statusMgr;    ///<MDB״̬������
        MdbStatus*        m_pstatus;      ///<״ָ̬��
        MDBLatchMgr*      m_pddlLock;     ///<DDLLOCK
        SrcLatchInfo*     m_ddlLatch;     ///<DDLLOCKINFO
        friend class ChkptMgr;
    private:
        bool        m_openFlag;     ///<�Ƿ��Ѵ����ӱ��
        time_t      m_spUpdateTime; ///<��ռ�ˢ��ʱ��
    public:
        //���Խӿ�
        virtual bool dumpSpaceInfo(ostream& r_os);
        // �ڴ�ʹ���� add by chenm
        // Ĭ�Ϸ������б�ռ�ʹ����
        void getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName = NULL);
};
#endif //_MEMMANAGER_H_INCLUDE_200080411_

