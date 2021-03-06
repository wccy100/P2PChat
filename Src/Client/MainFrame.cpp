
#include "stdafx.h"
#include "MainFrame.h"
#include "defaults.h"
#include "resource.h"
#include "func.h"
#include "md5.h"
#include "setdebugnew.h"
#include <regex>

#include "common_video\libyuv\include\webrtc_libyuv.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

const TCHAR* const kLabel_Title = _T("Label_title");
const TCHAR* const kBtn_Close = _T("btn_close");
const TCHAR* const kBtn_Mini = _T("btn_mini");
const TCHAR* const kBtn_login = _T("btn_login");
const TCHAR* const kBtn_video = _T("btn_video");
const TCHAR* const kBtn_reset = _T("btn_reset");
const TCHAR* const kCtrl_video = _T("ctrl_video");
const TCHAR* const kBtn_EndCall = _T("btn_endcall");
const TCHAR* const kEdit_user = _T("edit_user");
const TCHAR* const kEdit_pass = _T("edit_pass");
const TCHAR* const kEdit_server = _T("edit_server");
const TCHAR* const kLayout_Login = _T("layout_login");
const TCHAR* const kLayout_Work = _T("layout_work");
const TCHAR* const kLayout_Stream = _T("layout_stream");
const TCHAR* const kLabel_tip = _T("label_tip");
const TCHAR* const kList_room = _T("ListRooms");


std::map<MainWindow::UI,CDuiString> vecLayouts;



MainFrame::MainFrame()
	: m_strUserName("")
	, m_strPassWord("")
	, ui_(CONNECT_TO_SERVER)
	, callback_()
{
	vecLayouts.insert(std::make_pair(CONNECT_TO_SERVER, kLayout_Login));
	vecLayouts.insert(std::make_pair(LIST_PEERS, kLayout_Work));
	vecLayouts.insert(std::make_pair(STREAMING, kLayout_Stream));
}

MainFrame::~MainFrame()
{
	PostQuitMessage(0);
}

HWND MainFrame::handle()
{
	return *this;
}

void MainFrame::RegisterObserver(MainWndCallback* callback)
{
	ASSERT(callback);
	callback_ = callback;

}

bool MainFrame::IsWindow()
{
	return true;
}

void MainFrame::SwitchToConnectUI()
{
	ui_ = CONNECT_TO_SERVER;

	doUiChanged();

	pBtnLogin->SetEnabled(true);
	pEditUser->SetEnabled(true);
	pEditPass->SetEnabled(true);
	pEditServer->SetEnabled(true);
}

void MainFrame::SwitchToPeerList(const Peers& peers)
{
	ui_ = LIST_PEERS;

	doUiChanged();

	userInfo_ = peers;

	while (pListUsers->GetCount() > userInfo_.size())
	{
		pListUsers->RemoveAt(pListUsers->GetCount() - 1);
	}

	while (pListUsers->GetCount() < userInfo_.size())
	{
		CListTextElementUI* pListElement = new CListTextElementUI;
		pListUsers->Add(pListElement);
	}

	int index = 0;
	for (const auto &it : userInfo_)
	{
		CListTextElementUI *item = static_cast<CListTextElementUI*>(pListUsers->GetItemAt(index));
		if (item)
		{
			item->SetTag(it.first);
		}
		index++;
	}

	PostNotification("登陆成功");
}

void MainFrame::SwitchToStreamingUI()
{
	ui_ = STREAMING;
	doUiChanged();
}

void MainFrame::MessageBox(const char* caption, const char* text, bool is_error)
{

}

MainWindow::UI MainFrame::current_ui()
{
	return ui_;
}

void MainFrame::StartLocalRenderer(webrtc::VideoTrackInterface* local_video)
{
	local_renderer_.reset(new VideoRenderer(handle(), 1, 1, local_video));
}

void MainFrame::StopLocalRenderer()
{
	local_renderer_.reset();
}

void MainFrame::StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video)
{
	remote_renderer_.reset(new VideoRenderer(handle(), 1, 1, remote_video));
}

void MainFrame::StopRemoteRenderer()
{
	remote_renderer_.reset();
}

