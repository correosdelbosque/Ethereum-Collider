#include "stdafx.h"
#include "MainDlg.h"

#include <Winhttp.h>

#include "libdevcore/FixedHash.h"
#include "libdevcrypto/Common.h"
#include "StringProcess.h"
#include "WinHttpClient.h"
#include "jansson.h"

bool bDontUpdate = false;
bool bFirstLoad = true;
bool bQuit = false;

using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned short>;
random_bytes_engine rbe;

IMPLEMENT_DYNAMIC(MainDlg, CDialog)

MainDlg::MainDlg(CWnd* pParent /*=nullptr*/) : CDialog(IDD_DIALOG1, pParent)
{

}

MainDlg::~MainDlg()
{
	bQuit = true;
}

BOOL MainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);

	m_edit.SetLimitText(64);
	m_edit.SetWindowText(_T("Private Key"));
	GetDlgItem(IDC_EDIT2)->SetWindowText(_T("Public Address"));
	GetDlgItem(IDC_EDIT3)->SetWindowText(_T("Ethereum (ETH) Donation Public Address: 0x4b2023c1473DFF118914c2f3b06c5Df912828d77"));

	return TRUE;
}

void MainDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_EDIT1, m_edit);
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(MainDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &MainDlg::OnBnClickedButton1)
	ON_EN_CHANGE(IDC_EDIT1, &MainDlg::OnPrivateKeyChange)
	ON_BN_CLICKED(IDC_BUTTON2, &MainDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &MainDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &MainDlg::OnBnClickedButton4)
END_MESSAGE_MAP()

double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		return;

	PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}

double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

UINT __cdecl BruteForceCracker(LPVOID lpParam)
{
	MainDlg* dlg = (MainDlg*)lpParam;
	CWnd* publicAddr = dlg->GetDlgItem(IDC_EDIT2);
	CWnd* balanceLabel = dlg->GetDlgItem(IDC_STATIC1);

	//dev::Public asd;
	//const std::string asd2 = toAddress(asd).hex();

	//const dev::Secret key = dev::Secret("0000000000000000000000000000000000000000000000000000000000000000");
	//dev::h256 asd777;
	//asd777.randomize(rbe);
	//auto cccc = sign(key, asd777);

	//if (verify(toPublic(key), cccc, asd777))
	//	return 0;

	do
	{
		//StartCounter();

		bDontUpdate = true;

		dev::h256 entropy;
		entropy.randomize(rbe);

		const dev::Secret key = dev::Secret(entropy);
		const std::string addr = "0x" + toAddress(key).hex();
		const std::string secret = dev::toHex(key.makeInsecure().asArray());

		//auto p = GetCounter();

		SetWindowTextA(dlg->m_edit.m_hWnd, secret.c_str());
		SetWindowTextA(publicAddr->m_hWnd, addr.c_str());

		//CString asd;
		//asd.Format(_T("%f"), 1000 / p);
		//SetWindowText(balanceLabel->m_hWnd, asd);

		char url[2047];
		snprintf(url, 2047, "https://api.etherscan.io/api?module=account&action=balance&address=%s&tag=latest", addr.c_str());

		WinHttpClient client(UTF8ToWideString(url));
		client.SendPost();

		if (client.GetResponseHeader().find(L"HTTP/1.1 200 OK\r\n") != std::wstring::npos)
		{
			const std::wstring http_response_content = client.GetResponseContent();
			json_error_t err;

			if (json_t* json_response = json_loads(WideStringToUTF8(http_response_content).c_str(), 0, &err)) // parse and read json response from server
			{
				const std::string balance = json_string_value(json_object_get(json_response, "result"));
				json_decref(json_response);

				if (balance != "0")
				{
					const std::string blc = "Balance: " + balance + " WEI";
					SetWindowTextA(balanceLabel->m_hWnd, blc.c_str());

					bQuit = true;

					dlg->GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
					dlg->GetDlgItem(IDC_EDIT2)->EnableWindow(TRUE);
					dlg->GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
					dlg->GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
					dlg->GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
				}
				else
					SetWindowTextA(balanceLabel->m_hWnd, "Balance: 0 ETH - Empty Wallet.");
			}
		}
		else
			AfxMessageBox(client.GetResponseHeader().c_str(), MB_OK | MB_ICONERROR);

		dlg->UpdateWindow();
	} while (!bQuit);

	return 0;
}

void MainDlg::OnPrivateKeyChange()
{
	if (bDontUpdate || bFirstLoad)
	{
		bDontUpdate = false;
		bFirstLoad  = false;
		return;
	}

	GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);

	CWnd* publicAddr = GetDlgItem(IDC_EDIT2);
	CWnd* balanceLabel = GetDlgItem(IDC_STATIC1);

	CString keyText;
	m_edit.GetWindowText(keyText);

	std::wstring keyString = keyText.GetString();

	if (keyString.length() != 64)
	{
		SetWindowTextA(publicAddr->m_hWnd, "Wrong Private Key.");
		return;
	}

	const dev::Secret key = dev::Secret(WideStringToUTF8(keyString));
	const std::string addr = "0x" + toAddress(key).hex();

	SetWindowTextA(publicAddr->m_hWnd, addr.c_str());

	char url[2047];
	snprintf(url, 2047, "https://api.etherscan.io/api?module=account&action=balance&address=%s&tag=latest", addr.c_str());

	WinHttpClient client(UTF8ToWideString(url));
	client.SendPost();

	if (client.GetResponseHeader().find(L"HTTP/1.1 200 OK\r\n") != std::wstring::npos)
	{
		const std::wstring http_response_content = client.GetResponseContent();
		json_error_t err;

		if (json_t* json_response = json_loads(WideStringToUTF8(http_response_content).c_str(), 0, &err)) // parse and read json response from server
		{
			const std::string balance = json_string_value(json_object_get(json_response, "result"));
			json_decref(json_response);

			if (balance != "0")
			{
				if (balance.length() > 18)
				{
					const size_t len = balance.length() - 18;
					std::string int_part = balance.substr(0, len);
					std::string frac_part = balance.substr(len, balance.length());

					const std::string blc = "Balance: " + int_part + ',' + frac_part + " ETH";
					SetWindowTextA(balanceLabel->m_hWnd, blc.c_str());
				}
				else
				{
					const std::string blc = "Balance: " + balance + " WEI";
					SetWindowTextA(balanceLabel->m_hWnd, blc.c_str());
				}
			}
			else
				SetWindowTextA(balanceLabel->m_hWnd, "Balance: 0 ETH - Empty Wallet.");
		}
	}
	else
		AfxMessageBox(client.GetResponseHeader().c_str(), MB_OK | MB_ICONERROR);

	UpdateWindow();
	GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
}

void MainDlg::OnBnClickedButton1()
{
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
	bQuit = false;
	m_thread = AfxBeginThread(BruteForceCracker, this);
	GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
}

void MainDlg::OnBnClickedButton2()
{
	bQuit = true;

	GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT2)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
}

void MainDlg::OnBnClickedButton3()
{
	bQuit = true;

	GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
	BruteForceCracker(this);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
}

void MainDlg::OnBnClickedButton4()
{
	AfxMessageBox(_T("Engineering by: UnknownSec.\nMade somewhere in the earth planet (2018)."), MB_OK | MB_ICONINFORMATION);
}
