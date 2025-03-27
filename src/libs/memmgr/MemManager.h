/**
*   MDB内存管理类.
*   提供内存操作的接口
*   @author 高将飞 <gaojf@lianchuang.com>
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
    public: //提供给Db接口:数据库级操作（访问进程）
        /**
         *openMdb 打开数据库连接.
         *@return  true 成功,失败 抛出异常
         */
        bool openMdb()  ;
        /**
         *openMdb 断开数据库连接.
         *@return  true 成功,失败 抛出异常
         */
        bool closeMdb()  ;
    public://提供给Db接口:数据库级操作(管理进程)
        /**
         *creatMdb 数据库创建.
         *         只包括控制区的创建(空数据库)
         *         参数等信息暂采用配置文件方式提供
         *         所有内容均未创建，包括文件表空间
         *@return  true 创建成功,失败 抛出异常
         */
        bool creatMdb() ;

        /**暂时不实现
         *dropMdb 数据库删除.
         *         清除所有内存数据库信息
         *@return  true 删除成功,失败 抛出异常
         */
        bool dropMdb()  ;

        /**
         *startMdb 数据库启动.
         *@return  true 启动成功,失败 抛出异常
         */
        bool startMdb()  ;
        /**
         *afterStartMdb 数据库启动后,清理scn和槽位信息等.
         * 失败 抛出异常
         */
        void afterStartMdb();
        /**
         *stopMdb 数据库停止.
         *@return  true 停止成功,失败 抛出异常
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
        //地址转换接口
        /**
         *getPhAddr 根据内部位置信息求取物理地址信息.
         *@param   r_mdbAddr: 输入为内部位置信息，输出为物理地址信息
         *@return  true 成功,失败 抛出异常
         */
        bool getPhAddr(MdbAddress& r_mdbAddr)  ;
        /**
         *getPhAddr 根据内部位置信息求取物理地址信息.
         *@param   r_shmPos: 输入为内部位置信息
         *@param   r_phAddr: 输出为物理地址信息
         *@return  true 成功,失败 抛出异常
         */
        bool getPhAddr(const ShmPosition& r_shmPos, char * &r_phAddr)  ;
        /**
         *getShmPos 根据物理地址信息求取内部位置信息.
         *@param   r_mdbAddr: 输入为物理地址信息，输出为内部位置信息
         *@return  true 成功,失败 抛出异常
         */
        bool getShmPos(MdbAddress& r_mdbAddr)  ;
        /**
         *memcopy 内存Copy.
         *@param   r_desAddr: 源物理地址
         *@param   r_srcAddr: 目标物理地址
         *@param   r_size   : 复制的字节数
         *@return  true 成功,失败 抛出异常
         */
        bool memcopy(void* r_desAddr, const void* r_srcAddr, const size_t& r_size)  ;
        /**
         *memcopy 内存Copy.
         *@param   r_desAddr: 源物理地址
         *@param   r_shmPos : 目标内部地址
         *@param   r_size   : 复制的字节数
         *@return  true 成功,失败 抛出异常
         */
        bool memcopy(void* r_desAddr, const ShmPosition& r_shmPos, const size_t& r_size)  ;
    public:
        void getlastckpscn(T_SCN& r_scn);
        void setlastckpscn(const T_SCN& r_scn);

        MDbGlobalInfo* getMdbGInfo() {
            return m_ctlMemMgr.getMdbGInfo();
        };

        /**
         *addGlobalParam 增加全局参数信息.
         *@param   r_gParam: 全局参数信息
         *@return  true 成功,失败 抛出异常
         */
        bool addGlobalParam(const GlobalParam& r_gParam)  ;
        /**
         *getGlobalParam 获取全局参数信息.
         *@param   r_paramname: 参数名称
         *@param   r_gParam   : 存放获取的参数信息
         *@return  true 成功,失败 抛出异常
         */
        bool getGlobalParam(const char* r_paramname, GlobalParam& r_gParam)  ;
        /**
         *getGlobalParams 获取所有全局参数信息.
         *@param   r_gparamList   : 存放获取的参数信息
         *@return  true 成功,false 失败
         */
        bool getGlobalParams(vector<GlobalParam>& r_gparamList)  ;
        /**
         *updateGlobalParam 更新全局参数信息.
         *@param   r_paramname   : 参数名称
         *@param   r_value       : 值
         *@return  true 成功,失败 抛出异常
         */
        bool updateGlobalParam(const char* r_paramname, const char* r_value)  ;
        /**
         *deleteGlobalParam 删除全局参数信息.
         *@param   r_paramname   : 参数名称
         *@return  true 成功,失败 抛出异常
         */
        bool deleteGlobalParam(const char* r_paramname)  ;
        /**
         *registerSession 登记Session信息.
         *@param   r_sessionInfo   : SESSION信息
         *@return  true 成功,失败 抛出异常
         */
        bool registerSession(SessionInfo& r_sessionInfo)  ;
        /**
         *unRegisterSession 注销Session信息.
         *@param   r_sessionInfo   : SESSION信息
         *@return  true 成功,失败 抛出异常
         */
        bool unRegisterSession(const SessionInfo& r_sessionInfo)  ;
        /**
         *getSessionInfos 获取所有Session信息.
         *@param   r_sessionInfoList   : 存放获取的信息
         *@return  true 成功,失败 抛出异常
         */
        bool getSessionInfos(vector<SessionInfo> &r_sessionInfoList)  ;
        bool clearSessionInfos(); //add by gaojf 2009-3-2 4:05

    public:
        /**
         *createTbSpace 根据表空间参数信息,创建数据表空间.
         *@param   r_spaceInfo   : 表空间信息,需要设置表空间名称
         *                          大小、类型等信息
         *@param   t_flag        : 0 第一次创建, 非0 非第一次创建
         *@return  true 成功,失败 抛出异常
         */
        bool createTbSpace(SpaceInfo& r_spaceInfo, const int& r_flag = 0)  ;

        /**
         *getSpaceInfo 根据表空间名称取对应的表空间信息.
         *@param   r_spaceName   : 表空间名称
         *@param   r_spaceInfo   : 表空间信息
         *@return  true 成功,失败 抛出异常
         */
        bool getSpaceInfo(const char* r_spaceName, SpaceInfo& r_spaceInfo)  ;

        /**
         *getSpaceInfoList 取所有数据区和索引区表空间信息.
         *@param   r_spaceInfoList   : 存放取得的表空间信息
         *@return  true 成功,失败 抛出异常
         */
        bool getSpaceInfoList(vector<SpaceInfo> &r_spaceInfoList)  ;

        /**
         *addTableSpace 给表或索引绑定表空间.
         *@param   r_spaceName   : 表空间名称
         *@param   r_tableName   : 表名或索引名
         *@param   r_tableType   : 表类型
         *@return  true 成功,失败 抛出异常
         */
        bool addTableSpace(const char* r_spaceName, const char* r_tableName, const T_TABLETYPE& r_tableType)  ;

        /**
         *getSpaceListByTable 根据表取其绑定的表空间名称.
         *@param   r_tableName   : 表名
         *@param   r_spaceList   : 存放表关联的表空间名称
         *@return  true 成功,失败 抛出异常
         */
        bool getSpaceListByTable(const char* r_tableName, vector<string> &r_spaceList)  ;
        /**
         *getSpaceListByIndex 根据索引取其绑定的表空间名称.
         *@param   r_indexName   : 索引名
         *@param   r_spaceList   : 存放表关联的表空间名称
         *@return  true 成功,失败 抛出异常
         */
        bool getSpaceListByIndex(const char* r_indexName, vector<string> &r_spaceList)  ;

        /**
         *updateMgrInfo 更新表空间管理器(表空间新增等情况需要更新).
         *@return  true 成功,失败 抛出异常
         */
        bool updateMgrInfo()  ; //更新数据表空间管理信息
        /**
         *getPageInfo 根据页面内部位置信息取其物理指针.
         *@param r_pagePos:页面内部位置信息
         *@param r_pPage  :页面物理指针
         *@return  true 成功，false 失败
         */
        bool getPageInfo(const ShmPosition& r_pagePos, PageInfo *&r_pPage);
        //根据Slot地址，取对应的页面信息和页面地址
        bool getPageBySlot(const ShmPosition& r_slot, ShmPosition& r_pagePos, PageInfo* &r_pPage);
    public:
        /**
         *createTable 根据表定义创建表描述符(不负责索引创建和内存申请).
         *@param r_tableDef : 表定义
         *@param r_tableDesc: 表描述符指针
         *@return  true 成功,失败 抛出异常
         */
        bool createTable(const TableDef& r_tableDef, TableDesc* &r_tableDesc)  ;
        /**
         *dropTable 删除表（包括删除表包含的索引、释放内存）.
         *@param r_tableName : 表名称
         *@return  true 成功,失败 抛出异常
         */
        bool dropTable(const char* r_tableName)  ;
        /**
         *truncateTable 清空表,释放内存（包括索引的清空）.
         *@param r_tableName : 表名称
         *@return  true 成功,失败 抛出异常
         */
        bool truncateTable(const char* r_tableName)  ;
        /**
         *allocateTableMem 表申请内存.
         *@param r_tableDesc : 表描述符指针(指向共享内存)
         *@param r_num       : 节点数
         *@param r_slotAddrList: 存放申请到的内存地址信息(包含物理地址和偏移量)
         *@return  true 成功,失败 抛出异常
         */
        bool allocateTableMem(TableDesc* &r_tableDesc, const int& r_num,
                              vector<MdbAddress>& r_slotAddrList);


        /**
         *createIndex 创建索引描述符：空索引(索引内容的创建在索引管理类中管理).
         *@param r_idxDef : 索引定义(指向共享内存)
         *@param r_idxDesc: 索引描述符(指向共享内存)
         *@return  true 成功,失败 抛出异常
         */
        bool createIndex(const IndexDef& r_idxDef, IndexDesc* &r_idxDesc)  ;
        /**
         *dropIndex 删除索引.
         *@param r_idxName : 索引名称
         *@param r_tableName: 表名称(可缺省)
         *@return  true 成功,失败 抛出异常
         */
        bool dropIndex(const char* r_idxName, const char* r_tableName = 0)  ;
        /**
         *truncateIndex 清空索引（保留索引描述符）.
         *@param r_idxName : 索引名称
         *@param r_tableName: 表名称(可缺省)
         *@return  true 成功,失败 抛出异常
         */
        bool truncateIndex(const char* r_idxName, const char* r_tableName = 0)  ;
        /**
         *allocateIdxMem 申请索引内存.
         *@param r_indexDesc : 索引描述符
         *@param r_num       : 索引节点个数
         *@param r_addrList  : 存放申请到的内存地址信息(包含物理地址和偏移量)
         *@return  true 成功,失败 抛出异常
         */
        bool allocateIdxMem(IndexDesc& r_indexDesc, const int& r_num, vector<MdbAddress> &r_addrList)  ;
        /**
         *allocateIdxMem 申请一个索引节点内存.
         *@param r_indexDesc : 索引描述符
         *@param r_addr      : 存放申请到的内存地址信息(包含物理地址和偏移量)
         *@return  true 成功,失败 抛出异常
         */
        bool allocateIdxMem(IndexDesc* &r_pIndexDesc, MdbAddress& r_addr)  ;

        /**
         *getTableDescList 取所有表描述符信息.
         *@param r_tableDescList : 存放表描述符信息
         *@return  true 成功,失败 抛出异常
         */
        bool getTableDescList(vector<TableDesc>& r_tableDescList)  ;
        /**
         *getTableDefList 取所有表定义信息（不包括索引定义）.
         *@param r_tableDefList : 存放表描述符信息
         *@return  true 成功,失败 抛出异常
         */
        bool getTableDefList(vector<TableDef>& r_tableDefList)  ;

        /**
         *getTableDescByName 根据表名取表描述符信息.
         *@param r_tableName : 表名
         *@param r_pTableDesc: 表描述符指针(指向共享内存)
         *@return  true 成功,失败 抛出异常
         */
        bool getTableDescByName(const char* r_tableName, TableDesc* &r_pTableDesc)  ;
        /**
         *getIndexListByTable 根据表描述符取索引信息.
         *@param r_tablDesc : 表描述符
         *@param r_pIndexList: 存放该表对应的所有索引描述符指针列表
         *@return  true 成功,失败 抛出异常
         */
        bool getIndexListByTable(const TableDesc& r_tablDesc, vector<IndexDesc*> &r_pIndexList)  ;
        /**
         *getIndexListByTable 根据表描述符取索引信息.
         *@param r_tablDesc : 表描述符
         *@param r_pIndexList: 存放该表对应的所有索引定义信息
         *@return  true 成功,失败 抛出异常
         */
        bool getIndexListByTable(const char* r_tableName, vector<IndexDef> &r_pIndexList)  ;
        /**
         *getIndexDescByName 根据索引名称取索引信息.
         *@param r_indexName : 索引名称
         *@param r_pIndexDesc: 索引描述符（指向共享内存）
         *@return  true 成功,失败 抛出异常
         */
        bool getIndexDescByName(const char* r_indexName, IndexDesc* &r_pIndexDesc)  ;
        /**
         *getTableMemInfo 根据表名/索引名取对应的内存占用信息.
         *@param r_tableName : 表名/索引名
         *@param r_tbMemInfo : 内存占用信息
         *@return  true 成功,失败 抛出异常
         */
        bool getTableMemInfo(const char* r_tableName, TbMemInfo& r_tbMemInfo)  ;
        /**
         *getSlotByShmPos 根据偏移量取Slot指针.
         *@param r_shmPos : 内存偏移量
         *@param r_pSlot  : Slot指针（指向共享内存）
         *@return  true 成功,失败 抛出异常
         */
        bool getSlotByShmPos(const ShmPosition& r_shmPos, UsedSlot* &r_pSlot)  ;
        UndoMemMgr* getUndoMemMgr()
        {
            return &m_undoMemMgr;
        }
        //r_flag 为1 SCN 自动+1  0不加
        void        getscn(T_SCN& r_scn, const int r_flag = 1)
        {
            m_ctlMemMgr.getscn(r_scn, r_flag);
        }
        void        gettid(long& transid)
        {
            m_ctlMemMgr.gettid(transid);
        }

        /**
         *freeIdxMem 释放索引内存.
         *@param r_indexDesc : 索引描述符
         *@param r_addrList  : 存放需要释放的内存地址信息(必须有偏移量)
         *@return  true 成功,失败 抛出异常
         */
        bool freeIdxMem(IndexDesc& r_indexDesc, const vector<ShmPosition> &r_addrList, const T_SCN& r_scn)  ;
        /**
         *freeIdxMem 释放一个索引节点内存.
         *@param r_indexDesc : 索引描述符
         *@param r_addr  : 存放需要释放的内存地址信息(必须有偏移量)
         *@return  true 成功,失败 抛出异常
         */
        bool freeIdxMem(IndexDesc& r_indexDesc, const ShmPosition& r_addr, const T_SCN& r_scn)  ;

        bool addRedoConfig(const char* key, const size_t& value);
        bool addRedoConfig(const char* key, const char* value);
    protected:
        DescPageInfos* getdescinfo(const char& r_type, const size_t& r_offset);
        size_t getdescOffset(const char& r_type, const DescPageInfos* r_desc);

        /**
         *freeTableMem 表释放内存.
         *@param r_tableDesc : 表描述符指针(指向共享内存)
         *@param r_slotAddrList: 存放需要释放的内存地址信息
         *                     : r_slotAddrList必须有ShmPosition信息
         *@return  true 成功,失败 抛出异常
         */
        bool freeTableMem(TableDesc* r_tableDesc, const vector<MdbAddress>& r_slotAddrList, const T_SCN& r_scn);
        bool freeTableMem(TableDesc* r_tableDesc, const MdbAddress& r_slotAddr, const T_SCN& r_scn);
        /**
         *getSpAddrList 取表空间地址信息(控制区表空间除外).
         *r_refetch  0-不重新获取地址，1-重新获取地址 //modified by gaojf 2012/5/10 9:13:11
         *@return  无
         */
        void getSpAddrList(const int& r_refetch = 0);
        /**
         *createSpace 创建一个表空间.
         *@param r_spaceInfo : 表空间定义信息
         *@param r_flag  : 第一次创建标记：0第一次，1否
         *@return  true 成功,false 失败
         */
        bool createSpace(SpaceInfo& r_spaceInfo, const int& r_flag);
        /**
         *createSpace 创建各个表空间.
         *@param r_spaceInfo : 表空间定义信息列表
         *@param r_flag  : 第一次创建标记：0第一次，1否
         *@return  true 成功,false 失败
         */
        bool createSpace(const int& r_flag, vector<SpaceInfo> &r_spaceInfo);
        /**
         *attachSpace attach各个表空间(索引和数据).
         *@return  true 成功,false 失败
         */
        bool attachSpace();
        /**
         *detachSpace detach各个表空间(索引和数据)
         *@return  true 成功,false 失败
         */
        bool detachSpace();
        /**
         *initSpace 初始化各个表空间0第一次创建，1非第一次
         *@param r_flag  : 0第一次，1否
         *@return  true 成功,false 失败
         */
        bool initSpace(const int r_flag);

        /**
         *allocateMem 申请内存（r_num个节点）
         *@param r_spaceNum: 表空间个数
         *@param r_spaceList:表空间列表
         *@param r_descPageInfo:描述符对应的内存页面信息
         *@param r_num   : 申请的节点数
         *@param r_slotAddrList  : 存放申请到的节点地址(包括内部地址信息和物理地址信息)
         *@param r_flag          : 0 表,需要将Slot串起来, 1索引，不需串
         *@return  true 成功,失败 抛出异常
         */
        bool allocateMem(const int& r_spaceNum, const T_NAMEDEF r_spaceList[MAX_SPACE_NUM],
                         DescPageInfos* r_descpageinfogs, const int& r_num,
                         vector<MdbAddress>& r_slotAddrList,
                         const int& r_flag);
        /**
         *freeMem 释放一个节点的内存
         *@param r_descPageInfo:描述符对应的内存页面信息
         *@param r_addr  : 节点的内部地址信息
         *@param r_falg          : 0 表,需要将Slot串起来, 1索引，不需串
         *@return  true 成功,失败 抛出异常
         */
        bool freeMem(DescPageInfos* r_descpageinfogs, const ShmPosition& r_addr,
                     const int& r_flag, const T_SCN& r_scn)  ;
        /**
         *getSessionId 获取SESSIONID.
         *@return  获取到的Id,-1表示失败
         */
        void  getSessionId(int& sessionid, short int& semmark); //获取SessionId
        /**
         *realseSid 释放SESSIONID.
         *@param   r_sid: 需要释放的SESSIONID
         *@return  true 成功,false 失败
         */
        bool realseSid(const int& r_sid);

    protected:
        T_NAMEDEF         m_mdbName;      ///<数据库名称
        MdbCtlInfo*       m_pMdbCtlInfo;  ///<控制信息
        ControlFile       m_ctlFile;      ///<数据库控制文件
        vector<SpaceInfo> m_spaceInfoList;///<表空间信息
        /////////////////////////////////////////////////////
        ControlMemMgr     m_ctlMemMgr;    ///<控制区内存管理
        TableMemMgr       m_tableMemMgr;  ///<数据区内存管理
        IndexMemMgr       m_indexMemMgr;  ///<索引区内存管理
        UndoMemMgr        m_undoMemMgr;   ///<Undo区内存管理
        /////////////////////////////////////////////////////
        MdbAddressMgr     m_addressMgr;   ///<数据库地址管理器(转换)
        MdbStatusMgr      m_statusMgr;    ///<MDB状态管理器
        MdbStatus*        m_pstatus;      ///<状态指针
        MDBLatchMgr*      m_pddlLock;     ///<DDLLOCK
        SrcLatchInfo*     m_ddlLatch;     ///<DDLLOCKINFO
        friend class ChkptMgr;
    private:
        bool        m_openFlag;     ///<是否已打开连接标记
        time_t      m_spUpdateTime; ///<表空间刷新时间
    public:
        //调试接口
        virtual bool dumpSpaceInfo(ostream& r_os);
        // 内存使用率 add by chenm
        // 默认返回所有表空间使用率
        void getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName = NULL);
};
#endif //_MEMMANAGER_H_INCLUDE_200080411_

