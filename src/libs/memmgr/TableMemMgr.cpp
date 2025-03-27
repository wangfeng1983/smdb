#include "TableMemMgr.h"

TableMemMgr::TableMemMgr()
{
    m_tableSpaceMgrList.clear();
}
TableMemMgr::~TableMemMgr()
{
    for (vector<TableSpaceMgr*>::iterator t_itr = m_tableSpaceMgrList.begin();
            t_itr != m_tableSpaceMgrList.end(); t_itr++)
    {
        delete *t_itr;
        *t_itr = NULL;
    }
    m_tableSpaceMgrList.clear();
}


//根据表空间定义信息,创建表空间
bool TableMemMgr::createTbSpace(SpaceInfo& r_spaceInfo, const int& r_flag)
{
    TableSpaceMgr* t_pTbSpaceMgr = new TableSpaceMgr(r_spaceInfo);
    if (t_pTbSpaceMgr->createTbSpace(r_flag) == false)
    {
#ifdef _DEBUG_
        cout << "t_pTbSpaceMgr->createTbSpace() false!" << endl;
#endif
        return false;
    }
    r_spaceInfo = *(t_pTbSpaceMgr->getSpaceInfo());
    m_tableSpaceMgrList.push_back(t_pTbSpaceMgr);
    return true;
}

bool TableMemMgr::attachAllTbSpace()
{
    bool t_bRet = true;
    for (vector<TableSpaceMgr*>::iterator t_itr = m_tableSpaceMgrList.begin();
            t_itr != m_tableSpaceMgrList.end(); t_itr++)
    {
        if ((*t_itr)->attach() == false)
        {
#ifdef _DEBUG_
            cout << "attach() !"
                 << " " << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
        }
    }
    return t_bRet;
}
bool TableMemMgr::detachAllTbSpace()
{
    bool t_bRet = true;
    for (vector<TableSpaceMgr*>::iterator t_itr = m_tableSpaceMgrList.begin();
            t_itr != m_tableSpaceMgrList.end(); t_itr++)
    {
        if ((*t_itr)->detach() == false)
        {
#ifdef _DEBUG_
            cout << "detach() !"
                 << " " << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
        }
    }
    return t_bRet;
}
bool TableMemMgr::deleteAllTbSpace()
{
    bool t_bRet = true;
    for (vector<TableSpaceMgr*>::iterator t_itr = m_tableSpaceMgrList.begin();
            t_itr != m_tableSpaceMgrList.end(); t_itr++)
    {
        if ((*t_itr)->deleteTbSpace() == false)
        {
#ifdef _DEBUG_
            cout << "deleteTbSpace false!"
                 << " " << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
        }
    }
    return t_bRet;
}

//初始化表空间
//r_status: -1 初始(一次未做); 0：checkpoint备份; 1：stop database 备份  add by gaojf 2012/5/8 14:26:25
bool TableMemMgr::initSpaceInfo(const int& r_flag, const char* r_path, const int& r_status)
{
    bool t_bRet = true;
    int  t_flag = r_flag;
    for (vector<TableSpaceMgr*>::iterator t_itr = m_tableSpaceMgrList.begin();
            t_itr != m_tableSpaceMgrList.end(); t_itr++)
    {
        t_flag = r_flag;
        if ((*t_itr)->getSpaceInfo()->m_type == TABLE_SPACE_NP)
        {
            if (r_status != 1) //非backup备份出来则该表空间重新初始化
            {
                t_flag = 0;
            }
        }
        if ((*t_itr)->initTbSpace(t_flag, r_path) == false)
        {
#ifdef _DEBUG_
            cout << "initTbSpace(" << t_flag << ") false!"
                 << " " << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
        }
    }
    return t_bRet;
}

