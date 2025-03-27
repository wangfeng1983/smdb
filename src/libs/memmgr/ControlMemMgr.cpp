#include "ControlMemMgr.h"

ControlMemMgr::ControlMemMgr()
{
    m_pCtlInfo = NULL;
}
ControlMemMgr::~ControlMemMgr()
{
    m_pCtlInfo = NULL;
}
void ControlMemMgr::initialize(MdbCtlInfo& r_ctlInfo)
{
    m_pCtlInfo = &r_ctlInfo;
    SpaceInfo  t_spaceInfo;
    strcpy(t_spaceInfo.m_spaceName, r_ctlInfo.m_ctlSpaceName);
    strcpy(t_spaceInfo.m_dbName, r_ctlInfo.m_dbName);
    sprintf(t_spaceInfo.m_fileName, "%s.data", r_ctlInfo.m_ctlSpaceName);
    t_spaceInfo.m_size = r_ctlInfo.m_size;
    t_spaceInfo.m_pageSize = r_ctlInfo.m_pageSize;
    t_spaceInfo.m_type = CONTROL_SPACE;
    t_spaceInfo.m_spaceCode = 1; //���������1
    SpaceMgrBase::initialize(t_spaceInfo);
}
/**
 *createControlSpace ������������ռ�.
 *@param   r_flag : 0 ��һ�δ���,1 ���ļ�������Ϣ
 *@return  true �����ɹ�,false ʧ��
 */