void MainFrame::QueueUIThreadCallback(int msg_id, void* data)
{
	::PostMessage(*this, UI_THREAD_CALLBACK, static_cast<WPARAM>(msg_id), reinterpret_cast<LPARAM>(data));
// 	::PostThreadMessage(ui_thread_id_, UI_THREAD_CALLBACK,
// 		static_cast<WPARAM>(msg_id), reinterpret_cast<LPARAM>(data));

}


LPCTSTR MainFrame::GetWindowClassName() const
{
	return _T("shuzai_monitor");
}

CControlUI* MainFrame::CreateControl(LPCTSTR pstrClass)
{
	return NULL;
}

void MainFrame::OnFinalMessage(HWND hWnd)
{
	if (callback_) 
	{
		if (ui_ == STREAMING)
		{
			callback_->DisconnectFromCurrentPeer();
		}
		else
		{
			callback_->DisconnectFromServer();
		}
	}

	if (callback_)
		callback_->Close();

	m_TrayIcon.RemoveIcon();

	WindowImplBase::OnFinalMessage(hWnd);
	delete this;
}

CDuiString MainFrame::GetSkinFile()
{
	return _T("UI.xml");
}

CDuiString MainFrame::GetSkinFolder()
{
	return  _T("skin\\");
}

LRESULT MainFrame::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::ResponseDefaultKeyEvent(WPARAM wParam)
{
	if (wParam == VK_RETURN)
	{
		return FALSE;
	}
	else if (wParam == VK_ESCAPE)
	{
		return TRUE;
	}
	return FALSE;
}

void MainFrame::OnTimer(TNotifyUI& msg)
{
}

void MainFrame::OnExit(TNotifyUI& msg)
{
	Close();
}

void MainFrame::InitWindow()
{
	ui_thread_id_ = ::GetCurrentThreadId();

	pBtnLogin = static_cast<CButtonUI*>(m_PaintManager.FindControl(kBtn_login));

	pBtnVideo = static_cast<CButtonUI*>(m_PaintManager.FindControl(kBtn_video));

	pBtnVideo->SetEnabled(false);

	pCtrlVideo = static_cast<CControlUI*>(m_PaintManager.FindControl(kCtrl_video));

	pEditUser = static_cast<CEditUI*>(m_PaintManager.FindControl(kEdit_user));

	pEditPass = static_cast<CEditUI*>(m_PaintManager.FindControl(kEdit_pass));

	pEditServer = static_cast<CEditUI*>(m_PaintManager.FindControl(kEdit_server));

	pListUsers = static_cast<CListUI*>(m_PaintManager.FindControl(kList_room));
	pListUsers->SetTextCallback(this);//[1]

	doReset();
	doUiChanged();

	HICON hAppIcon = ::LoadIcon(CPaintManagerUI::GetInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
	m_TrayIcon.AddIcon(m_hWnd, WM_TRAYICON_NOTIFY, 1, hAppIcon, _T("一对一音视频"));

//	::SetWindowLongA(*this, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
}

void MainFrame::Notify(TNotifyUI& msg)
{
	if (_tcsicmp(msg.sType, _T("windowinit")) == 0)
	{

	}
	else if (_tcsicmp(msg.sType, _T("killfocus")) == 0)
	{

	}
	else if (_tcsicmp(msg.sType, _T("click")) == 0)
	{
		if (_tcsicmp(msg.pSender->GetName(), kBtn_Close) == 0)
		{
			OnExit(msg);
		}
		else if (_tcsicmp(msg.pSender->GetName(), kBtn_Mini) == 0)
		{
			::ShowWindow(m_hWnd, SW_HIDE);
		}
		else if (_tcsicmp(msg.pSender->GetName(), kBtn_login) == 0)
		{
			doLogin();
		}
		else if (_tcsicmp(msg.pSender->GetName(), kBtn_reset) == 0)
		{
			doReset();
		}
		else if (_tcsicmp(msg.pSender->GetName(), kBtn_video) == 0)
		{
			onBtnVideoClick();
		}
		else if (_tcsicmp(msg.pSender->GetName(), kBtn_EndCall) == 0)
		{
			if (ui_ == STREAMING) {
				callback_->DisconnectFromCurrentPeer();
			}
		}
	}
	else if (msg.sType == _T("itemclick"))
	{
		int id = msg.pSender->GetTag();

		auto it = userInfo_.find(id);
		if (it != userInfo_.end())
		{
			TCHAR szBuf[MAX_PATH] = { 0 };
			_stprintf(szBuf, _T("%d"), id);
			pBtnVideo->SetUserData(szBuf);
			pBtnVideo->SetEnabled(true);
		}
	}
	else if (msg.sType == _T("itemactivate"))
	{
		int id = msg.pSender->GetTag();

		auto it = userInfo_.find(id);
		if (it != userInfo_.end())
		{
			TCHAR szBuf[MAX_PATH] = { 0 };
			_stprintf(szBuf, _T("%d"), id);
			pBtnVideo->SetUserData(szBuf);
			pBtnVideo->SetEnabled(true);
		}

		onBtnVideoClick();
	}
}

LRESULT MainFrame::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	switch (uMsg)
	{
	case WM_TRAYICON_NOTIFY:
	{
		m_TrayIcon.OnTrayIconNotify(wParam, lParam);
		OnTrayMessage(wParam, lParam);
		bHandled = TRUE;
		break;
	}	
	case WM_TIMER:
	{
		OnTimer(wParam, lParam);
		bHandled = TRUE;
		break;
	}	
	case WM_COMMAND:
	{
		OnCommand(wParam, lParam);
		bHandled = false;
		break;
	}	
	case WM_UPDATEIMG:
	{
		OnUpdateImg(wParam, lParam);
		bHandled = false;
		break;
	}
	case  UI_THREAD_CALLBACK:
	{
		callback_->UIThreadCallback(static_cast<int>(wParam),
			reinterpret_cast<void*>(lParam));
		bHandled = false;
		break;
	}
	default:
		break;
	}
	return 0;
}

