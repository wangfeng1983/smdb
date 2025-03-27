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
 *creatMdb ���ݿⴴ��.
 *         ֻ�����������Ĵ���(�����ݿ�)
 *         ��������Ϣ�ݲ��������ļ���ʽ�ṩ
 *         �������ݾ�δ�����������ļ���ռ�
 *@return  true �����ɹ�,false ʧ��
 */
bool MemManager::creatMdb()
{
#ifdef _DEBUG_
    cout << "\t��ʼ��ȡ�����ļ�......" << __FILE__ << __LINE__ << endl;
#endif
    //1. ��ȡ�����ļ��е���Ϣ
    Mdb_Config  t_dbConfig;
    if (t_dbConfig.getConfigInfo(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "r_dbConfig.getConfigInfo(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "ȡ�����ݿ��Ӧ��������Ϣʧ��!");
    }
#ifdef _DEBUG_
    cout << "\t��ȡ�����ļ��ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ���������ļ�......" << __FILE__ << __LINE__ << endl;
#endif
    //2. ���������ļ�
    if (m_ctlFile.createFile(t_dbConfig) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlFile.createFile(t_dbConfig) failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "���������ļ�ʧ��!");
    }
    m_pMdbCtlInfo = m_ctlFile.getCtlInfoPt();
    getMdbStatus();
    m_pstatus->setvalue(-1, 0, T_SCN());
    m_pstatus->setpath(m_pMdbCtlInfo->m_datapath);
    writeStatus();
#ifdef _DEBUG_
    cout << "\t���������ļ��ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ������������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //3. ������������ռ�
    //3.1��ʼ��
    m_ctlMemMgr.initialize(*m_pMdbCtlInfo);
    int t_flag = 0; //��һ�δ���
    //3.2.������������ռ�
    if (m_ctlMemMgr.createControlSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.createControlSpace(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "������������ռ�ʧ��!");
    }
    //3.3.���ӿ�������ռ�
    if (m_ctlMemMgr.attach() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "���ӿ�������ռ�ʧ��!");
    }
    //3.4 ��ʼ����������ռ���Ϣ
    if (m_ctlMemMgr.initCtlDataInfo(t_flag, m_pstatus->getpath()) == false)
    {
#ifdef _DEBUG_
        cout << "initCtlDataInfo.attach(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "��ʼ����������ռ���Ϣʧ��!");
    }
#ifdef _DEBUG_
    cout << "\t������������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ����������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //3.5 ����������ռ�
    if (createSpace(t_flag, t_dbConfig.m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "createSpace() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "������ռ�ʧ��!");
    }
#ifdef _DEBUG_
    cout << "\t������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ����������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //4 ����������ռ�
    if (attachSpace() == false)
    {
#ifdef _DEBUG_
        cout << "attach() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "���ӱ�ռ�ʧ��!");
    }
#ifdef _DEBUG_
    cout << "\t����������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ��ʼ��������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //5. ��ʼ�����ݿ�����������
    if (initSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "initSpace(" << t_flag << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "����ʼ�����ݿ�����������ʧ��!");
    }
    //��ȡ���������������ı�ռ���Ϣ
    m_tableMemMgr.getSpaceInfoList(m_spaceInfoList);
    m_spaceInfoList.push_back(*(m_indexMemMgr.getSpaceInfo()));
    m_spaceInfoList.push_back(*(m_undoMemMgr.getSpaceInfo()));
    getSpAddrList(); //ȡ��ַ��Ϣ
#ifdef _DEBUG_
    cout << "\t��ʼ��������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t����ռ����ݱ��ݳ�ȥ......" << __FILE__ << __LINE__ << endl;
#endif
    m_openFlag = true;
    //����ʱע��
    //7. ֹͣ���ݿ�
    stopMdb();
    return true;
}

/**
 *dropMdb ���ݿ�ɾ��.
 *         ��������ڴ����ݿ���Ϣ
 *         ��������Ϣ�ݲ��������ļ���ʽ�ṩ
 *@return  true ɾ���ɹ�,false ʧ��
 */
