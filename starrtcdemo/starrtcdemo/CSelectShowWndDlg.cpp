// CSelectShowWndDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CSelectShowWndDlg.h"
#include "afxdialogex.h"


// CSelectShowWndDlg 对话框

IMPLEMENT_DYNAMIC(CSelectShowWndDlg, CDialogEx)

CSelectShowWndDlg::CSelectShowWndDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SELECT_SHOW_WND, pParent)
{
	m_nSelect = 0;
	m_bSure = false;
}

CSelectShowWndDlg::~CSelectShowWndDlg()
{
}

void CSelectShowWndDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSelectShowWndDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CSelectShowWndDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CSelectShowWndDlg 消息处理程序


void CSelectShowWndDlg::OnBnClickedButton1()
{
	if (((CButton *)GetDlgItem(IDC_RADIO1))->GetCheck() == 1)
	{
		m_nSelect = 0;
	}

	if (((CButton *)GetDlgItem(IDC_RADIO2))->GetCheck() == 1)
	{
		m_nSelect = 1;
	}

	if (((CButton *)GetDlgItem(IDC_RADIO3))->GetCheck() == 1)
	{
		m_nSelect = 2;
	}

	if (((CButton *)GetDlgItem(IDC_RADIO4))->GetCheck() == 1)
	{
		m_nSelect = 3;
	}
	m_bSure = true;

	OnOK();
}