UILIB_RESOURCETYPE MainFrame::GetResourceType() const
{
	return UILIB_ZIPRESOURCE;
}

LPCTSTR MainFrame::GetResourceID() const
{
	return MAKEINTRESOURCE(IDR_ZIPRES);
}

LPCTSTR MainFrame::GetItemText(CControlUI* pControl, int iIndex, int iSubItem)
{
	UINT32 id = pControl->GetTag();
	TCHAR szBuf[MAX_PATH] = { 0 };
	switch (iSubItem)
	{
	case 0:
		_stprintf(szBuf, _T("%d"), id);
		break;
	case 1:
	{
#ifndef _UNICODE
		AString name = userInfo_[id];
		CString text(name.c_str());
#else
		AString name = userInfo_[id];
		CA2W text(name.c_str());
#endif
		_stprintf(szBuf, text);
	}
	break;
	case 2:
	{
		_stprintf(szBuf, _T("在线"));
	}
	break;
	}
	pControl->SetUserData(szBuf);
	return pControl->GetUserData();
}

LRESULT MainFrame::OnTimer(WPARAM wParam, LPARAM lParam)
{
	m_TrayIcon.OnTimer(wParam);

	return S_OK;
}

//托盘菜单函数
LRESULT MainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_TRAYCLOSE)
	{
		::PostMessage(*this, WM_CLOSE, NULL, NULL);
	}
	else if (wParam == ID_OPENWEB)
	{
		::ShellExecuteA(handle(), "open", WEBURL.c_str(), NULL, NULL, SW_SHOW);
	}
	return 0;
}
//系统托盘函数
LRESULT MainFrame::OnTrayMessage(WPARAM wParam, LPARAM lParam)
{
	if (lParam == WM_LBUTTONDBLCLK)	//双击事件
	{
		::SetWindowPos(*this, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		::ShowWindow(*this, SW_SHOW);
	}
	else if (lParam == WM_RBUTTONDOWN)	//右键单击事件
	{
		HMENU hMenu = ::LoadMenu(CPaintManagerUI::GetInstance(), MAKEINTRESOURCE(IDR_TRAYMENU));
		hMenu = ::GetSubMenu(hMenu, 0);
		CPoint Curpt;
		::GetCursorPos(&Curpt);			//获取鼠标指针位置
		::SetForegroundWindow(*this);	// 修正当用户按下ESCAPE 键或者在菜单之外单击鼠标时菜单不会消失的情况
		::TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, Curpt.x, Curpt.y, 0, this->handle(), NULL);
	}
	return 0;
}