bool MemManager::dropMdb()
{
    //coding later
    return true;
}

/**
 *startMdb ���ݿ�����.
 *         ���ݿ�����ֹͣ���쳣�ָ��������
 *         ��������Ϣ�ݲ��������ļ���ʽ�ṩ
 *@return  true �����ɹ�,false ʧ��
 */
bool MemManager::startMdb()
{
    m_spUpdateTime = 0;
    //1. ��ȡ�����ļ��е���Ϣ add by gaojf
    Mdb_Config  t_dbConfig;
    if (t_dbConfig.getConfigInfo(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "r_dbConfig.getConfigInfo(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "ȡ�����ݿ��Ӧ��������Ϣʧ��!");
        return false;
    }
    if (m_ctlFile.initialize(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlFile.initialize(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "�����ļ���ʼ��ʧ��!");
        return false;
    }
    m_pMdbCtlInfo = m_ctlFile.getCtlInfoPt();
    getMdbStatus();
#ifdef _DEBUG_
    m_pstatus->dump();
    cout << "\t��ʼ������������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //3. ������������ռ�
    //3.1��ʼ��
    m_ctlMemMgr.initialize(*m_pMdbCtlInfo);
    int t_flag = 1; //�ǵ�һ�δ���
    //3.2.������������ռ�
    if (m_ctlMemMgr.createControlSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.createControlSpace(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "������������ռ�ʧ��!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t��ʼ���ӿ�������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //3.3.���ӿ�������ռ�
    if (m_ctlMemMgr.attach() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "���ӿ�������ռ�ʧ��!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t��ʼ��ʼ����������ռ���Ϣ......" << __FILE__ << __LINE__ << endl;
#endif
    //3.4 ��ʼ����������ռ���Ϣ
    if (m_ctlMemMgr.initCtlDataInfo(t_flag, m_pstatus->getpath()) == false)
    {
#ifdef _DEBUG_
        cout << "initCtlDataInfo.attach(" << t_flag << ") false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "��ʼ����������ռ���Ϣʧ��!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t������������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ����������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //�ӿ�������ռ���ȡ������ռ���Ϣ
    if (getSpaceInfoList(m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "getSpaceInfoList failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "�ӿ�������ռ���ȡ������ռ���Ϣʧ��!");
        return false;
    }
    //Ϊ��֧�������ļ�����,��������shmKey add by gaojf 2010/11/23 11:05:47 begin
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
                throw Mdb_Exception(__FILE__, __LINE__, "�������ݶ�Ӧ��ռ��shmKeyʧ��!");
                return false;
            }
        }
        
        //��������ռ��������ļ��������ڵ�Ϊ׼ 2012/9/18 7:08:50 gaojf
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
                        throw Mdb_Exception(__FILE__, __LINE__, "�������ݶ�Ӧ��ռ��shmKeyʧ��!");
                        return false;
                    }
                    break;
                }
            }
        }
    }
    //end gaojf 2010/11/23 11:05:52
    //3.5 ����������ռ�
    if (createSpace(t_flag, m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "createSpace() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "������ռ�ʧ��!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ����������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //4 ����������ռ�
    if (attachSpace() == false)
    {
#ifdef _DEBUG_
        cout << "attach() failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "���ӱ�ռ�ʧ��!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t����������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\t��ʼ��ʼ��������ռ�......" << __FILE__ << __LINE__ << endl;
#endif
    //5. ��ʼ�����ݿ�����������
    if (initSpace(t_flag) == false)
    {
#ifdef _DEBUG_
        cout << "initSpace(" << t_flag << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "��ʼ�����ݿ�����������ʧ��!");
        return false;
    }
#ifdef _DEBUG_
    cout << "\t��ʼ��������ռ�ɹ�......" << __FILE__ << __LINE__ << endl;
    cout << "\tStartMdb OK!" << endl;
#endif
    getSpAddrList(1);
    m_openFlag = true;
    //���ݱ����������崴����Ӧ��Undo��
    vector<TableDesc> t_tabledesclist;
    getTableDescList(t_tabledesclist);
    m_undoMemMgr.createUndoDescs(t_tabledesclist);
    //����redo��־������Ϣ
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
    //���������ļ�����backup������,
    //��Ҫ�������ı�DATA_TABLE_NP������HASH_TREE_NP��ʼ����
    //��Ҫ������������HASH_INDEX_NP ��Ӧ����������Ϣ����ʼ����
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
                //1.��ʼ������ʼ��ֹҳ����Ϣ
                for (t_i = 0; t_i < MAX_INDEX_NUM; ++t_i)
                {
                    t_pTableDesc->m_pageInfo[t_i].initPos();
                }
                t_pIndexList.clear();
                getIndexListByTable(*t_tbdescitr, t_pIndexList);
                for (vector<IndexDesc*>::iterator t_pidxitr = t_pIndexList.begin();
                        t_pidxitr != t_pIndexList.end(); ++t_pidxitr)
                {
                    //2.��ʼ��������ʼ��ֹҳ����Ϣ
                    for (t_i = 0; t_i < MAX_INDEX_NUM; ++t_i)
                    {
                        (*t_pidxitr)->m_pageInfo[t_i].initPos();
                    }
                    //3.��ʼ����������Ϣ
                    m_indexMemMgr.initHashSeg((*t_pidxitr)->m_header, t_scn);
                }
            }
        }
    }
    //add by gaojf 2012/5/8 14:49:41 end
    return true;
}