bool TableMemMgr::dumpDataIntoFile(const char* r_path)
{
    bool t_bRet = true;
#ifdef _DEBUG_
    time_t t_startTime, t_endTime;
    time(&t_startTime);
#endif
    for (int i = 0; i < m_tableSpaceMgrList.size(); i++)
    {
        if (m_tableSpaceMgrList[i]->dumpDataIntoFile(r_path) == false)
            t_bRet = false;
    }
    return t_bRet;
    /*
    int t_threadNum=0,t_i;
    ThreadParam *t_threadParam;
    t_threadParam = new ThreadParam[m_tableSpaceMgrList.size()];
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setstacksize(&t_attr, 20*1024*1024 );
    for(vector<TableSpaceMgr*>::iterator t_itr=m_tableSpaceMgrList.begin();
      t_itr!=m_tableSpaceMgrList.end();t_itr++)
    {
    t_threadParam[t_threadNum].m_pSpaceMgr=*t_itr;
    strcpy(t_threadParam[t_threadNum].m_path,r_path);
    if (pthread_create(&(t_threadParam[t_threadNum].m_threadId),&t_attr,
                       (void *(*)(void *))dumpDataInfoFile_r,
                       &(t_threadParam[t_threadNum]))!= 0)
    {
      #ifdef _DEBUG_
        cout<<"Create Thread Error"<<endl;
      #endif
      for(t_i = 0;t_i < t_threadNum;t_i++)
          pthread_join(t_threadParam[t_i].m_threadId,NULL);
      delete[] t_threadParam;
      pthread_attr_destroy(&t_attr);
      return false;
    }else
    {
      t_threadNum++;
    }
    }
    pthread_attr_destroy(&t_attr);
    //等待各个线程结束
    for(t_i = 0; t_i < t_threadNum; t_i++)
    {
    pthread_join(t_threadParam[t_i].m_threadId,NULL);
    if (t_threadParam[t_i].m_bRet == false)
    {
      #ifdef _DEBUG_
        cout<<"子线程处理错误 id="<<t_i<<" "<<__FILE__<<__LINE__<<endl;
      #endif
      t_bRet = false;
    }
    }
    delete[] t_threadParam;
    #ifdef _DEBUG_
      time(&t_endTime);
      cout<<"线程数："<<t_threadNum<<" ";
      cout<<"多线程处理耗时为:"<<difftime(t_endTime,t_startTime)<<"秒"<<endl;
    #endif
    */
    return t_bRet;
}
bool TableMemMgr::attach_int(SpaceInfo& r_spaceInfo)
{
    TableSpaceMgr* t_pTbSpaceMgr = new TableSpaceMgr(r_spaceInfo);
    m_tableSpaceMgrList.push_back(t_pTbSpaceMgr);
    if (t_pTbSpaceMgr->attach_init() == false)
    {
#ifdef _DEBUG_
        cout << "attach_init() !" << " " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    r_spaceInfo = *(t_pTbSpaceMgr->getSpaceInfo());
    return true;
}
bool TableMemMgr::freePage(MemManager* r_memMgr, const ShmPosition& r_pagePos, const bool& r_undoflag)
{
    bool t_bRet = true;
    unsigned int t_spaceCode;
    t_spaceCode = r_pagePos.getSpaceCode();
    //1.先找到该page对应的TableSpaceMgr:t_tableMgrItr
    for (m_tbMgrItr = m_tableSpaceMgrList.begin();
            m_tbMgrItr != m_tableSpaceMgrList.end(); m_tbMgrItr++)
    {
        if (t_spaceCode == (*m_tbMgrItr)->getSpaceCode())
        {
            break;
        }
    }
    if (m_tbMgrItr == m_tableSpaceMgrList.end())
    {
#ifdef _DEBUG_
        cout << "未找到对应的TableSpaceMgr:t_tableMgrItr!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
        return t_bRet;
    }
    //2.调用t_tableMgrItr回收该page
    if ((*m_tbMgrItr)->freePage(r_memMgr, r_pagePos, r_undoflag) == false)
    {
#ifdef _DEBUG_
        cout << "(*t_tableMgrItr)->freePage(r_pagePos) false!" << __FILE__ << __LINE__ << endl;
#endif
        t_bRet = false;
    }
    return t_bRet;
}

bool TableMemMgr::allocatePage(MemManager* r_memMgr, const char* r_spaceName,
                               PageInfo* &r_pPage, ShmPosition& r_pagePos, const bool& r_undoflag)
{
    //1.先找到该page对应的TableSpaceMgr:t_tableMgrItr
    for (m_tbMgrItr = m_tableSpaceMgrList.begin();
            m_tbMgrItr != m_tableSpaceMgrList.end(); m_tbMgrItr++)
    {
        if (strcasecmp(r_spaceName, (*m_tbMgrItr)->getSpaceName()) == 0)
        {
            break;
        }
    }
    if (m_tbMgrItr == m_tableSpaceMgrList.end())
    {
#ifdef _DEBUG_
        cout << "未找到对应的TableSpaceMgr!" << r_spaceName << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return (*m_tbMgrItr)->allocatePage(r_memMgr, r_pPage, r_pagePos, r_undoflag);
}

//根据Slot地址，取对应的页面信息和页面地址
bool TableMemMgr::getPageBySlot(const ShmPosition& r_slot, ShmPosition& r_pagePos, PageInfo* &r_pPage)
{
    bool t_bRet = true;
    unsigned int  t_spaceCode = r_slot.getSpaceCode();
    size_t        t_slotOffSet, t_pageOffSet;
    t_slotOffSet = r_slot.getOffSet();
    for (m_tbMgrItr = m_tableSpaceMgrList.begin();
            m_tbMgrItr != m_tableSpaceMgrList.end(); m_tbMgrItr++)
    {
        if (t_spaceCode == (*m_tbMgrItr)->getSpaceCode())
        {
            break;
        }
    }
    if (m_tbMgrItr == m_tableSpaceMgrList.end())
    {
        t_bRet = false;
        return t_bRet;
    }
    //通过t_slotOffSet计算t_pageOffSet
    if ((*m_tbMgrItr)->getPagePosBySlotPos(t_slotOffSet, t_pageOffSet) == false)
    {
        t_bRet = false;
        return t_bRet;
    }
    r_pagePos.setValue(t_spaceCode, t_pageOffSet);
    r_pPage = (PageInfo*)((*m_tbMgrItr)->getSpaceAddr() + r_pagePos.getOffSet());
    return t_bRet;
}

//调试接口
bool TableMemMgr::dumpSpaceInfo(ostream& r_os)
{
    for (m_tbMgrItr = m_tableSpaceMgrList.begin();
            m_tbMgrItr != m_tableSpaceMgrList.end();
            m_tbMgrItr++)
    {
        (*m_tbMgrItr)->dumpSpaceInfo(r_os);
    }
    return true;
}

void* TableMemMgr::dumpDataInfoFile_r(ThreadParam& r_param)
{
    r_param.m_bRet = true;
    if (r_param.m_pSpaceMgr->dumpDataIntoFile(r_param.m_path) == false)
    {
        r_param.m_bRet = false;
    }
    pthread_exit(NULL);
    return NULL;
}

void TableMemMgr::getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName)
{
    // 若没有指定表空间 则计算MDB所有的表空间占用率
    float fPercent;
    if (cTableSpaceName == NULL)
    {
        for (int i = 0; i < m_tableSpaceMgrList.size(); ++i)
        {
            fPercent = m_tableSpaceMgrList[i]->getTableSpaceUsedPercent();
            vUserdPercent.insert(map<string, float>::value_type(m_tableSpaceMgrList[i]->getSpaceName(), fPercent));
        }
    }
    else
    {
        for (int i = 0; i < m_tableSpaceMgrList.size(); ++i)
        {
            if (strcasecmp(m_tableSpaceMgrList[i]->getSpaceName(), cTableSpaceName) == 0)
            {
                fPercent = m_tableSpaceMgrList[i]->getTableSpaceUsedPercent();
                vUserdPercent.insert(map<string, float>::value_type(m_tableSpaceMgrList[i]->getSpaceName(), fPercent));
                break;
            }
        }
    }
    return;
}
void TableMemMgr::afterStartMdb()
{
    for (vector<TableSpaceMgr*>::iterator t_itr = m_tableSpaceMgrList.begin();
            t_itr != m_tableSpaceMgrList.end(); t_itr++)
    {
        (*t_itr)->afterStartMdb();
    }
}