LRESULT MainFrame::OnUpdateImg(WPARAM wParam, LPARAM lParam)
{
	VideoRenderer* local_renderer = local_renderer_.get();
	VideoRenderer* remote_renderer = remote_renderer_.get();

	if (ui_ == STREAMING && (local_renderer || remote_renderer))
	{
		CDuiRect rc = pCtrlVideo->GetPos();
		int BitMapW = rc.GetWidth();
		int BitMapH = rc.GetHeight();
		//获取dc和内存位图
		HDC hdc = m_PaintManager.GetPaintDC();
		HDC dc_mem = ::CreateCompatibleDC(hdc);
		::SetStretchBltMode(dc_mem, HALFTONE);
		HBITMAP bmp_mem = ::CreateCompatibleBitmap(hdc, BitMapW, BitMapH);
		HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

		if (remote_renderer)
		{
			AutoLock<VideoRenderer> remote_lock(remote_renderer);

			const BITMAPINFO& bmi = remote_renderer->bmi();
			int height = abs(bmi.bmiHeader.biHeight);
			int width = bmi.bmiHeader.biWidth;

			const uint8_t* image = remote_renderer->image();
			if (image != NULL)
			{
				if (width*BitMapH > height*BitMapW)
				{
					int reHeight = (float)(BitMapW*height) / width;
					StretchDIBits(dc_mem, 0, (BitMapH - reHeight)*0.5, BitMapW, reHeight,
						0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);
				}
				else
				{
					int reWidth = (float)(BitMapH*width) / height;
					StretchDIBits(dc_mem, (BitMapW - reWidth)*0.5, 0, reWidth, BitMapH,
						0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);
				}
			}
		}

		if (local_renderer)
		{
			AutoLock<VideoRenderer> local_lock(local_renderer);

			const BITMAPINFO& bmi = local_renderer->bmi();
			int height = abs(bmi.bmiHeader.biHeight);
			int width = bmi.bmiHeader.biWidth;

			const uint8_t* image = local_renderer->image();
			if (image != NULL)
			{
				image = local_renderer->image();
				int thumb_width = BitMapW / 3;
				int thumb_height = BitMapH / 3;
				StretchDIBits(dc_mem,
					BitMapW - thumb_width - 10,
					BitMapH - thumb_height - 10,
					thumb_width, thumb_height,
					0, 0, bmi.bmiHeader.biWidth, -bmi.bmiHeader.biHeight,
					image, &bmi, DIB_RGB_COLORS, SRCCOPY);
			}
		}

		BitBlt(hdc, rc.left, rc.top, BitMapW, BitMapH,
			dc_mem, 0, 0, SRCCOPY);

		// Cleanup.
		::SelectObject(dc_mem, bmp_old);
		::DeleteObject(bmp_mem);
		::DeleteDC(dc_mem);
	}
	else
	{
		pCtrlVideo->Invalidate();
	}
	
	return S_OK;
}


void MainFrame::doLogin()
{
	if (pEditUser->GetText().IsEmpty())
	{
		PostNotification("用户名不能为空！");
		return;
	}

	if (pEditPass->GetText().IsEmpty())
	{
		PostNotification("密码不能为空！");
		return;
	}	

#ifndef _UNICODE
	m_strUserName = pEditUser->GetText();
	m_strPassWord = pEditPass->GetText();
#else
	m_strUserName = CW2A(pEditUser->GetText());
	m_strPassWord = CW2A(pEditPass->GetText());
#endif

	AString strUserName = m_strUserName;
	AString strPassWord = m_strPassWord;

// 	const std::regex pattern("^[a-zA-Z]\\w{2,14}$");//以字母开头，长度在3~15之间，只能包含字符、数字和下划线。
// 	if (!std::regex_match(m_strUserName, pattern))
// 	{
// 		PostNotification("用户名长度在3~15之间，只能包含字符、数字和下划线！");
// 		return;
// 	}
// 	const std::regex pattern2("^[a-zA-Z]\\w{5,17}$");//以字母开头，长度在3~15之间，只能包含字符、数字和下划线。
// 	if (!std::regex_match(m_strPassWord, pattern2))
// 	{
// 		PostNotification("密码长度在6~18之间，只能包含字符、数字和下划线！");
// 		return;
// 	}

	MD5 md5;
	md5.reset();
	md5.update((const void *)strPassWord.c_str(), strPassWord.length());
	strPassWord = md5.toString();

	std::transform(strPassWord.begin(), strPassWord.end(), strPassWord.begin(), ::tolower);

	AString server = SERVER;
#ifndef _UNICODE
	AString newServer = pEditServer->GetText();
#else
	AString newServer = CW2A(pEditServer->GetText());
#endif
	
	if (!newServer.empty())
	{
		server = newServer;
	}

	callback_->StartLogin(server,strUserName, strPassWord);

	pBtnLogin->SetEnabled(false);
	pEditUser->SetEnabled(false);
	pEditPass->SetEnabled(false);
	pEditServer->SetEnabled(false);

	PostNotification(_T("正在连接服务器..."));
}

