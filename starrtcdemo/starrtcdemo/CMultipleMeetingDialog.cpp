// CMultipleMeetingDialog.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CMultipleMeetingDialog.h"
#include "afxdialogex.h"
#include "HttpClient.h"
#include "json.h"
#include "CropType.h"
#include "CCreateLiveDialog.h"
#include "CInterfaceUrls.h"
enum MEETING_LIST_REPORT_NAME
{
	MEETING_NAME = 0,
	MEETING_ID,
	//MEETING_STATUS,
	MEETING_CREATER
};

// CMultipleMeetingDialog 对话框

IMPLEMENT_DYNAMIC(CMultipleMeetingDialog, CDialogEx)

CMultipleMeetingDialog::CMultipleMeetingDialog(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MULTIPLE_MEETING, pParent)
{
	m_pUserManager = pUserManager;
	XHMeetingManager::addChatroomGetListListener(this);
	m_pMeetingManager = new XHMeetingManager(this);
	m_pSoundManager = new CSoundManager(this);
	m_pCurrentProgram = NULL;
	m_pConfig = NULL;
}

CMultipleMeetingDialog::~CMultipleMeetingDialog()
{
	m_bStop = true;
	m_bExit = true;
	m_pConfig = NULL;
	XHMeetingManager::addChatroomGetListListener(NULL);
	if (m_pSoundManager != NULL)
	{
		delete m_pSoundManager;
		m_pSoundManager = NULL;
	}

	if (m_pMeetingManager != NULL)
	{
		delete m_pMeetingManager;
		m_pMeetingManager = NULL;
	}
	
	if (m_pDataShowView != NULL)
	{
		delete m_pDataShowView;
		m_pDataShowView = NULL;
	}
}

void CMultipleMeetingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTCONTROL_MEETING_LIST, m_MeetingList);
	DDX_Control(pDX, IDC_STATIC__MEETING_SHOW_AEWA, m_ShowArea);
}


BEGIN_MESSAGE_MAP(CMultipleMeetingDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_MEETING_LISTBRUSH, &CMultipleMeetingDialog::OnBnClickedButtonMeetingListbrush)
	ON_NOTIFY(NM_CLICK, IDC_LISTCONTROL_MEETING_LIST, &CMultipleMeetingDialog::OnNMClickListcontrolMeetingList)
	ON_BN_CLICKED(IDC_BUTTON_CREATE_MEETING, &CMultipleMeetingDialog::OnBnClickedButtonCreateMeeting)
END_MESSAGE_MAP()


// CMultipleMeetingDialog 消息处理程序


void CMultipleMeetingDialog::OnBnClickedButtonMeetingListbrush()
{
	getMeetingList();
}



BOOL CMultipleMeetingDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_MeetingList.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK;
	lStyle |= LVS_REPORT;
	SetWindowLong(m_MeetingList.m_hWnd, GWL_STYLE, lStyle);

	DWORD dwStyle = m_MeetingList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;                                        //选中某行使整行高亮(LVS_REPORT)
	dwStyle |= LVS_EX_GRIDLINES;                                            //网格线(LVS_REPORT)
	//dwStyle |= LVS_EX_CHECKBOXES;                                            //CheckBox
	m_MeetingList.SetExtendedStyle(dwStyle);

	m_MeetingList.InsertColumn(MEETING_ID, _T("ID"), LVCFMT_LEFT, 210);
	m_MeetingList.InsertColumn(MEETING_NAME, _T("Name"), LVCFMT_LEFT, 120);
	m_MeetingList.InsertColumn(MEETING_CREATER, _T("Creator"), LVCFMT_LEFT, 80);
	//m_MeetingList.InsertColumn(MEETING_STATUS, _T("liveState"), LVCFMT_LEFT, 80);
	getMeetingList();

	CRect rect;
	::GetWindowRect(m_ShowArea, rect);
	CRect dlgRect;
	::GetWindowRect(this->m_hWnd, dlgRect);
	int left = rect.left - dlgRect.left - 7;
	int top = rect.top - dlgRect.top - 25;

	CRect showRect(left, top, left + rect.Width() - 5, top + rect.Height() - 15);

	m_pDataShowView = new CDataShowView();
	m_pDataShowView->setDrawRect(showRect);

	CPicControl *pPicControl = new CPicControl();
	pPicControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, showRect, this, WM_USER + 100);
	//mShowPicControlVector[i] = pPicControl;
	pPicControl->ShowWindow(SW_SHOW);
	dwStyle = ::GetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE);
	::SetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);

	m_pDataShowView->m_pPictureControlArr.push_back(pPicControl);
	pPicControl->setInfo(this);

	CRect rectClient = showRect;
	CRect rectChild(rectClient.right - (int)(rectClient.Width()*0.25), rectClient.top, rectClient.right, rectClient.bottom);

	for (int n = 0; n < 6; n++)
	{
		CPicControl *pPictureControl = new CPicControl();
		pPictureControl->setInfo(this);
		rectChild.top = rectClient.top + (long)(n * rectClient.Height()*0.25);
		rectChild.bottom = rectClient.top + (long)((n + 1) * rectClient.Height()*0.25);
		pPictureControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, rectChild, this, WM_USER + 100 + n + 1);
		m_pDataShowView->m_pPictureControlArr.push_back(pPictureControl);
		DWORD dwStyle1 = ::GetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE);
		::SetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE, dwStyle1 | SS_NOTIFY);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CMultipleMeetingDialog::getMeetingList()
{
	char strListType[10] = { 0 };
	sprintf_s(strListType, "%d,%d", CHATROOM_LIST_TYPE_MEETING, CHATROOM_LIST_TYPE_MEETING_PUSH);
	if (m_pConfig != NULL && m_pConfig->m_bAEventCenterEnable)
	{
		list<ChatroomInfo> listData;
		CInterfaceUrls::demoQueryList(strListType, listData);
		chatroomQueryAllListOK(listData);
	}
	else
	{
		XHMeetingManager::getMeetingList("", strListType);
	}
}