bool ControlMemMgr::createControlSpace(const int& r_flag)
{
    if (r_flag == 0) //��һ�δ���
    {
        if (checkSpaceSize() == false)
        {
            //���д�С����
#ifdef _DEBUG_
            cout << "���д�С����!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
    }
    if (createSpace(m_spaceHeader) == false)
    {
#ifdef _DEBUG_
        cout << "createSpace false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
bool ControlMemMgr::initCtlDataInfo(const int& r_flag, const char* r_path)
{
    //////////////////////////////////////////////////////
    //��ʼ����Ϣ,����ļ��е���
    if (r_flag == 0) //��һ�δ���
    {
        return firstCreator();
    }
    else //if(r_flag==1)
    {
        if (loadDataFromFile(r_path) == false)
        {
#ifdef _DEBUG_
            cout << "loadDataFromFile() false!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
        if (initAfterAttach() == false)
        {
#ifdef _DEBUG_
            cout << "loadDataFromFile() false!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
        //SESSION��Ϣ
        int t_flag = 0;
        m_sessionInfoMgr.clear(t_flag);
        return true;
    }
}

/**
 *deleteControlSpace ɾ����������ռ�.
 *@return  true �����ɹ�,false ʧ��
 */
bool ControlMemMgr::deleteControlSpace()
{
    return deleteSpace(m_spaceHeader);
}

bool ControlMemMgr::firstCreator()
{
    size_t  t_offSet = 0;
    //1. ��ʼ����������ռ�ͷm_spaceHeader��Ϣ
    memcpy(m_pSpHeader, &m_spaceHeader, sizeof(SpaceInfo));
    t_offSet += sizeof(SpaceInfo);
    //2. ��ʼ��ȫ����Ϣ
    m_pGobalInfos = (MDbGlobalInfo*)(m_spaceHeader.m_shmAddr + t_offSet);
    m_pGobalInfos->init();
    t_offSet += sizeof(MDbGlobalInfo);
    //3. ��ռ���Ϣ��ʼ��
    if (m_spInfoMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_pCtlInfo->m_gparamMaxSpNum) == false)
    {
#ifdef _DEBUG_
        cout << "m_spInfoMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_spInfoMgr.getSize();
    //4. ��ʼ��ȫ�ֲ�����Ϣ
    if (m_gParamMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_pCtlInfo->m_gparamMaxNum) == false)
    {
#ifdef _DEBUG_
        cout << "m_gParamMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_gParamMgr.getSize();
    //5.��ʼ������������Ϣ
    if (m_tbDescMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_pCtlInfo->m_tableMaxNum) == false)
    {
#ifdef _DEBUG_
        cout << "m_tbDescMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_tbDescMgr.getSize();
    //6.��ʼ��������������Ϣ
    if (m_idxDescMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_pCtlInfo->m_indexMaxNum) == false)
    {
#ifdef _DEBUG_
        cout << "m_idxDescMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_idxDescMgr.getSize();
    //7.��ʼ��Session��Ϣ�б�
    if (m_sessionInfoMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_pCtlInfo->m_sessionMaxNum) == false)
    {
#ifdef _DEBUG_
        cout << "m_sessionInfoMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_sessionInfoMgr.getSize();
    //������ñ����ռ���Ϣ
    m_pGobalInfos->setReserve(t_offSet);
    return true;
}


//У���С�Ƿ��㹻
bool ControlMemMgr::checkSpaceSize()
{
    size_t  t_needSize = 0;
    //1. ��������Ŀռ��С
    t_needSize += sizeof(SpaceInfo); //��ռ�ͷ
    t_needSize += sizeof(MDbGlobalInfo); //ȫ����Ϣ
    //��ռ���Ϣ
    t_needSize += sizeof(NodeListInfo) + m_pCtlInfo->m_gparamMaxSpNum * sizeof(NodeTmpt<SpaceInfo>);
    //ȫ�ֲ�����Ϣ
    t_needSize += sizeof(NodeListInfo) + m_pCtlInfo->m_gparamMaxNum * sizeof(NodeTmpt<GlobalParam>);
    //����������Ϣ
    t_needSize += sizeof(NodeListInfo) + m_pCtlInfo->m_tableMaxNum * sizeof(NodeTmpt<TableDesc>);
    //������������Ϣ
    t_needSize += sizeof(NodeListInfo) + m_pCtlInfo->m_indexMaxNum * sizeof(NodeTmpt<IndexDesc>);
    //SESSION��Ϣ
    t_needSize += sizeof(NodeListInfo) + m_pCtlInfo->m_sessionMaxNum * sizeof(NodeTmpt<SessionInfo>);

    // ÿ�� ListManager ʹ�õĿռ������Ҫ��������ռ�õĲ���: sizeof(size_t) * N, --laism, 2012-05-23
    t_needSize += sizeof(size_t) * m_pCtlInfo->m_gparamMaxSpNum;
    t_needSize += sizeof(size_t) * m_pCtlInfo->m_gparamMaxNum;
    t_needSize += sizeof(size_t) * m_pCtlInfo->m_tableMaxNum;
    t_needSize += sizeof(size_t) * m_pCtlInfo->m_indexMaxNum;
    t_needSize += sizeof(size_t) * m_pCtlInfo->m_sessionMaxNum;

    //2. �Ƚ�
    if (t_needSize > m_pCtlInfo->m_size)
    {
#ifdef _DEBUG_
        cout << "���������СΪ��" << t_needSize
             << " Ŀǰ���õĿռ�Ϊ:" << m_pCtlInfo->m_size
             << " �ռ��С����!" << endl;
#endif
        return false;
    }
#ifdef _DEBUG_
    cout << "���������СΪ��" << t_needSize
         << " Ŀǰ���õĿռ�Ϊ:" << m_pCtlInfo->m_size
         << " !" << endl;
#endif
    return true;
}

unsigned int ControlMemMgr::getNSpaceCode()
{
    //�ò���Я��д����
    unsigned int t_spaceCode;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_spaceCode = -1;
        return t_spaceCode;
    }
    t_spaceCode = m_pGobalInfos->getNextSpCode();
    SpaceMgrBase::unlock();
    return t_spaceCode;
}

bool ControlMemMgr::addSpaceInfo(const SpaceInfo& r_spaceInfo)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    if (m_spInfoMgr.addElement(r_spaceInfo) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    updateSpTime();
    return t_bRet;
}
bool ControlMemMgr::getDSpaceInfo(const char* r_spaceName, SpaceInfo& r_spaceInfo)
{
    bool t_bRet = true;
    SpaceInfo t_spaceInfo;
    strcpy(t_spaceInfo.m_spaceName, r_spaceName);
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    t_bRet = m_spInfoMgr.getElement(t_spaceInfo, r_spaceInfo);
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::getSpaceInfos(vector<SpaceInfo> &r_spaceInfo)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    if (m_spInfoMgr.getElements(r_spaceInfo) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
//add by gaojf 2009-12-10 13:07:14 ���±�ռ���ϢshmKey begin
bool ControlMemMgr::updateSpaceInfo(const SpaceInfo& r_spaceInfo)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_spInfoMgr.updateElement(r_spaceInfo) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    updateSpTime();
    return t_bRet;
}
//end gaojf 2009-12-10 13:08:48
//��ȡSessionId
void  ControlMemMgr::getSessionId(int& sessionid, short int& semmark)
{
    if (m_pGobalInfos == NULL) return ;
    //int t_iret=-1;
    /*
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        //return t_iret;
        return;
    }
    */
    //��ֹһ��lockʧ�ܵ��´��� gaojf 2012/4/13 15:07:52
    int  t_i = 0, t_loop = 100;
    for (t_i = 0; t_i < t_loop; ++t_i)
    {
        try
        {
            SpaceMgrBase::lock();
        }
        catch (Mdb_Exception& e)
        {
            if (t_i == t_loop - 1)
            {
                return;
            }
            usleep(5);
            continue;
        }
        break;
    }
    m_pGobalInfos->getSessionId(sessionid, semmark);
    SpaceMgrBase::unlock();
    //return t_iret;
}
//�ͷ�SessionId
bool ControlMemMgr::realseSid(const int& r_sid)
{
    if (m_pGobalInfos == NULL) return false;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    m_pGobalInfos->releadSid(r_sid);
    SpaceMgrBase::unlock();
    return true;
}
bool ControlMemMgr::registerSession(SessionInfo& r_sessionInfo)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_sessionInfoMgr.addElement(r_sessionInfo) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::unRegisterSession(const SessionInfo& r_sessionInfo)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_sessionInfoMgr.deleteElement(r_sessionInfo) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::getSessionInfos(vector<SessionInfo> &r_sessionInfoList)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_sessionInfoMgr.getElements(r_sessionInfoList) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
//���³�ʼ��Session�б� add by gaojf 2009-3-2 3:54
bool ControlMemMgr::reInitSessionInfos()
{
    m_sessionInfoMgr.reInit();
    return true;
}
///end

///<attach��ʽ��ʼ��
bool ControlMemMgr::attach_init()
{
    //����ShmKey
    T_FILENAMEDEF  t_ctlFileName;
    sprintf(t_ctlFileName, "%s/ctl/%s.ctl", getenv(MDB_HOMEPATH.c_str()), m_spaceHeader.m_dbName);
    m_spaceHeader.m_shmKey = ftok(t_ctlFileName, m_spaceHeader.m_spaceCode);
    if (m_spaceHeader.m_shmKey < 0)
    {
#ifdef _DEBUG_
        cout << "ftok(" << t_ctlFileName << ",";
        cout << m_spaceHeader.m_spaceCode << ") false!" << endl;
#endif
        return false;
    }
    if (SpaceMgrBase::attach() == false)
    {
#ifdef _DEBUG_
        cout << "SpaceMgrBase::attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return initAfterAttach();
}
bool ControlMemMgr::initAfterAttach()
{
    size_t  t_offSet = 0;
    //1. ������������ռ�ͷm_spaceHeader��Ϣ
    t_offSet += sizeof(SpaceInfo);
    //2. attach��ʼ��ȫ����Ϣ
    m_pGobalInfos = (MDbGlobalInfo*)(m_spaceHeader.m_shmAddr + t_offSet);
    t_offSet += sizeof(MDbGlobalInfo);
    //3. ��ռ���Ϣ��ʼ��
    if (m_spInfoMgr.attach_init(m_spaceHeader.m_shmAddr, t_offSet) == false)
    {
#ifdef _DEBUG_
        cout << "m_spInfoMgr.attach_init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_spInfoMgr.getSize();
    //4. ��ʼ��ȫ�ֲ�����Ϣ
    if (m_gParamMgr.attach_init(m_spaceHeader.m_shmAddr, t_offSet) == false)
    {
#ifdef _DEBUG_
        cout << "m_gParamMgr.attach_init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_gParamMgr.getSize();
    //5.��ʼ������������Ϣ
    if (m_tbDescMgr.attach_init(m_spaceHeader.m_shmAddr, t_offSet) == false)
    {
#ifdef _DEBUG_
        cout << "m_tbDescMgr.attach_init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_tbDescMgr.getSize();
    //6.��ʼ��������������Ϣ
    if (m_idxDescMgr.attach_init(m_spaceHeader.m_shmAddr, t_offSet) == false)
    {
#ifdef _DEBUG_
        cout << "m_idxDescMgr.attach_init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_idxDescMgr.getSize();
    //7.��ʼ��Session��Ϣ�б�
    if (m_sessionInfoMgr.attach_init(m_spaceHeader.m_shmAddr, t_offSet) == false)
    {
#ifdef _DEBUG_
        cout << "m_sessionInfoMgr.attach_init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_sessionInfoMgr.getSize();
    return true;
}

bool ControlMemMgr::addTableDesc(const TableDesc& r_tableDesc, TableDesc* &r_pTableDesc,
                                 const T_SCN& r_scn)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    int t_flag = 1; //֧�ֱ�����������
    if (m_tbDescMgr.addElement(r_tableDesc, r_pTableDesc, t_flag) == false)
    {
        t_bRet = false;
    }
    m_tbDescMgr.update_scn(r_scn);
    r_pTableDesc->initpageinfoscn(r_scn);
    SpaceMgrBase::unlock();
    updateTbDescTime();
    return t_bRet;
}

//ɾ����������ʱ��ǰ�ᣬ������Ѿ�truncate���Ҷ�Ӧ������Ҳȫ��ɾ��
bool ControlMemMgr::deleteTableDesc(const TableDesc& r_tableDesc, const T_SCN& r_scn)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    int  t_flag = 1;
    if (m_tbDescMgr.deleteElement(r_tableDesc, t_flag) == false)
    {
        t_bRet = false;
    }
    m_tbDescMgr.update_scn(r_scn);
    SpaceMgrBase::unlock();
    updateTbDescTime();
    return t_bRet;
}
bool ControlMemMgr::getTableDescList(vector<TableDesc> &r_tableDescList)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    if (m_tbDescMgr.getElements(r_tableDescList) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::getTableDescByName(const char* r_tableName, TableDesc* &r_pTableDesc)
{
    TableDesc t_tableDesc;
    strcpy(t_tableDesc.m_tableDef.m_tableName, r_tableName);
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    if (m_tbDescMgr.getElement(t_tableDesc, r_pTableDesc) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::addIndexDesc(const IndexDesc& r_indexDesc, IndexDesc* &r_pIndexDesc, const T_SCN& r_scn)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    int t_flag = 1; //֧�ֱ�����������
    if (m_idxDescMgr.addElement(r_indexDesc, r_pIndexDesc, t_flag) == false)
    {
        t_bRet = false;
    }
    m_idxDescMgr.update_scn(r_scn);
    r_pIndexDesc->initpageinfoscn(r_scn);
    SpaceMgrBase::unlock();
    updateIndexTime();
    updateTbDescTime();
    return t_bRet;
}

bool ControlMemMgr::deleteIndexDesc(const IndexDesc& r_indexDesc, const T_SCN& r_scn)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    int  t_flag = 1;
    if (m_idxDescMgr.deleteElement(r_indexDesc, t_flag) == false)
    {
        t_bRet = false;
    }
    m_idxDescMgr.update_scn(r_scn);
    SpaceMgrBase::unlock();
    updateIndexTime();
    updateTbDescTime();
    return t_bRet;
}
bool ControlMemMgr::getIndexDescByName(const char* r_indexName, IndexDesc* &r_pIndexDesc)
{
    IndexDesc t_indexDesc;
    strcpy(t_indexDesc.m_indexDef.m_indexName, r_indexName);
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    if (m_idxDescMgr.getElement(t_indexDesc, r_pIndexDesc) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
void ControlMemMgr::getIndexDescByPos(const size_t& r_offSet, IndexDesc* &r_pIndexDesc)
{
    r_pIndexDesc = (IndexDesc*)(getSpaceAddr() + r_offSet);
}
void ControlMemMgr::updateTbDescTime()
{
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return;
    }
    m_pGobalInfos->updateTbTime();
    SpaceMgrBase::unlock();
}
void ControlMemMgr::updateIndexTime()
{
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return;
    }
    m_pGobalInfos->updateIdxTime();
    SpaceMgrBase::unlock();
}
void ControlMemMgr::updateSpTime()
{
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return;
    }
    m_pGobalInfos->updateSpTime();
    SpaceMgrBase::unlock();
}
void ControlMemMgr::updateDbTime()
{
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return;
    }
    m_pGobalInfos->updateDbTime();
    SpaceMgrBase::unlock();
}

bool ControlMemMgr::addGlobalParam(const GlobalParam& r_gParam, const T_SCN& r_scn)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    int t_flag = 0;
    if (m_gParamMgr.addElement(r_gParam, t_flag) == false)
    {
        t_bRet = false;
    }
    m_gParamMgr.update_scn(r_scn);
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::getGlobalParam(const char* r_paramname, GlobalParam& r_gParam)
{
    if (strlen(r_paramname) >= sizeof(T_NAMEDEF))
    {
#ifdef _DEBUG_
        cout << "���ƹ���!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    GlobalParam t_gParam;
    strcpy(t_gParam.m_paramName, r_paramname);
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_gParamMgr.getElement(t_gParam, r_gParam) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::getGlobalParams(vector<GlobalParam>& r_gparamList)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_gParamMgr.getElements(r_gparamList) == false)
    {
        t_bRet = false;
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}

bool ControlMemMgr::updateGlobalParam(const char* r_paramname, const char* r_value, const T_SCN& r_scn)
{
    GlobalParam t_gParam;
    if (strlen(r_paramname) >= sizeof(T_NAMEDEF) ||
            strlen(r_value) >= sizeof(T_GPARAMVALUE))
    {
#ifdef _DEBUG_
        cout << "���ƻ�ֵ����!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    strcpy(t_gParam.m_paramName, r_paramname);
    strcpy(t_gParam.m_value, r_value);
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_gParamMgr.updateElement(t_gParam) == false)
    {
        t_bRet = false;
    }
    m_gParamMgr.update_scn(r_scn);
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool ControlMemMgr::deleteGlobalParam(const char* r_paramname, const T_SCN& r_scn)
{
    if (strlen(r_paramname) >= sizeof(T_NAMEDEF))
    {
#ifdef _DEBUG_
        cout << "���ƹ���!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    int t_flag = 0;
    GlobalParam t_gParam;
    strcpy(t_gParam.m_paramName, r_paramname);
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return false;
    }
    if (m_gParamMgr.deleteElement(t_gParam, t_flag) == false)
    {
        t_bRet = false;
    }
    m_gParamMgr.update_scn(r_scn);
    SpaceMgrBase::unlock();
    return t_bRet;
}

//���Խӿ�
bool ControlMemMgr::dumpSpaceInfo(ostream& r_os)
{
    r_os << "---------��ռ�:" << m_pSpHeader->m_spaceName << "���� ��ʼ!---------" << endl;
    SpaceMgrBase::dumpSpaceInfo(r_os);
    r_os << "---------���ݿ�ȫ����Ϣ----------------------" << endl;
    r_os << "m_spUpTime:" << ctime(&(m_pGobalInfos->m_spUpTime)) << endl;
    r_os << "m_tbUpTime:" << ctime(&(m_pGobalInfos->m_tbUpTime)) << endl;
    r_os << "m_idxUpTime:" << ctime(&(m_pGobalInfos->m_idxUpTime)) << endl;
    r_os << "m_nextSpCode:" << m_pGobalInfos->m_nextSpCode << endl;
    r_os << "m_curSessionId:" << m_pGobalInfos->m_curSessionId << endl;
    r_os << "m_scn:         " << m_pGobalInfos->m_scn << endl;
    r_os << "m_scnlacth:    " << m_pGobalInfos->m_scnlacth << endl;
    r_os << "------SESSION״̬�б�(��ռ��)----------------" << endl;
    for (int i = 0; i < MAX_SESSION_NUM; i++)
    {
        if (m_pGobalInfos->m_sidState[i])
        {
            r_os << "  " << i << " " << m_pGobalInfos->m_sidState[i] << endl;
        }
    }
    r_os << "-------------------------------------" << endl;
    r_os << "m_reserve:" << m_pGobalInfos->m_reserve << endl;
    r_os << "--------------------------------------------" << endl;
    r_os << "---------��ռ���Ϣ----------------------" << endl;
    m_spInfoMgr.dumpInfo(r_os);
    r_os << "-----------------------------------------" << endl;
    r_os << "---------ȫ�ֲ�����Ϣ----------------------" << endl;
    m_gParamMgr.dumpInfo(r_os);
    r_os << "-----------------------------------------" << endl;
    r_os << "---------����������Ϣ----------------------" << endl;
    m_tbDescMgr.dumpInfo(r_os);
    r_os << "-----------------------------------------" << endl;
    r_os << "---------������������Ϣ----------------------" << endl;
    m_idxDescMgr.dumpInfo(r_os);
    r_os << "-----------------------------------------" << endl;
    r_os << "---------SESSION��Ϣ----------------------" << endl;
    m_sessionInfoMgr.dumpInfo(r_os);
    r_os << "-----------------------------------------" << endl;
    r_os << "---------��ռ�:" << m_pSpHeader->m_spaceName << "���� ��ֹ!---------" << endl;
    return true;
}
//add by gaojf MDB2.0
void ControlMemMgr::getscn(T_SCN& r_scn, const int r_flag)
{
    return m_pGobalInfos->getscn(r_scn, r_flag);
}

void ControlMemMgr::gettid(long& transid)
{
    return m_pGobalInfos->gettid(transid);
}

DescPageInfos* ControlMemMgr::getdescinfo(const char& r_type, const size_t& r_offset)
{
    DescPageInfos* t_pdesc = NULL;
    if (r_type == '1')
    {
        t_pdesc = m_tbDescMgr.getElement(r_offset);
    }
    else  //r_type=='2'
    {
        t_pdesc = m_idxDescMgr.getElement(r_offset);
    }
    return t_pdesc;
}
size_t ControlMemMgr::getdescOffset(const char& r_type, const DescPageInfos* r_desc)
{
    size_t t_offset = 0;
    if (r_type == '1')
    {
        t_offset = m_tbDescMgr.getOffset((const TableDesc*)r_desc);
    }
    else  //r_type=='2'
    {
        t_offset = m_idxDescMgr.getOffset((const IndexDesc*)r_desc);
    }
    return t_offset;
}

void ControlMemMgr::getlastckpscn(T_SCN& r_scn)
{
    m_pGobalInfos->getlastckpscn(r_scn);
}
void ControlMemMgr::setlastckpscn(const T_SCN& r_scn)
{
    m_pGobalInfos->setlastckpscn(r_scn);
}

/**
 *afterStartMdb ���ݿ�������,����scn��Ϣ��.
 * ʧ�� �׳��쳣
 */
void ControlMemMgr::afterStartMdb()
{
    m_pGobalInfos->init(1);          //����һЩȫ����Ϣ,scn��0
    m_spInfoMgr.startmdb_init();     //��ռ���Ϣ
    m_gParamMgr.startmdb_init();     //ȫ�ֲ���
    m_tbDescMgr.startmdb_init();     //��������
    m_idxDescMgr.startmdb_init();    //����������
    m_sessionInfoMgr.startmdb_init();//Session��Ϣ
}

