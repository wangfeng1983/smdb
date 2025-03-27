#ifndef _CONTROLMEMMGR_H_INCLUDE_20080422_
#define _CONTROLMEMMGR_H_INCLUDE_20080422_
#include "SpaceMgrBase.h"
#include "Mdb_Config.h"
#include "CtlElementTmpt.h"
#include "TableDescript.h"

class ControlMemMgr: public SpaceMgrBase
{
    public:
        ControlMemMgr();
        virtual ~ControlMemMgr();
    public:
        void initialize(MdbCtlInfo& r_ctlInfo);
        /**
         *createControlSpace ������������ռ�.
         *@param   r_flag : 0 ��һ�δ���,1 ���ļ�������Ϣ
         *@return  true �����ɹ�,false ʧ��
         */
        bool createControlSpace(const int& r_flag);

        //��ʼ����������Ϣ
        bool initCtlDataInfo(const int& r_flag, const char* r_path);
        bool deleteControlSpace();//ɾ����ռ�
        unsigned int getNSpaceCode();//��ȡ��һ����ռ����
        void getSessionId(int& sessionid, short int& semmark); //��ȡSessionId
        bool realseSid(const int& r_sid);//�ͷ�SessionId
        bool reInitSessionInfos(); //���³�ʼ��Session�б� add by gaojf 2009-3-2 3:54
        bool addSpaceInfo(const SpaceInfo& r_spaceInfo);
        bool getDSpaceInfo(const char* r_spaceName, SpaceInfo& r_spaceInfo);
        bool getSpaceInfos(vector<SpaceInfo> &r_spaceInfo);
        //add by gaojf 2009-12-10 13:07:14 ���±�ռ���ϢshmKey
        bool updateSpaceInfo(const SpaceInfo& r_spaceInfo);
        //r_flag Ϊ1 �Զ�+1  0����
        void getscn(T_SCN& r_scn, const int r_flag = 1); //add by gaojf MDB2.0
        void gettid(long& transid); //add by wangfp MDB2.0
        void getlastckpscn(T_SCN& r_scn);
        void setlastckpscn(const T_SCN& r_scn);

        DescPageInfos* getdescinfo(const char& r_type, const size_t& r_offset);
        size_t getdescOffset(const char& r_type, const DescPageInfos* r_desc);
    public:
        bool attach_init(); ///<attach��ʽ��ʼ��
        /**
         *afterStartMdb ���ݿ�������,����scn��Ϣ��.
         * ʧ�� �׳��쳣
         */
        void afterStartMdb();
    public:
        bool registerSession(SessionInfo& r_sessionInfo);
        bool unRegisterSession(const SessionInfo& r_sessionInfo);
        bool getSessionInfos(vector<SessionInfo> &r_sessionInfoList);
    public:
        //ֻ���𴴽���ṹ���޶�Ӧ������
        bool addTableDesc(const TableDesc& r_tableDesc, TableDesc* &r_pTableDesc, const T_SCN& r_scn);
        //ɾ����������ʱ��ǰ�ᣬ������Ѿ�truncate���Ҷ�Ӧ������Ҳȫ��ɾ��
        bool deleteTableDesc(const TableDesc& r_tableDesc, const T_SCN& r_scn);
        bool getTableDescList(vector<TableDesc> &r_tableDescList);
        bool getTableDescByName(const char* r_tableName, TableDesc* &r_pTableDesc);
    public:
        bool addIndexDesc(const IndexDesc& r_indexDesc, IndexDesc* &r_pIndexDesc, const T_SCN& r_scn);
        bool deleteIndexDesc(const IndexDesc& r_indexDesc, const T_SCN& r_scn);
        bool getIndexDescByName(const char* r_indexName, IndexDesc* &r_pIndexDesc);
        void getIndexDescByPos(const size_t& r_offSet, IndexDesc* &r_pIndexDesc);
    public:
        bool addGlobalParam(const GlobalParam& r_gParam, const T_SCN& r_scn);
        bool getGlobalParam(const char* r_paramname, GlobalParam& r_gParam);
        bool getGlobalParams(vector<GlobalParam>& r_gparamList);
        bool updateGlobalParam(const char* r_paramname, const char* r_value, const T_SCN& r_scn);
        bool deleteGlobalParam(const char* r_paramname, const T_SCN& r_scn);
    public:
        MDbGlobalInfo* getMdbGInfo()
        {
            return m_pGobalInfos;
        }
        void updateTbDescTime();
        void updateIndexTime();
        void updateSpTime();
        void updateDbTime();
        size_t getOffset(const char* r_addr)
        {
            return r_addr - getSpaceAddr();
        }
    protected:
        //��һ�δ���������
        bool firstCreator();
        //У���С�Ƿ��㹻
        bool checkSpaceSize();
        //attach �� init
        bool initAfterAttach();
    private:
        MdbCtlInfo*  m_pCtlInfo;
    protected: //�öζ���������ṹ����
        MDbGlobalInfo*              m_pGobalInfos;    ///<ȫ����Ϣ
        ListManager<SpaceInfo>      m_spInfoMgr;      ///<��ռ���Ϣ
        ListManager<GlobalParam>    m_gParamMgr;      ///<ȫ�ֲ���
        ListManager<TableDesc>      m_tbDescMgr;      ///<��������
        ListManager<IndexDesc>      m_idxDescMgr;     ///<����������
        ListManager<SessionInfo>    m_sessionInfoMgr; ///<Session��Ϣ
        friend class ChkptMgr;
    public:
        //���Խӿ�
        virtual bool dumpSpaceInfo(ostream& r_os);
};
#endif //_CONTROLMEMMGR_H_INCLUDE_20080422_