void CMultipleMeetingDialog::setConfig(CConfigManager* pConfig)
{
	m_pConfig = pConfig;
}

void CMultipleMeetingDialog::getLocalSoundData(char* pData, int nLength)
{
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->insertAudioRaw((uint8_t*)pData, nLength);
	}
}

void CMultipleMeetingDialog::querySoundData(char** pData, int* nLength)
{
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->querySoundData((uint8_t**)pData, nLength);
	}
}
/**
 * 查询聊天室列表回调
 */
int CMultipleMeetingDialog::chatroomQueryAllListOK(list<ChatroomInfo>& listData)
{
	m_MeetingList.DeleteAllItems();
	list<ChatroomInfo>::iterator iter = listData.begin();
	int i = 0;
	for (; iter != listData.end(); iter++)
	{
		m_MeetingList.InsertItem(i, (char*)(*iter).m_strName.c_str());
		m_MeetingList.SetItemText(i, MEETING_ID, (char*)(*iter).m_strRoomId.c_str());
		m_MeetingList.SetItemText(i, MEETING_CREATER, (char*)(*iter).m_strCreaterId.c_str());
	}
	return 0;
}

/**
 * 有新用户加入会议
 * @param meetingID 会议ID
 * @param userID 新加入者ID
 */
void CMultipleMeetingDialog::onJoined(string meetID, string userID)
{
	if (m_pDataShowView != NULL)
	{
		bool bBigPic = false;
		if (m_pUserManager->m_ServiceParam.m_strUserId == userID)
		{
			bBigPic = true;
		}
		m_pDataShowView->addUser(userID, bBigPic);
		m_pDataShowView->setShowPictures();
	}
}

/**
 * 有人离开会议
 * @param meetingID 会议ID
 * @param userID 离开者ID
 */
void CMultipleMeetingDialog::onLeft(string meetingID, string userID)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeUser(userID);
		m_pMeetingManager->changeToSmall(userID);
		string strUserId = "";
		bool bLeft = m_pDataShowView->isLeftOneUser(strUserId);
		if (bLeft && strUserId != "")
		{
			m_pMeetingManager->changeToBig(strUserId);
			m_pDataShowView->changeShowStyle(strUserId, true);
		}
		m_pDataShowView->setShowPictures();
	}
}

/**
 * 一些异常情况引起的出错，请在收到该回调后主动断开会议
 * @param meetingID 会议ID
 * @param error 错误信息
 */
void CMultipleMeetingDialog::onMeetingError(string meetingID, string error)
{
	CString strErr;
	strErr.Format("live error errmsg:%s", error);
	AfxMessageBox(strErr);
}


/**
 * 聊天室成员数变化
 * @param number
 */
void CMultipleMeetingDialog::onMembersUpdated(int number)
{
}

/**
 * 自己被踢出聊天室
 */
void CMultipleMeetingDialog::onSelfKicked()
{
}

/**
 * 自己被踢出聊天室
 */
void CMultipleMeetingDialog::onSelfMuted(int seconds)
{
}

/**
 * 收到消息
 * @param message
 */
void CMultipleMeetingDialog::onReceivedMessage(CIMMessage* pMessage)
{
}

/**
 * 收到私信消息
 * @param message
 */
void CMultipleMeetingDialog::onReceivePrivateMessage(CIMMessage* pMessage)
{
}
int CMultipleMeetingDialog::getRealtimeData(string strUserId, uint8_t* data, int len)
{
	return 0;
}
int CMultipleMeetingDialog::getVideoRaw(string strUserId, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(FMT_YUV420P, strUserId, w, h, videoData, videoDataLen);
	}
	return 0;
}


void CMultipleMeetingDialog::addUpId()
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->addUser(m_pUserManager->m_ServiceParam.m_strUserId,true);
		m_pDataShowView->setShowPictures();
	}
}

void CMultipleMeetingDialog::insertVideoRaw(uint8_t* videoData, int dataLen, int isBig)
{
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->insertVideoRaw(videoData, dataLen, isBig);
	}
}