/**
 *stopMdb ���ݿ�ֹͣ.
 *         ��������Ϣ�ݲ��������ļ���ʽ�ṩ
 *@return  true ֹͣ�ɹ�,false ʧ��
 */
bool MemManager::stopMdb()
{
    bool t_bRet = true;
    if (m_openFlag == false)
    {
        //����Ѿ��Ͽ���������
        openMdb();
    }
    else
    {
        updateMgrInfo();
    }
    backupMdb();
    closeMdb();
    //���б�ռ���Ϣ���ѻ��
    //2. ɾ����ռ䣨�ڴ沿�֣�
    if (m_tableMemMgr.deleteAllTbSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_tableMemMgr.deleteAllTbSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "ɾ����������ռ�ʧ��!");
    }
    //3. ɾ�������ռ�
    if (m_indexMemMgr.deleteIdxSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_indexMemMgr.deleteIdxSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "ɾ�������ռ�ʧ��!");
    }
    //4. ɾ��Undo��ռ�
    m_undoMemMgr.deleteUndoSpace();
    //5. ɾ����������ռ�
    if (m_ctlMemMgr.deleteControlSpace() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.deleteControlSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "ɾ����������ռ�!");
    }
    //6. ɾ��latch�������ڴ� by chenm 2010-11-03 11:37:20
    if (MDBLatchShmMgr::deleteLatchShm(m_pddlLock->m_latchshminfo) == false)
    {
#ifdef _DEBUG_
        cout << "MDBLatchShmMgr::deleteLatchShm(m_pddlLock->m_latchshminfo) false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "ɾ��latch����ռ�ʧ��!");
    }
    MDBLatchMgr::unregister(m_mdbName, pthread_self());
    return t_bRet;
}
//�����ݿ�����
bool MemManager::openMdb()
{
    if (m_ctlFile.initialize(m_mdbName) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlFile.initialize(" << m_mdbName << ") failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "�����ļ���ʼ��ʧ��!");
        return false;
    }
    m_pMdbCtlInfo = m_ctlFile.getCtlInfoPt();
    getMdbStatus();
    //3 ��ʼ��������������
    m_ctlMemMgr.initialize(*m_pMdbCtlInfo);
    //3.3.���ӷ�ʽ��ʼ����������ռ�
    if (m_ctlMemMgr.attach_init() == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.attach_init() false!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "���ӿ�������ռ�ʧ��!");
        return false;
    }
    //�ӿ�������ռ���ȡ������ռ���Ϣ
    if (getSpaceInfoList(m_spaceInfoList) == false)
    {
#ifdef _DEBUG_
        cout << "getSpaceInfoList failed!" << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "�ӿ�������ռ���ȡ������ռ���Ϣʧ��!");
        return false;
    }
    //2 ����������Ϣ������������������ռ�
    for (vector<SpaceInfo>::iterator t_spInfoItr = m_spaceInfoList.begin();
            t_spInfoItr != m_spaceInfoList.end(); t_spInfoItr++)
    {
        switch (t_spInfoItr->m_type)
        {
            case TABLE_SPACE://��������ռ�
            case TABLE_SPACE_NP: //����Ҫ�־û�����������ռ� 2012/5/7 20:28:26 gaojf
                //3. ��ʼ����ռ�
                if (m_tableMemMgr.attach_int(*t_spInfoItr) == false)
                {
#ifdef _DEBUG_
                    cout << "m_tableMemMgr.attach_int(*t_spInfoItr) false!" << __FILE__ << __LINE__ << endl;
#endif
                    throw Mdb_Exception(__FILE__, __LINE__, "��ʼ����������ռ�ʧ��!");
                    return false;
                }
                break;
            case INDEX_SPACE://��������ռ�
                //3. ��ʼ����ռ�
                if (m_indexMemMgr.attach_init(*t_spInfoItr) == false)
                {
#ifdef _DEBUG_
                    cout << "m_indexMemMgr.attach_int(*t_spInfoItr) false!" << __FILE__ << __LINE__ << endl;
#endif
                    throw Mdb_Exception(__FILE__, __LINE__, "��ʼ����������ռ�ʧ��!");
                    return false;
                }
                break;
            case UNDO_SPACE: //Undo��ռ�
                //4.��ʼ����ռ�
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
                cout << "��ռ����ʹ���!spaceType=" << t_spInfoItr->m_type
                     << " " << __FILE__ << __LINE__ << endl;
#endif
                throw Mdb_Exception(__FILE__, __LINE__, "��ռ�����ʧ��!");
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
//�Ͽ����ݿ�����
bool MemManager::closeMdb()
{
    bool t_bRet = true;
    if (m_openFlag == true)
    {
        //1.detach ����ռ�
        if (detachSpace() == false)
        {
#ifdef _DEBUG_
            cout << "detachSpace()() false!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "detach��ռ�ʧ��!");
        }
        m_openFlag = false;
    }
    return t_bRet;
}


//���ݱ�ռ������Ϣ,������ռ�
//��������Ϣ����
bool MemManager::createSpace(SpaceInfo& r_spaceInfo, const int& r_flag)
{
    bool t_bret = true;
    //1. ��ȡ��ռ����
    if (r_flag == 0) //��һ����Ҫ����SpaceCode�ʹ�С
    {
        r_spaceInfo.m_spaceCode = m_ctlMemMgr.getNSpaceCode();
        //r_spaceInfo.m_pageSize =m_pMdbCtlInfo->m_pageSize;
    }
    //2. ������ռ�
    switch (r_spaceInfo.m_type)
    {
        case TABLE_SPACE://��������ռ�
        case TABLE_SPACE_NP: //����Ҫ�־û�����������ռ� 2012/5/7 20:28:26 gaojf
            //3. ��ʼ����ռ�
            if (m_tableMemMgr.createTbSpace(r_spaceInfo, r_flag) == false)
            {
#ifdef _DEBUG_
                cout << "m_tableMemMgr.createTbSpace(r_spaceInfo) false!" << __FILE__ << __LINE__ << endl;
#endif
                t_bret = false;
            }
            break;
        case INDEX_SPACE://��������ռ�
            m_indexMemMgr.initialize(r_spaceInfo);
            //3. ��ʼ����ռ�
            if (m_indexMemMgr.createIdxSpace(r_spaceInfo, r_flag) == false)
            {
#ifdef _DEBUG_
                cout << "m_indexMemMgr.createIdxSpace() false!" << __FILE__ << __LINE__ << endl;
#endif
                t_bret = false;
            }
            break;
        case UNDO_SPACE: //Undo��ռ�
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
            cout << "��ռ����ʹ���!spaceType=" << r_spaceInfo.m_type
                 << " " << __FILE__ << __LINE__ << endl;
#endif
            t_bret = false;
    };
    if (r_flag == 0) //��һ����Ҫ����ռ���Ϣ�����������ռ�
    {
        //4. �ڿ������мӱ�ռ���Ϣ
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
        throw Mdb_Exception(__FILE__, __LINE__, "�ñ�ռ䲻����!");
        return false;
    }
    return true;
}

bool MemManager::getSpaceInfoList(vector<SpaceInfo> &r_spaceInfoList)
{
    time_t t_spTime = m_ctlMemMgr.getMdbGInfo()->m_spUpTime;
    if (m_ctlMemMgr.getSpaceInfos(r_spaceInfoList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "ȡ��ռ��б���Ϣʧ��!");
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
            throw Mdb_Exception(__FILE__, __LINE__, "����ƫ����ȡ�����ַʧ��!");
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
            throw Mdb_Exception(__FILE__, __LINE__, "����ƫ����ȡ�����ַʧ��!");
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
            throw Mdb_Exception(__FILE__, __LINE__, "���������ַȡƫ����ʧ��!");
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
//����������ռ�
bool MemManager::createSpace(const int& r_flag, vector<SpaceInfo> &r_spaceInfo)
{
    //2 ����������Ϣ������������������ռ�
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
//attach������ռ�(����������)
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

//detach������ռ�(����������)
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
//��ʼ��������ռ�0��һ�δ�����1�ǵ�һ��
bool MemManager::initSpace(const int r_flag)
{
    //cout<<"m_pstatus->getpath()="<<m_pstatus->getpath()<<"\t"<<__FILE__<<__LINE__<<endl;
    bool t_bRet = true;
    //modified by gaojf 2012/5/8 14:39:09 �����ļ���״̬��Ϣ
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
//��ȡSessionId
void  MemManager::getSessionId(int& sessionid, short int& semmark)
{
    m_ctlMemMgr.getSessionId(sessionid, semmark);
}
//�ͷ�SessionId
bool MemManager::realseSid(const int& r_sid)
{
    return m_ctlMemMgr.realseSid(r_sid);
}

bool MemManager::backupMdb()
{
    //�����ݱ��ݳ�ȥ
    //0. ��ȡDDL��
    lockddl();
    bool t_bRet = true;
    try
    {
        //1. ��ȡMDB״̬
        getMdbStatus();
        T_SCN  t_scn;
        getscn(t_scn);
        m_pstatus->setvalue(1, (m_pstatus->m_filegroup + 1) % 2, t_scn);
        // add by chenm 2010-09-10 �ĳ�һ����ռ� һ���߳�д����ģʽ
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
          throw Mdb_Exception(__FILE__, __LINE__, "��������������ʧ��!");
        }

        if(m_tableMemMgr.dumpDataIntoFile(m_pstatus->getpath())==false)
        {
          #ifdef _DEBUG_
            cout<<"m_tableMemMgr.dumpDataIntoFile() false!"
                <<" "<<__FILE__<<__LINE__<<endl;
          #endif
          t_bRet=false;
          throw Mdb_Exception(__FILE__, __LINE__, "��������������ʧ��!");
        }
        if(m_indexMemMgr.dumpDataIntoFile(m_pstatus->getpath())==false)
        {
          #ifdef _DEBUG_
            cout<<"m_indexMemMgr.dumpDataIntoFile() false!"
                <<" "<<__FILE__<<__LINE__<<endl;
          #endif
          t_bRet=false;
          throw Mdb_Exception(__FILE__, __LINE__, "��������������ʧ��!");
        }*/
        //������MDB״̬
        if (m_statusMgr.writeStatus() == false)
        {
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "дMDB״̬����ʧ��!");
        }
    }
    catch (Mdb_Exception& e)
    {
        unlockddl();
        throw e;
    }
    //��DDL��
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
        throw Mdb_Exception(__FILE__, __LINE__, "ID��Դռ��!");
    }
    else
    {
        r_sessionInfo.m_sessionId = t_sessionId;
        r_sessionInfo.m_semmark = t_semmark;
        if (m_ctlMemMgr.registerSession(r_sessionInfo) == false)
        {
            m_ctlMemMgr.realseSid(t_sessionId);
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "����SESSIONʧ��!");
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
        throw Mdb_Exception(__FILE__, __LINE__, "�ͷ�SESSIONʧ��!");
    }
    if (m_ctlMemMgr.realseSid(r_sessionInfo.m_sessionId) == false)
    {
        t_bRet = false;
        throw Mdb_Exception(__FILE__, __LINE__, "�ͷ�SESSIONIDʧ��!");
    }
    */
    //�ظ����Զ�Ρ�������ʧ��gaojf 2012/4/12 20:27:13
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
 *getSessionInfos ��ȡ����Session��Ϣ.
 *@param   r_sessionInfoList   : ��Ż�ȡ����Ϣ
 *@return  true �ɹ�,false ʧ��
 */
bool MemManager::getSessionInfos(vector<SessionInfo> &r_sessionInfoList)
{
    if (m_ctlMemMgr.getSessionInfos(r_sessionInfoList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "ȡSESSION�б���Ϣʧ��!");
        return false;
    }
    return true;
}

bool MemManager::updateMgrInfo()
{
    if (m_spUpdateTime >= m_ctlMemMgr.getMdbGInfo()->m_spUpTime)
    {
        //����Ҫ����
        return true;
    }
    vector<SpaceInfo> t_spaceInfoList;///<��ռ���Ϣ
    bool              t_flag;
    getSpaceInfoList(t_spaceInfoList);
    //�ҳ����ӵı�ռ���Ϣ
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
                    throw Mdb_Exception(__FILE__, __LINE__, "������������ռ�ʧ��!");
                    return false;
                }
                m_spaceInfoList.push_back(*t_spItr);
            }
        }
    }
    getSpAddrList();
    return true;
}