void MainFrame::doReset()
{
#ifndef _UNICODE
	pEditServer->SetText(SERVER);
#else
	pEditServer->SetText(CA2W(SERVER.c_str()));
#endif	
}

void MainFrame::doUiChanged()
{
	for (auto &it : vecLayouts)
	{
		CVerticalLayoutUI *layout = static_cast<CVerticalLayoutUI*>(m_PaintManager.FindControl(it.second));
		if (layout)
		{
			layout->SetVisible(it.first == ui_);
		}
	}

	CLabelUI *label_title = static_cast<CLabelUI*>(m_PaintManager.FindControl(kLabel_Title));
	if (label_title)
	{
		CString strTitle = _T("一对一音视频");
		if (!m_strUserName.empty())
		{
#ifndef _UNICODE
			strTitle += CString(_T(" 当前用户:")) + m_strUserName;
#else
			strTitle += CString(_T(" 当前用户:")) + CA2W(m_strUserName.c_str());
#endif
		}

		label_title->SetText(strTitle);
	}
}

void MainFrame::onBtnVideoClick()
{
	CDuiString strID = pBtnVideo->GetUserData();
	int peer_id = _ttoi(strID);
	if (peer_id != 0)
	{
		callback_->ConnectToPeer(peer_id);
	}
}

void MainFrame::PostNotification(const CString& message)
{
	CLabelUI *label_tip = static_cast<CLabelUI*>(m_PaintManager.FindControl(kLabel_tip));
	if (label_tip)
	{
		label_tip->SetText(message);
	}
}

void MainFrame::PacketBufferFull()
{
	PostNotification("缓冲区已满！");
}

void MainFrame::PacketError(UInt32)
{
	PostNotification("数据包异常！");
}

void MainFrame::PacketUnknown(UInt32)
{
	PostNotification("未知数据！");
}

MainFrame::VideoRenderer::VideoRenderer(
	HWND wnd, int width, int height,
	webrtc::VideoTrackInterface* track_to_render)
	: wnd_(wnd), rendered_track_(track_to_render) {
	::InitializeCriticalSection(&buffer_lock_);
	ZeroMemory(&bmi_, sizeof(bmi_));
	bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi_.bmiHeader.biPlanes = 1;
	bmi_.bmiHeader.biBitCount = 32;
	bmi_.bmiHeader.biCompression = BI_RGB;
	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
	rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

MainFrame::VideoRenderer::~VideoRenderer() {
	rendered_track_->RemoveSink(this);
	::DeleteCriticalSection(&buffer_lock_);
}

void MainFrame::VideoRenderer::SetSize(int width, int height) {
	AutoLock<VideoRenderer> lock(this);

	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
	image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
}

void MainFrame::VideoRenderer::OnFrame(
	const webrtc::VideoFrame& video_frame) {
		{
			AutoLock<VideoRenderer> lock(this);
// 			rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer(
// 				video_frame.video_frame_buffer());
// 			if (video_frame.rotation() != webrtc::kVideoRotation_0) {
// 				buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
// 			}
			SetSize(video_frame.width(), video_frame.height());
			RTC_DCHECK(image_.get() != NULL);
// 			libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(),
// 				buffer->DataU(), buffer->StrideU(),
// 				buffer->DataV(), buffer->StrideV(),
// 				image_.get(),
// 				bmi_.bmiHeader.biWidth *
// 				bmi_.bmiHeader.biBitCount / 8,
// 				buffer->width(), buffer->height());

			webrtc::ConvertFromI420(video_frame, webrtc::VideoType::kARGB, 0, image_.get());

		}
	::PostMessage(wnd_, WM_UPDATEIMG, 0, 0);
}
