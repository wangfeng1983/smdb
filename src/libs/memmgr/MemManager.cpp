#include "MemManager.h"
#include "redo/RedoConst.h"
#include <iostream>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "MDBLatchShmMgr.h"


MemManager::MemManager(const char* r_dbName)
{
    strcpy(m_mdbName, r_dbName);
    m_spaceInfoList.clear();
    m_openFlag = false;
    m_pddlLock = MDBLatchMgr::getInstance(m_mdbName);
    m_ddlLatch = m_pddlLock->getSrcLatchInfo(T_DBLK_DB, "__DDL_LOCK__");
    m_undoMemMgr.setMemMgr(this);
}
MemManager::~MemManager()
{
    closeMdb();
}

/**
 *creatMdb 数据库创建.
 *         只包括控制区的创建(空数据库)
 *         参数等信息暂采用配置文件方式提供
 *         所有内容均未创建，包括文件表空间
 *@return  true 创建成功,false 失败
 */
bool MemManager::creatMdb()
{
#ifdef _DEBUG_
    cout << "\t开始读取配置文件......" << __FILE__ << __LINE__ << endl;
#endif
    //1. 读取配置文件中的信息
    Mdb_Config  t_dbConfig;
    if (t_dbConfig.getConfigInfo(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "r_dbConfig.getConfigInfo(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "取该数据库对应的配置信息失败!");
    }
#ifdef _DEBUG_
    cout << "\t读取配置文件成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始创建控制文件......" << __FILE__ << __LINE__ << endl;
#endif
    //2. 创建控制文件
    if (m_ctlFile.createFile(t_dbConfig) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlFile.createFile(t_dbConfig) failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "创建控制文件失败!");
    }
    m_pMdbCtlInfo = m_ctlFile.getCtlInfoPt();
    getMdbStatus();
    m_pstatus->setvalue(-1, 0, T_SCN());
    m_pstatus->setpath(m_pMdbCtlInfo->m_datapath);
    writeStatus();
#ifdef _DEBUG_
    cout << "\t创建控制文件成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始创建控制区表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //3. 创建控制区表空间
    //3.1初始化
    m_ctlMemMgr.initialize(*m_pMdbCtlInfo);
    int t_flag = 0; //第一次创建
    //3.2.创建控制区表空间
    if (m_ctlMemMgr.createControlSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.createControlSpace(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "创建控制区表空间失败!");
    }
    //3.3.连接控制区表空间
    if (m_ctlMemMgr.attach() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "连接控制区表空间失败!");
    }
    //3.4 初始化控制区表空间信息
    if (m_ctlMemMgr.initCtlDataInfo(t_flag, m_pstatus->getpath()) == false)
    {
#ifdef _DEBUG_
        cout << "initCtlDataInfo.attach(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "初始化控制区表空间信息失败!");
    }
#ifdef _DEBUG_
    cout << "\t创建控制区表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始创建其它表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //3.5 创建其它表空间
    if (createSpace(t_flag, t_dbConfig.m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "createSpace() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "创建表空间失败!");
    }
#ifdef _DEBUG_
    cout << "\t创建表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始连接其它表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //4 连接其它表空间
    if (attachSpace() == false)
    {
#ifdef _DEBUG_
        cout << "attach() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "连接表空间失败!");
    }
#ifdef _DEBUG_
    cout << "\t连接其它表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始初始化其它表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //5. 初始化数据库区和索引区
    if (initSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "initSpace(" << t_flag << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "连初始化数据库区和索引区失败!");
    }
    //获取数据区和索引区的表空间信息
    m_tableMemMgr.getSpaceInfoList(m_spaceInfoList);
    m_spaceInfoList.push_back(*(m_indexMemMgr.getSpaceInfo()));
    m_spaceInfoList.push_back(*(m_undoMemMgr.getSpaceInfo()));
    getSpAddrList(); //取地址信息
#ifdef _DEBUG_
    cout << "\t初始化其它表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t将表空间数据备份出去......" << __FILE__ << __LINE__ << endl;
#endif
    m_openFlag = true;
    //测试时注释
    //7. 停止数据库
    stopMdb();
    return true;
}

/**
 *dropMdb 数据库删除.
 *         清除所有内存数据库信息
 *         参数等信息暂采用配置文件方式提供
 *@return  true 删除成功,false 失败
 */
bool MemManager::dropMdb()
{
    //coding later
    return true;
}

/**
 *startMdb 数据库启动.
 *         数据库正常停止或异常恢复后的启动
 *         参数等信息暂采用配置文件方式提供
 *@return  true 启动成功,false 失败
 */