//���Խӿ�
bool MemManager::dumpSpaceInfo(ostream& r_os)
{
    m_ctlMemMgr.dumpSpaceInfo(r_os);
    m_tableMemMgr.dumpSpaceInfo(r_os);
    m_indexMemMgr.dumpSpaceInfo(r_os);
    m_undoMemMgr.dumpSpaceInfo(r_os);
    return true;
}


//���ݱ�ռ������Ϣ,������ռ�
//��������Ϣ����
bool MemManager::createTbSpace(SpaceInfo& r_spaceInfo, const int& r_flag)
{
    //��ȡDDL��
    lockddl();
    getMdbStatus();
    //1. ��ȡ��ռ����
    if (r_flag == 0)
    {
        r_spaceInfo.m_spaceCode = m_ctlMemMgr.getNSpaceCode();
        r_spaceInfo.m_pageSize = m_pMdbCtlInfo->m_pageSize;
    }
    try
    {
        //2. ������ռ�
        switch (r_spaceInfo.m_type)
        {
            case TABLE_SPACE://��������ռ�
            case TABLE_SPACE_NP: //����Ҫ�־û�����������ռ� 2012/5/7 20:28:26 gaojf
                {
                    TableSpaceMgr* t_pTbSpaceMgr = new TableSpaceMgr(r_spaceInfo);
                    if (t_pTbSpaceMgr->createTbSpace(r_flag) == false)
                    {
#ifdef _DEBUG_
                        cout << "t_pTbSpaceMgr->createTbSpace() false!" << endl;
#endif
                        throw Mdb_Exception(__FILE__, __LINE__, "�������ݱ�ռ�ʧ��!");
                    }
                    if (t_pTbSpaceMgr->attach() == false)
                    {
#ifdef _DEBUG_
                        cout << "attach() !"
                             << " " << __FILE__ << __LINE__ << endl;
#endif
                        throw Mdb_Exception(__FILE__, __LINE__, "���ӱ�ռ�ʧ��!");
                    }
                    r_spaceInfo = *(t_pTbSpaceMgr->getSpaceInfo());
                    //����ʽ��ʼ���ñ�ռ�
                    if (t_pTbSpaceMgr->initTbSpace(r_flag, m_pstatus->getpath()) == false)
                    {
#ifdef _DEBUG_
                        cout << "t_pTbSpaceMgr->initTbSpace(t_flag) false!" << endl;
#endif
                        throw Mdb_Exception(__FILE__, __LINE__, "����ʽ��ʼ���ñ�ռ�ʧ��!");
                    }
                    m_tableMemMgr.addSpaceMgr(t_pTbSpaceMgr);
                    m_spaceInfoList.push_back(r_spaceInfo);
                    // ���ɸ���������Ӧ�������ļ�
                    t_pTbSpaceMgr->dumpDataIntoFile(m_pstatus->getpath());
                    break;
                }
            default:
#ifdef _DEBUG_
                cout << "��ռ����ʹ���!spaceType=" << r_spaceInfo.m_type
                     << " " << __FILE__ << __LINE__ << endl;
#endif
                throw Mdb_Exception(__FILE__, __LINE__, "��ռ����ʹ���!");
        };
        //3. �ڿ������мӱ�ռ���Ϣ
        if (m_ctlMemMgr.addSpaceInfo(r_spaceInfo) == false)
        {
#ifdef _DEBUG_
            cout << "m_ctlMemMgr.addSpaceInfo(r_spaceInfo) false!"
                 << __FILE__ << __LINE__ << endl;
#endif
            throw Mdb_Exception(__FILE__, __LINE__, "���ӱ�ռ���Ϣ��������ʧ��!");
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
 *addGlobalParam ����ȫ�ֲ�����Ϣ.
 *@param   r_gParam: ȫ�ֲ���������
 *@return  true �ɹ�,false ʧ��
 */
bool MemManager::addGlobalParam(const GlobalParam& r_gParam)
{
    T_SCN t_scn;
    getscn(t_scn);
    if (m_ctlMemMgr.addGlobalParam(r_gParam, t_scn) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "����ȫ�ֲ�����������ʧ��!");
        return false;
    }
    return true;
}
/**
 *getGlobalParam ��ȡȫ�ֲ�����Ϣ.
 *@param   r_paramname: ��������
 *@param   r_gParam   : ��Ż�ȡ�Ĳ�����Ϣ
 *@return  true �ɹ�,false ʧ��
 */
bool MemManager::getGlobalParam(const char* r_paramname, GlobalParam& r_gParam)
{
    if (m_ctlMemMgr.getGlobalParam(r_paramname, r_gParam) == false)
    {
        //throw Mdb_Exception(__FILE__, __LINE__, "�޶�Ӧ��ȫ�ֲ�����Ϣ!ParamName=%s",r_paramname);
        return false;
    }
    return true;
}
/**
 *getGlobalParams ��ȡ����ȫ�ֲ�����Ϣ.
 *@param   r_gparamList   : ��Ż�ȡ�Ĳ�����Ϣ
 *@return  true �ɹ�,false ʧ��
 */
bool MemManager::getGlobalParams(vector<GlobalParam>& r_gparamList)
{
    if (m_ctlMemMgr.getGlobalParams(r_gparamList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "ȡ����ȫ�ֲ�����Ϣ!");
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
        throw Mdb_Exception(__FILE__, __LINE__, "�޶�Ӧ��ȫ�ֲ�����Ϣ!");
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
        throw Mdb_Exception(__FILE__, __LINE__, "�޶�Ӧ��ȫ�ֲ�����Ϣ!");
        return false;
    }
    return true;
}
#ifdef _USEDSLOT_LIST_
//����ƫ����ȡSlotָ��
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
//����Slot��ַ��ȡ��Ӧ��ҳ����Ϣ��ҳ���ַ
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
 *afterStartMdb ���ݿ�������,����scn�Ͳ�λ��Ϣ��.
 * ʧ�� �׳��쳣
 */
void MemManager::afterStartMdb()
{
    //1. ����������
    m_ctlMemMgr.afterStartMdb();
    //2. ����������
    m_tableMemMgr.afterStartMdb();
    //3. ����������
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