int CMultipleMeetingDialog::cropVideoRawNV12(int w, int h, uint8_t* videoData, int dataLen, int yuvProcessPlan, int rotation, int needMirror, uint8_t* outVideoDataBig, uint8_t* outVideoDataSmall)
{
	int ret = 0;
	if (m_pMeetingManager != NULL)
	{
		ret = m_pMeetingManager->cropVideoRawNV12(w, h, videoData, dataLen, yuvProcessPlan, 0, 0, outVideoDataBig, outVideoDataSmall);
	}
	return ret;
}
void CMultipleMeetingDialog::drawPic(YUV_TYPE type, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(type, m_pUserManager->m_ServiceParam.m_strUserId, w, h, videoData, videoDataLen);
	}
}

void CMultipleMeetingDialog::changeShowStyle(string strUserId)
{
	string changeUserId = "";
	if (m_pDataShowView != NULL)
	{
		changeUserId = m_pDataShowView->changeShowStyle(strUserId, true);
		m_pDataShowView->setShowPictures();
	}
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->changeToBig(strUserId);
		if (changeUserId != "")
		{
			m_pMeetingManager->changeToSmall(changeUserId);
		}
	}
}
void CMultipleMeetingDialog::closeCurrentLive()
{
	if (m_pMeetingManager != NULL)
	{
		m_pMeetingManager->leaveMeeting(m_CurrentMeetingId);
	}
	stopGetData();

	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	
}

void CMultipleMeetingDialog::OnNMClickListcontrolMeetingList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}

	*pResult = 0;
	int nItem = -1;
	if (pNMItemActivate != NULL)
	{
		nItem = pNMItemActivate->iItem;
		if (nItem >= 0)
		{
			CString strId = m_MeetingList.GetItemText(nItem, MEETING_ID);
			CString strName = m_MeetingList.GetItemText(nItem, MEETING_NAME);
			CString strCreater = m_MeetingList.GetItemText(nItem, MEETING_CREATER);
			if (m_pCurrentProgram == NULL)
			{
				m_pCurrentProgram = new CLiveProgram();
			}
			m_pCurrentProgram->m_strId = strId;
			m_pCurrentProgram->m_strName = strName;
			m_pCurrentProgram->m_strCreator = strCreater;
			if (m_pMeetingManager != NULL)
			{
				if (strId.GetLength() == 32)
				{
					bool bRet = m_pMeetingManager->joinMeeting(strId.GetBuffer(0));
					if (!bRet)
					{
						AfxMessageBox("申请加入失败！");
					}
					else
					{
						startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
						if (m_pSoundManager != NULL)
						{
							m_pSoundManager->startSoundData(true);
						}
					}
				}			
			}
		}
	}
}


void CMultipleMeetingDialog::OnBnClickedButtonCreateMeeting()
{
	CString strName = "";
	bool bPublic = false;
	XH_CHATROOM_TYPE chatRoomType = XH_CHATROOM_TYPE::XH_CHATROOM_TYPE_GLOBAL_PUBLIC;
	XH_MEETING_TYPE channelType = XH_MEETING_TYPE::XH_MEETING_TYPE_GLOBAL_PUBLIC;

	CCreateLiveDialog dlg(m_pUserManager);
	if (dlg.DoModal() == IDOK)
	{
		strName = dlg.m_strLiveName;
		bPublic = dlg.m_bPublic;
	}
	else
	{
		return;
	}

	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	m_pDataShowView->removeAllUser();
	m_pDataShowView->setShowPictures();

	if (m_pMeetingManager != NULL)
	{
		string strMeetingId = m_pMeetingManager->createMeeting(strName.GetBuffer(0), chatRoomType, channelType);
		if (strMeetingId != "")
		{
			string strInfo = "{\"id\":\"";
			strInfo += strMeetingId;
			strInfo += "\",\"creator\":\"";
			strInfo += m_pUserManager->m_ServiceParam.m_strUserId;
			strInfo += "\",\"name\":\"";
			strInfo += strName.GetBuffer(0);
			strInfo += "\"}";
			if (m_pConfig != NULL && m_pConfig->m_bAEventCenterEnable)
			{
				CInterfaceUrls::demoSaveToList(m_pUserManager->m_ServiceParam.m_strUserId, CHATROOM_LIST_TYPE_MEETING, strMeetingId, strInfo);
			}
			else
			{
				m_pMeetingManager->saveToList(m_pUserManager->m_ServiceParam.m_strUserId, CHATROOM_LIST_TYPE_MEETING, strMeetingId, strInfo);
			}
			bool bRet = m_pMeetingManager->joinMeeting(strMeetingId);
			if (bRet)
			{
				startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
				if (m_pSoundManager != NULL)
				{
					m_pSoundManager->startSoundData(true);
				}
			}
			getMeetingList();
		}
		else
		{
			AfxMessageBox("创建会议失败 !");
		}
	}
}