bool MemManager::startMdb()
{
    m_spUpdateTime = 0;
    //1. 读取配置文件中的信息 add by gaojf
    Mdb_Config  t_dbConfig;
    if (t_dbConfig.getConfigInfo(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "r_dbConfig.getConfigInfo(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "取该数据库对应的配置信息失败!");
        return false;
    }
    if (m_ctlFile.initialize(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlFile.initialize(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "控制文件初始化失败!");
        return false;
    }
    m_pMdbCtlInfo = m_ctlFile.getCtlInfoPt();
    getMdbStatus();
#ifdef _DEBUG_
    m_pstatus->dump();
    cout << "\t开始创建控制区表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //3. 创建控制区表空间
    //3.1初始化
    m_ctlMemMgr.initialize(*m_pMdbCtlInfo);
    int t_flag = 1; //非第一次创建
    //3.2.创建控制区表空间
    if (m_ctlMemMgr.createControlSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.createControlSpace(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "创建控制区表空间失败!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t开始连接控制区表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //3.3.连接控制区表空间
    if (m_ctlMemMgr.attach() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "连接控制区表空间失败!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t开始初始化控制区表空间信息......" << __FILE__ << __LINE__ << endl;
#endif
    //3.4 初始化控制区表空间信息
    if (m_ctlMemMgr.initCtlDataInfo(t_flag, m_pstatus->getpath()) == false)
    {
#ifdef _DEBUG_
        cout << "initCtlDataInfo.attach(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "初始化控制区表空间信息失败!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t创建控制区表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始连接其它表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //从控制区表空间中取其它表空间信息
    if (getSpaceInfoList(m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "getSpaceInfoList failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "从控制区表空间中取其它表空间信息失败!");
        return false;
    }
    //为了支持数据文件重用,重新生成shmKey add by gaojf 2010/11/23 11:05:47 begin
    for (vector<SpaceInfo>::iterator t_spItr = m_spaceInfoList.begin();
            t_spItr != m_spaceInfoList.end(); ++t_spItr)
    {
        int t_shmKey = ftok(m_ctlFile.getFileName(), t_spItr->m_spaceCode);
        if (t_spItr->m_shmKey != t_shmKey)
        {
            t_spItr->m_shmKey = t_shmKey;
            if (m_ctlMemMgr.updateSpaceInfo(*t_spItr) == false)
            {
#ifdef _DEBUG_
                cout << "updateSpaceInfo(*t_spItr) failed!" << __FILE__ << __LINE__ << endl;
#endif
                throw Mdb_Exception(__FILE__, __LINE__, "更新数据对应表空间的shmKey失败!");
                return false;
            }
        }
        
        //控制区表空间以配置文件中最后调节的为准 2012/9/18 7:08:50 gaojf
        if (strcasecmp(m_pMdbCtlInfo->m_undoSpaceName, t_spItr->m_spaceName) == 0)
        {
            for (vector<SpaceInfo>::iterator t_confspItr = t_dbConfig.m_spaceInfoList.begin();
                    t_confspItr != t_dbConfig.m_spaceInfoList.end(); t_confspItr++)
            {
                if (strcasecmp(m_pMdbCtlInfo->m_undoSpaceName, t_confspItr->m_spaceName) == 0)
                {
                    t_spItr->m_size = t_confspItr->m_size;
                    if (m_ctlMemMgr.updateSpaceInfo(*t_spItr) == false)
                    {
#ifdef _DEBUG_
                        cout << "updateSpaceInfo(*t_spItr) failed!" << __FILE__ << __LINE__ << endl;
#endif
                        throw Mdb_Exception(__FILE__, __LINE__, "更新数据对应表空间的shmKey失败!");
                        return false;
                    }
                    break;
                }
            }
        }
    }
    //end gaojf 2010/11/23 11:05:52
    //3.5 创建其它表空间
    if (createSpace(t_flag, m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "createSpace() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "创建表空间失败!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t创建表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始连接其它表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //4 连接其它表空间
    if (attachSpace() == false)
    {
#ifdef _DEBUG_
        cout << "attach() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "连接表空间失败!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t连接其它表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\t开始初始化其它表空间......" << __FILE__ << __LINE__ << endl;
#endif
    //5. 初始化数据库区和索引区
    if (initSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "initSpace(" << t_flag << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "初始化数据库区和索引区失败!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t初始化其它表空间成功......" << __FILE__ << __LINE__ << endl;
    cout << "\tStartMdb OK!" << endl;
#endif
    getSpAddrList(1);
    m_openFlag = true;
    //根据表描述符定义创建对应的Undo表
    vector<TableDesc> t_tabledesclist;
    getTableDescList(t_tabledesclist);
    m_undoMemMgr.createUndoDescs(t_tabledesclist);
    //增加redo日志配置信息
    addRedoConfig("DB_NAME", m_mdbName);
    addRedoConfig(redo_const::param::PATH, t_dbConfig.m_redoInfo.m_path);
    addRedoConfig(redo_const::param::LOGFILE_SIZE, t_dbConfig.m_redoInfo.m_logfilesize);
    addRedoConfig(redo_const::param::GROUP_COUNT, t_dbConfig.m_redoInfo.m_groupcount);
    addRedoConfig(redo_const::param::BUFFER_COUNT, t_dbConfig.m_redoInfo.m_buffercount);
    addRedoConfig(redo_const::param::BUFFER_SIZE, t_dbConfig.m_redoInfo.m_buffersize);
    addRedoConfig(redo_const::param::WRITE_MODE, t_dbConfig.m_redoInfo.m_writemode);
    addRedoConfig(redo_const::param::SYNC_MODE, t_dbConfig.m_redoInfo.m_syncmode);
    addRedoConfig("LOGWRITER_CHANNEL_COUNT", t_dbConfig.m_redoInfo.m_chnlst.size());
    for (int i = 0; i < t_dbConfig.m_redoInfo.m_chnlst.size(); i++)
    {
        T_NAMEDEF key;
        sprintf(key, "LOGWRITER_CHANNEL_%d", i);
        addRedoConfig(key, t_dbConfig.m_redoInfo.m_chnlst[i]);
    }
    //add by gaojf 2012/5/8 14:49:41 begin
    //对于数据文件不是backup出来的,
    //需要对新增的表DATA_TABLE_NP和索引HASH_TREE_NP初始化。
    //需要对新增的索引HASH_INDEX_NP 对应的索引区信息（初始化）
    TableDesc* t_pTableDesc = NULL;
    vector<IndexDesc*> t_pIndexList;
    T_SCN t_scn;
    getscn(t_scn);
    int   t_i = 0;
    if (m_pstatus->m_filetype != 1)
    {
        for (vector<TableDesc>::const_iterator t_tbdescitr = t_tabledesclist.begin();
                t_tbdescitr != t_tabledesclist.end(); ++t_tbdescitr)
        {
            if (t_tbdescitr->m_tableDef.m_tableType == DATA_TABLE_NP)
            {
                if (getTableDescByName(t_tbdescitr->m_tableDef.m_tableName, t_pTableDesc) == false)
                {
#ifdef _DEBUG_
                    cout << "getTableDescByName(" << t_tbdescitr->m_tableDef.m_tableName << ") false!" << __FILE__ << __LINE__ << endl;
#endif
                    return false;
                }
                //1.初始化表起始终止页面信息
                for (t_i = 0; t_i < MAX_INDEX_NUM; ++t_i)
                {
                    t_pTableDesc->m_pageInfo[t_i].initPos();
                }
                t_pIndexList.clear();
                getIndexListByTable(*t_tbdescitr, t_pIndexList);
                for (vector<IndexDesc*>::iterator t_pidxitr = t_pIndexList.begin();
                        t_pidxitr != t_pIndexList.end(); ++t_pidxitr)
                {
                    //2.初始化索引起始终止页面信息
                    for (t_i = 0; t_i < MAX_INDEX_NUM; ++t_i)
                    {
                        (*t_pidxitr)->m_pageInfo[t_i].initPos();
                    }
                    //3.初始化索引区信息
                    m_indexMemMgr.initHashSeg((*t_pidxitr)->m_header, t_scn);
                }
            }
        }
    }
    //add by gaojf 2012/5/8 14:49:41 end
    return true;
}

/**
 *stopMdb 数据库停止.
 *         参数等信息暂采用配置文件方式提供
 *@return  true 停止成功,false 失败
 */
bool MemManager::stopMdb()
{
    bool t_bRet = true;
    if (m_openFlag == false)
    {
        //如果已经断开，则连接
        openMdb();
    }
    else
    {
        updateMgrInfo();
    }
    backupMdb();
    closeMdb();
    //所有表空间信息均已获得
    //2. 删除表空间（内存部分）
    if (m_tableMemMgr.deleteAllTbSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_tableMemMgr.deleteAllTbSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "删除数据区表空间失败!");
    }
    //3. 删除索引空间
    if (m_indexMemMgr.deleteIdxSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_indexMemMgr.deleteIdxSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "删除索引空间失败!");
    }
    //4. 删除Undo表空间
    m_undoMemMgr.deleteUndoSpace();
    //5. 删除控制区表空间
    if (m_ctlMemMgr.deleteControlSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.deleteControlSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "删除控制区表空间!");
    }
    //6. 删除latch区域共享内存 by chenm 2010-11-03 11:37:20
    if (MDBLatchShmMgr::deleteLatchShm(m_pddlLock->m_latchshminfo) == false)
    {
#ifdef _DEBUG_
        cout << "MDBLatchShmMgr::deleteLatchShm(m_pddlLock->m_latchshminfo) false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "删除latch共享空间失败!");
    }
    MDBLatchMgr::unregister(m_mdbName, pthread_self());
    return t_bRet;
}
//打开数据库连接
bool MemManager::openMdb()
{
    if (m_ctlFile.initialize(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlFile.initialize(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "控制文件初始化失败!");
        return false;
    }
    m_pMdbCtlInfo = m_ctlFile.getCtlInfoPt();
    getMdbStatus();
    //3 初始化控制区管理类
    m_ctlMemMgr.initialize(*m_pMdbCtlInfo);
    //3.3.连接方式初始化控制区表空间
    if (m_ctlMemMgr.attach_init() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.attach_init() false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "连接控制区表空间失败!");
        return false;
    }
    //从控制区表空间中取其它表空间信息
    if (getSpaceInfoList(m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "getSpaceInfoList failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "从控制区表空间中取其它表空间信息失败!");
        return false;
    }
    //2 根据配置信息创建索引和数据区表空间
    for (vector<SpaceInfo>::iterator t_spInfoItr = m_spaceInfoList.begin();
            t_spInfoItr != m_spaceInfoList.end(); t_spInfoItr++)
    {
        switch (t_spInfoItr->m_type)
        {
            case TABLE_SPACE://数据区表空间
            case TABLE_SPACE_NP: //不需要持久化的数据区表空间 2012/5/7 20:28:26 gaojf
                //3. 初始化表空间
                if (m_tableMemMgr.attach_int(*t_spInfoItr) == false)
                {
#ifdef _DEBUG_
                    cout << "m_tableMemMgr.attach_int(*t_spInfoItr) false!" << __FILE__ << __LINE__ << endl;
#endif
                    throw Mdb_Exception(__FILE__, __LINE__, "初始化数据区表空间失败!");
                    return false;
                }
                break;
            case INDEX_SPACE://索引区表空间
                //3. 初始化表空间
                if (m_indexMemMgr.attach_init(*t_spInfoItr) == false)
                {
#ifdef _DEBUG_
                    cout << "m_indexMemMgr.attach_int(*t_spInfoItr) false!" << __FILE__ << __LINE__ << endl;
#endif
                    throw Mdb_Exception(__FILE__, __LINE__, "初始化索引区表空间失败!");
                    return false;
                }
                break;
            case UNDO_SPACE: //Undo表空间
                //4.初始化表空间
                if (m_undoMemMgr.attach_init(*t_spInfoItr) == false)
                {
#ifdef _DEBUG_
                    cout << "m_undoMemMgr.attach_init(r_spaceInfo) false!" << __FILE__ << __LINE__ << endl;
#endif
                    return false;
                }
                break;
            default:
#ifdef _DEBUG_
                cout << "表空间类型错误!spaceType=" << t_spInfoItr->m_type
                     << " " << __FILE__ << __LINE__ << endl;
#endif
                throw Mdb_Exception(__FILE__, __LINE__, "表空间类型失败!");
                return false;
        };
    }
    getSpAddrList();
    m_undoMemMgr.setMemMgr(this);
    m_openFlag = true;
#ifdef _DEBUG_
    cout << "\topenMdb OK!" << endl;
    m_addressMgr.dump();
#endif
    return true;
}
//断开数据库连接
bool MemManager::closeMdb()
{
    bool t_bRet = true;
    if (m_openFlag == true)
    {
        //1.detach 各表空间
        if (detachSpace() == false)
        {
#ifdef _DEBUG_
            cout << "detachSpace()() false!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "detach表空间失败!");
        }
        m_openFlag = false;
    }
    return t_bRet;
}


//根据表空间参数信息,创建表空间
//并将该信息填上
bool MemManager::createSpace(SpaceInfo& r_spaceInfo, const int& r_flag)
{
    bool t_bret = true;
    //1. 获取表空间代码
    if (r_flag == 0) //第一次需要设置SpaceCode和大小
    {
        r_spaceInfo.m_spaceCode = m_ctlMemMgr.getNSpaceCode();
        //r_spaceInfo.m_pageSize =m_pMdbCtlInfo->m_pageSize;
    }
    //2. 创建表空间
    switch (r_spaceInfo.m_type)
    {
        case TABLE_SPACE://数据区表空间
        case TABLE_SPACE_NP: //不需要持久化的数据区表空间 2012/5/7 20:28:26 gaojf
            //3. 初始化表空间
            if (m_tableMemMgr.createTbSpace(r_spaceInfo, r_flag) == false)
            {
#ifdef _DEBUG_
                cout << "m_tableMemMgr.createTbSpace(r_spaceInfo) false!" << __FILE__ << __LINE__ << endl;
#endif
                t_bret = false;
            }
            break;
        case INDEX_SPACE://索引区表空间
            m_indexMemMgr.initialize(r_spaceInfo);
            //3. 初始化表空间
            if (m_indexMemMgr.createIdxSpace(r_spaceInfo, r_flag) == false)
            {
#ifdef _DEBUG_
                cout << "m_indexMemMgr.createIdxSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
                t_bret = false;
            }
            break;
        case UNDO_SPACE: //Undo表空间
            m_undoMemMgr.initialize(r_spaceInfo);
            if (m_undoMemMgr.createUndoSpace(r_spaceInfo) == false)
            {
#ifdef _DEBUG_
                cout << "m_undoMemMgr.createUndoSpace(r_spaceInfo) false!" << __FILE__ << __LINE__ << endl;
#endif
                t_bret = false;
            }
            break;
        default:
#ifdef _DEBUG_
            cout << "表空间类型错误!spaceType=" << r_spaceInfo.m_type
                 << " " << __FILE__ << __LINE__ << endl;
#endif
            t_bret = false;
    };
    if (r_flag == 0) //第一次需要将表空间信息加入控制区表空间
    {
        //4. 在控制区中加表空间信息
        if (m_ctlMemMgr.addSpaceInfo(r_spaceInfo) == false)
        {
#ifdef _DEBUG_
            cout << "m_ctlMemMgr.addSpaceInfo(r_spaceInfo) false!"
                 << __FILE__ << __LINE__ << endl;
#endif
            t_bret = false;
        }
    }
    return t_bret;
}
bool MemManager::getSpaceInfo(const char* r_spaceName, SpaceInfo& r_spaceInfo)
{
    if (m_ctlMemMgr.getDSpaceInfo(r_spaceName, r_spaceInfo) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "该表空间不存在!");
        return false;
    }
    return true;
}

bool MemManager::getSpaceInfoList(vector<SpaceInfo> &r_spaceInfoList)
{
    time_t t_spTime = m_ctlMemMgr.getMdbGInfo()->m_spUpTime;
    if (m_ctlMemMgr.getSpaceInfos(r_spaceInfoList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "取表空间列表信息失败!");
        return false;
    }
    m_spUpdateTime = t_spTime;
    return true;
}

bool MemManager::getPhAddr(MdbAddress& r_mdbAddr)
{
    if (m_addressMgr.getPhAddr(r_mdbAddr) == false)
    {
        updateMgrInfo();
        if (m_addressMgr.getPhAddr(r_mdbAddr) == false)
        {
            abort();
            throw Mdb_Exception(__FILE__, __LINE__, "根据偏移量取物理地址失败!");
            return false;
        }
    }
    return true;
}
bool MemManager::getPhAddr(const ShmPosition& r_shmPos, char * &r_phAddr)
{
    if (m_addressMgr.getPhAddr(r_shmPos, r_phAddr) == false)
    {
        updateMgrInfo();
        if (m_addressMgr.getPhAddr(r_shmPos, r_phAddr) == false)
        {
            cout << "r_shmPos={" << r_shmPos << "} " << __FILE__ << __LINE__ << endl;
            abort();
            throw Mdb_Exception(__FILE__, __LINE__, "根据偏移量取物理地址失败!");
            return false;
        }
    }
    return true;
}

bool MemManager::getShmPos(MdbAddress& r_mdbAddr)
{
    if (m_addressMgr.getShmPos(r_mdbAddr) == false)
    {
        updateMgrInfo();
        if (m_addressMgr.getShmPos(r_mdbAddr) == false)
        {
            abort();
            throw Mdb_Exception(__FILE__, __LINE__, "根据物理地址取偏移量失败!");
            return false;
        }
    }
    return true;
}

void MemManager::getSpAddrList(const int& r_refetch)
{
    vector<SpaceAddress> t_spAddrList;
    SpaceAddress         t_spAddr;
    for (vector<SpaceInfo>::iterator t_spItr = m_spaceInfoList.begin();
            t_spItr != m_spaceInfoList.end(); t_spItr++)
    {
        t_spAddr.m_spaceCode = t_spItr->m_spaceCode;
        t_spAddr.m_size     = t_spItr->m_size;
        if (r_refetch != 0)
        {
            int t_shmId = shmget(t_spItr->m_shmKey, t_spItr->m_size, SHM_MODEFLAG);
            t_spItr->m_shmAddr = MdbShmAdrrMgr::getShmAddrByShmId(t_shmId);
        }
        t_spAddr.m_sAddr    = t_spItr->m_shmAddr;
        t_spAddr.m_eAddr    = t_spItr->m_shmAddr + (t_spItr->m_size - 1);
        t_spAddrList.push_back(t_spAddr);
    }
    m_addressMgr.initialize(t_spAddrList);
}
//创建各个表空间
bool MemManager::createSpace(const int& r_flag, vector<SpaceInfo> &r_spaceInfo)
{
    //2 根据配置信息创建索引和数据区表空间
    for (vector<SpaceInfo>::iterator t_spInfoItr = r_spaceInfo.begin();
            t_spInfoItr != r_spaceInfo.end(); t_spInfoItr++)
    {
        if (createSpace(*t_spInfoItr, r_flag) == false)
        {
#ifdef _DEBUG_
            cout << "createSpace(*t_spInfoItr) false!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
    }
    return true;
}
//attach各个表空间(索引和数据)
bool MemManager::attachSpace()
{
    bool t_bRet = true;
    if (m_tableMemMgr.attachAllTbSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_tableMemMgr.attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    if (m_indexMemMgr.attach() == false)
    {
#ifdef _DEBUG_
        cout << "m_indexMemMgr.attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    if (m_undoMemMgr.attach() == false)
    {
#ifdef _DEBUG_
        cout << "m_undoMemMgr.attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    return t_bRet;
}

//detach各个表空间(索引和数据)
bool MemManager::detachSpace()
{
    bool t_bRet = true;
    if (m_ctlMemMgr.detach() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.detach() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    if (m_tableMemMgr.detachAllTbSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_tableMemMgr.detach() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    if (m_indexMemMgr.detach() == false)
    {
#ifdef _DEBUG_
        cout << "m_indexMemMgr.detach() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    if (m_undoMemMgr.detach() == false)
    {
#ifdef _DEBUG_
        cout << "m_undoMemMgr.detach() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    m_spaceInfoList.clear();
    return t_bRet;
}
//初始化各个表空间0第一次创建，1非第一次
bool MemManager::initSpace(const int r_flag)
{
    //cout<<"m_pstatus->getpath()="<<m_pstatus->getpath()<<"\t"<<__FILE__<<__LINE__<<endl;
    bool t_bRet = true;
    //modified by gaojf 2012/5/8 14:39:09 增加文件的状态信息
    if (m_tableMemMgr.initSpaceInfo(r_flag, m_pstatus->getpath(), m_pstatus->m_filetype) == false)
    {
#ifdef _DEBUG_
        cout << "m_tableMemMgr.initSpaceInfo(" << r_flag << ") false!"
             << " " << __FILE__ << __LINE__ << endl;
#endif
    }
    if (m_indexMemMgr.initSpaceInfo(r_flag, m_pstatus->getpath()) == false)
    {
#ifdef _DEBUG_
        cout << "m_indexMemMgr.initSpaceInfo(" << r_flag << ") false!"
             << " " << __FILE__ << __LINE__ << endl;
#endif
    }
    if (m_undoMemMgr.initSpaceInfo(m_pMdbCtlInfo->m_tableMaxNum,
                                   RedoInfo::m_redosize,
                                   m_pMdbCtlInfo->m_undohashsize) == false)
    {
#ifdef _DEBUG_
        cout << "m_indexMemMgr.initSpaceInfo( ) false!"
             << " " << __FILE__ << __LINE__ << endl;
#endif
    }
    return t_bRet;
}
//获取SessionId
void  MemManager::getSessionId(int& sessionid, short int& semmark)
{
    m_ctlMemMgr.getSessionId(sessionid, semmark);
}
//释放SessionId
bool MemManager::realseSid(const int& r_sid)
{
    return m_ctlMemMgr.realseSid(r_sid);
}

bool MemManager::backupMdb()
{
    //将数据备份出去
    //0. 获取DDL锁
    lockddl();
    bool t_bRet = true;
    try
    {
        //1. 获取MDB状态
        getMdbStatus();
        T_SCN  t_scn;
        getscn(t_scn);
        m_pstatus->setvalue(1, (m_pstatus->m_filegroup + 1) % 2, t_scn);
        // add by chenm 2010-09-10 改成一个表空间 一个线程写出的模式
        vector<SpaceMgrBase*> tSpaceMgrBases;
        tSpaceMgrBases.clear();
        vector<ThreadParam*> tThreadParams;
        tThreadParams.clear();
        tSpaceMgrBases.push_back(&m_ctlMemMgr);
        tSpaceMgrBases.push_back(&m_indexMemMgr);
        m_tableMemMgr.getSpaceMgr(tSpaceMgrBases);
        int t_j, iThreadParamsSize;
        for (int i = 0; i < tSpaceMgrBases.size(); ++i)
        {
            ThreadParam* tThreadparam = new ThreadParam();
            tThreadParams.push_back(tThreadparam);
            tThreadparam->m_pSpaceMgr = tSpaceMgrBases[i];
            tThreadparam->m_pstatus   = m_pstatus;
            if (pthread_create(&(tThreadparam->m_threadId), NULL,
                               (void * (*)(void*))writeOneTBSpace,
                               tThreadparam) != 0)
            {
                for (t_j = 0; t_j < i; ++t_j)
                {
                    pthread_join(tThreadParams[t_j]->m_threadId, NULL);
                    delete tThreadParams[t_j];
                }
                throw Mdb_Exception(__FILE__, __LINE__, "Create Thread Error!");
            }
        }
        bool isErr = false;
        for (t_j = 0; t_j < tSpaceMgrBases.size(); ++t_j)
        {
            pthread_join(tThreadParams[t_j]->m_threadId, NULL);
            if (tThreadParams[t_j]->m_result == false)
            {
                isErr = true;
            }
        }
        tSpaceMgrBases.clear();
        for (iThreadParamsSize = 0; iThreadParamsSize < tThreadParams.size(); ++iThreadParamsSize)
        {
            delete tThreadParams[iThreadParamsSize];
        }
        tThreadParams.clear();
        if (isErr == true)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "shm dump to file error!");
        }
        // over 2010-09-10 20:48:23
        /*
        if(m_ctlMemMgr.dumpDataIntoFile(m_pstatus->getpath())==false)
        {
          #ifdef _DEBUG_
            cout<<"m_ctlMemMgr.dumpDataIntoFile() false!"
                <<" "<<__FILE__<<__LINE__<<endl;
          #endif
          t_bRet=false;
          throw Mdb_Exception(__FILE__, __LINE__, "导出控制区数据失败!");
        }

        if(m_tableMemMgr.dumpDataIntoFile(m_pstatus->getpath())==false)
        {
          #ifdef _DEBUG_
            cout<<"m_tableMemMgr.dumpDataIntoFile() false!"
                <<" "<<__FILE__<<__LINE__<<endl;
          #endif
          t_bRet=false;
          throw Mdb_Exception(__FILE__, __LINE__, "导出数据区数据失败!");
        }
        if(m_indexMemMgr.dumpDataIntoFile(m_pstatus->getpath())==false)
        {
          #ifdef _DEBUG_
            cout<<"m_indexMemMgr.dumpDataIntoFile() false!"
                <<" "<<__FILE__<<__LINE__<<endl;
          #endif
          t_bRet=false;
          throw Mdb_Exception(__FILE__, __LINE__, "导出索引区数据失败!");
        }*/
        //最后更新MDB状态
        if (m_statusMgr.writeStatus() == false)
        {
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "写MDB状态数据失败!");
        }
    }
    catch (Mdb_Exception& e)
    {
        unlockddl();
        throw e;
    }
    //解DDL锁
    unlockddl();
    return t_bRet;
}

void* MemManager::writeOneTBSpace(ThreadParam* arg)
{
    arg->m_result = arg->m_pSpaceMgr->dumpDataIntoFile(arg->m_pstatus->getpath());
    return NULL;
}


bool MemManager::memcopy(void* r_desAddr, const void* r_srcAddr, const size_t& r_size)
{
    memcpy(r_desAddr, r_srcAddr, r_size);
    return true;
}
bool MemManager::memcopy(void* r_desAddr, const ShmPosition& r_shmPos, const size_t& r_size)
{
    char* t_srcAddr;
    getPhAddr(r_shmPos, t_srcAddr);
    memcpy(r_desAddr, t_srcAddr, r_size);
    return true;
}

bool MemManager::registerSession(SessionInfo& r_sessionInfo)
{
    bool t_bRet = true;
    //int t_sessionId=-1;
    int t_sessionId;
    short int t_semmark;
    m_ctlMemMgr.getSessionId(t_sessionId, t_semmark);
    if ((t_sessionId < 0) || (t_semmark >= LOCKWAITS_SEM_MAX))
    {
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "ID资源占满!");
    }
    else
    {
        r_sessionInfo.m_sessionId = t_sessionId;
        r_sessionInfo.m_semmark = t_semmark;
        if (m_ctlMemMgr.registerSession(r_sessionInfo) == false)
        {
            m_ctlMemMgr.realseSid(t_sessionId);
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "插入SESSION失败!");
        }
    }
    return t_bRet;
}

bool MemManager::unRegisterSession(const SessionInfo& r_sessionInfo)
{
    bool t_bRet = true;
    /*
    if (m_ctlMemMgr.unRegisterSession(r_sessionInfo) == false)
    {
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "释放SESSION失败!");
    }
    if (m_ctlMemMgr.realseSid(r_sessionInfo.m_sessionId) == false)
    {
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "释放SESSIONID失败!");
    }
    */
    //重复尝试多次。不返回失败gaojf 2012/4/12 20:27:13
    int  t_i = 0, t_loop = 100;
    bool t_bret1 = false, t_bret2 = false;
    for (t_i = 0; t_i < t_loop; ++t_i)
    {
        if (m_ctlMemMgr.unRegisterSession(r_sessionInfo) == true)
        {
            t_bret1 = true;
            break;
        }
        usleep(5);
    }
    for (t_i = 0; t_i < t_loop; ++t_i)
    {
        if (m_ctlMemMgr.realseSid(r_sessionInfo.m_sessionId) == true)
        {
            t_bret2 = true;
            break;
        }
        usleep(5);
    }
    t_bRet = (t_bret1 && t_bRet);

    return t_bRet;
}

//add by gaojf 2009-3-2 4:05
bool MemManager::clearSessionInfos()
{
    return m_ctlMemMgr.reInitSessionInfos();
}
/**
 *getSessionInfos 获取所有Session信息.
 *@param   r_sessionInfoList   : 存放获取的信息
 *@return  true 成功,false 失败
 */
bool MemManager::getSessionInfos(vector<SessionInfo> &r_sessionInfoList)
{
    if (m_ctlMemMgr.getSessionInfos(r_sessionInfoList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "取SESSION列表信息失败!");
        return false;
    }
    return true;
}

bool MemManager::updateMgrInfo()
{
    if (m_spUpdateTime >= m_ctlMemMgr.getMdbGInfo()->m_spUpTime)
    {
        //不需要更新
        return true;
    }
    vector<SpaceInfo> t_spaceInfoList;///<表空间信息
    bool              t_flag;
    getSpaceInfoList(t_spaceInfoList);
    //找出增加的表空间信息
    for (vector<SpaceInfo>::iterator t_spItr = t_spaceInfoList.begin();
            t_spItr != t_spaceInfoList.end(); t_spItr++)
    {
        t_flag = true;
        for (vector<SpaceInfo>::iterator t_spItr2 = m_spaceInfoList.begin();
                t_spItr2 != m_spaceInfoList.end(); t_spItr2++)
        {
            if (*t_spItr == *t_spItr2)
            {
                t_flag = false;
                break;
            }
        }
        if (t_flag == true)
        {
            //modified by gaojf 2012/5/8 15:36:10
            if (t_spItr->m_type == TABLE_SPACE || t_spItr->m_type == TABLE_SPACE_NP)
            {
                if (m_tableMemMgr.attach_int(*t_spItr) == false)
                {
                    throw Mdb_Exception(__FILE__, __LINE__, "连接数据区表空间失败!");
                    return false;
                }
                m_spaceInfoList.push_back(*t_spItr);
            }
        }
    }
    getSpAddrList();
    return true;
}

//调试接口
bool MemManager::dumpSpaceInfo(ostream& r_os)
{
    m_ctlMemMgr.dumpSpaceInfo(r_os);
    m_tableMemMgr.dumpSpaceInfo(r_os);
    m_indexMemMgr.dumpSpaceInfo(r_os);
    m_undoMemMgr.dumpSpaceInfo(r_os);
    return true;
}


//根据表空间参数信息,创建表空间
//并将该信息填上
bool MemManager::createTbSpace(SpaceInfo& r_spaceInfo, const int& r_flag)
{
    //获取DDL锁
    lockddl();
    getMdbStatus();
    //1. 获取表空间代码
    if (r_flag == 0)
    {
        r_spaceInfo.m_spaceCode = m_ctlMemMgr.getNSpaceCode();
        r_spaceInfo.m_pageSize = m_pMdbCtlInfo->m_pageSize;
    }
    try
    {
        //2. 创建表空间
        switch (r_spaceInfo.m_type)
        {
            case TABLE_SPACE://数据区表空间
            case TABLE_SPACE_NP: //不需要持久化的数据区表空间 2012/5/7 20:28:26 gaojf
                {
                    TableSpaceMgr* t_pTbSpaceMgr = new TableSpaceMgr(r_spaceInfo);
                    if (t_pTbSpaceMgr->createTbSpace(r_flag) == false)
                    {
#ifdef _DEBUG_
                        cout << "t_pTbSpaceMgr->createTbSpace() false!" << endl;
#endif
                        throw Mdb_Exception(__FILE__, __LINE__, "创建数据表空间失败!");
                    }
                    if (t_pTbSpaceMgr->attach() == false)
                    {
#ifdef _DEBUG_
                        cout << "attach() !"
                             << " " << __FILE__ << __LINE__ << endl;
#endif
                        throw Mdb_Exception(__FILE__, __LINE__, "连接表空间失败!");
                    }
                    r_spaceInfo = *(t_pTbSpaceMgr->getSpaceInfo());
                    //创建式初始化该表空间
                    if (t_pTbSpaceMgr->initTbSpace(r_flag, m_pstatus->getpath()) == false)
                    {
#ifdef _DEBUG_
                        cout << "t_pTbSpaceMgr->initTbSpace(t_flag) false!" << endl;
#endif
                        throw Mdb_Exception(__FILE__, __LINE__, "创建式初始化该表空间失败!");
                    }
                    m_tableMemMgr.addSpaceMgr(t_pTbSpaceMgr);
                    m_spaceInfoList.push_back(r_spaceInfo);
                    // 生成该数据区对应的数据文件
                    t_pTbSpaceMgr->dumpDataIntoFile(m_pstatus->getpath());
                    break;
                }
            default:
#ifdef _DEBUG_
                cout << "表空间类型错误!spaceType=" << r_spaceInfo.m_type
                     << " " << __FILE__ << __LINE__ << endl;
#endif
                throw Mdb_Exception(__FILE__, __LINE__, "表空间类型错误!");
        };
        //3. 在控制区中加表空间信息
        if (m_ctlMemMgr.addSpaceInfo(r_spaceInfo) == false)
        {
#ifdef _DEBUG_
            cout << "m_ctlMemMgr.addSpaceInfo(r_spaceInfo) false!"
                 << __FILE__ << __LINE__ << endl;
#endif
            throw Mdb_Exception(__FILE__, __LINE__, "增加表空间信息到控制区失败!");
        }
    }
    catch (Mdb_Exception& e)
    {
        unlockddl();
        throw e;
    }
    unlockddl();
    return true;
}


/**
 *addGlobalParam 增加全局参数信息.
 *@param   r_gParam: 全局参数心想你
 *@return  true 成功,false 失败
 */
bool MemManager::addGlobalParam(const GlobalParam& r_gParam)
{
    T_SCN t_scn;
    getscn(t_scn);
    if (m_ctlMemMgr.addGlobalParam(r_gParam, t_scn) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "增加全局参数到控制区失败!");
        return false;
    }
    return true;
}
/**
 *getGlobalParam 获取全局参数信息.
 *@param   r_paramname: 参数名称
 *@param   r_gParam   : 存放获取的参数信息
 *@return  true 成功,false 失败
 */
bool MemManager::getGlobalParam(const char* r_paramname, GlobalParam& r_gParam)
{
    if (m_ctlMemMgr.getGlobalParam(r_paramname, r_gParam) == false)
    {
        //throw Mdb_Exception(__FILE__, __LINE__, "无对应的全局参数信息!ParamName=%s",r_paramname);
        return false;
    }
    return true;
}
/**
 *getGlobalParams 获取所有全局参数信息.
 *@param   r_gparamList   : 存放获取的参数信息
 *@return  true 成功,false 失败
 */
bool MemManager::getGlobalParams(vector<GlobalParam>& r_gparamList)
{
    if (m_ctlMemMgr.getGlobalParams(r_gparamList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "取所有全局参数信息!");
        return false;
    }
    return true;
}

bool MemManager::updateGlobalParam(const char* r_paramname, const char* r_value)
{
    T_SCN t_scn;
    getscn(t_scn);
    if (m_ctlMemMgr.updateGlobalParam(r_paramname, r_value, t_scn) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "无对应的全局参数信息!");
        return false;
    }
    return true;
}
bool MemManager::deleteGlobalParam(const char* r_paramname)
{
    T_SCN t_scn;
    getscn(t_scn);
    if (m_ctlMemMgr.deleteGlobalParam(r_paramname, t_scn) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "无对应的全局参数信息!");
        return false;
    }
    return true;
}
#ifdef _USEDSLOT_LIST_
//根据偏移量取Slot指针
bool MemManager::getSlotByShmPos(const ShmPosition& r_shmPos, UsedSlot* &r_pSlot)
{
    char* t_chAddr;
    try
    {
        getPhAddr(r_shmPos, t_chAddr);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        cout << "getPhAddr(" << r_shmPos << ",t_chAddr) false!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw e;
    }
    r_pSlot = (UsedSlot*)t_chAddr;
    return true;
}
#endif
bool MemManager::getPageInfo(const ShmPosition& r_pagePos, PageInfo *&r_pPage)
{
    char* t_chAddr;
    try
    {
        getPhAddr(r_pagePos, t_chAddr);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        cout << "getPhAddr(" << r_pagePos << ",t_chAddr) false!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw e;
    }
    r_pPage = (PageInfo*)t_chAddr;
    return true;
}

void MemManager::getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName)
{
    m_tableMemMgr.getTableSpaceUsedPercent(vUserdPercent, cTableSpaceName);
    m_indexMemMgr.getTableSpaceUsedPercent(vUserdPercent, cTableSpaceName);
    m_undoMemMgr.getTableSpaceUsedPercent(vUserdPercent, cTableSpaceName);
    return;
}

void MemManager::lockddl()
{
    m_pddlLock->mlatch_lock(m_ddlLatch);
}
// add by chenm 2010-09-09 13:02:55
bool MemManager::tryLockDDL()
{
    return m_pddlLock->mlatch_trylock(m_ddlLatch);
}
// over 2010-09-09 13:03:01
void MemManager::unlockddl()
{
    m_pddlLock->mlatch_unlock(m_ddlLatch);
}
MdbStatus* MemManager::getMdbStatus()
{
    m_statusMgr.initialize(m_mdbName, m_pMdbCtlInfo->m_lockpath);
    m_statusMgr.readStatus();
    m_pstatus = m_statusMgr.getStatus();
    return m_pstatus;
}
bool MemManager::writeStatus()
{
    return m_statusMgr.writeStatus();
}
DescPageInfos* MemManager::getdescinfo(const char& r_type, const size_t& r_offset)
{
    return m_ctlMemMgr.getdescinfo(r_type, r_offset);
}
size_t MemManager::getdescOffset(const char& r_type, const DescPageInfos* r_desc)
{
    return m_ctlMemMgr.getdescOffset(r_type, r_desc);
}
//根据Slot地址，取对应的页面信息和页面地址
bool MemManager::getPageBySlot(const ShmPosition& r_slot, ShmPosition& r_pagePos, PageInfo* &r_pPage)
{
    return m_tableMemMgr.getPageBySlot(r_slot, r_pagePos, r_pPage);
}

void MemManager::getlastckpscn(T_SCN& r_scn)
{
    return m_ctlMemMgr.getlastckpscn(r_scn);
}
void MemManager::setlastckpscn(const T_SCN& r_scn)
{
    return m_ctlMemMgr.setlastckpscn(r_scn);
}

/**
 *afterStartMdb 数据库启动后,清理scn和槽位信息等.
 * 失败 抛出异常
 */
void MemManager::afterStartMdb()
{
    //1. 控制区清理
    m_ctlMemMgr.afterStartMdb();
    //2. 数据区清理
    m_tableMemMgr.afterStartMdb();
    //3. 索引区清理
    m_indexMemMgr.afterStartMdb();
}

bool MemManager::addRedoConfig(const char* key, const size_t& value)
{
    GlobalParam gParam;
    if (!getGlobalParam(key, gParam))
    {
        strcpy(gParam.m_paramName, key);
        sprintf(gParam.m_value, "%ld", value);
        addGlobalParam(gParam);
    }
    else if (atoi(gParam.m_value) !=  value)
    {
        T_GPARAMVALUE mValue;
        sprintf(mValue, "%ld", value);
        updateGlobalParam(key, mValue);
    }
    return true;
}

bool MemManager::addRedoConfig(const char* key, const char* value)
{
    GlobalParam gParam;
    if (!getGlobalParam(key, gParam))
    {
        strcpy(gParam.m_paramName, key);
        strcpy(gParam.m_value, value);
        addGlobalParam(gParam);
    }
    else if (strcmp(gParam.m_value, value) != 0)
        updateGlobalParam(key, value);
    return true;
}



