// foo_ostlooper.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "resource.h"

/*
检查DIJOYSTATE.rgdwPOV[0]以确认十字键状态
以上为0顺时针增加角度，（4500一档
*/
#define Keycode_Button_Square 0
#define Keycode_Button_Cross 1 
#define Keycode_Button_Circle 2
#define Keycode_Button_Triangle 3
#define Keycode_Button_L1 4
#define Keycode_Button_R1 5
#define Keycode_Button_L2 6
#define Keycode_Button_R2 7
#define Keycode_Button_Share 8
#define Keycode_Button_Options 9
#define Keycode_Button_L3 10
#define Keycode_Button_R3 11
#define Keycode_Button_Home 12
#define Keycode_Button_Pad 13
#define Keycode_Button_Up 4500
#define Keycode_Button_Right 4502
#define Keycode_Button_Down 4504
#define Keycode_Button_Left 4506

#define Playback_Start_or_Pause  8
#define Playback_Stop  9
#define Playback_Next  10
#define Playback_Prev  11
#define Playback_Rand  12
#define Playback_Vol_Up  13
#define Playback_Vol_Down  14
#define Playback_Kick  15

DECLARE_COMPONENT_VERSION(
"DInputCtrl",
"0.0.3",
"DirectInput Contrllor\n"
);
VALIDATE_COMPONENT_FILENAME("foo_DInputCtrl.dll");

//全局变量
LPDIRECTINPUT8         pDI = 0;//DirectInput
IDirectInputDevice8* pDIDeviceWC = 0;//DirectInput Wireless Controller
DIJOYSTATE2 DIStateWC[2];//DirectInput Wireless Controller
HWND m_hwnd;
MMRESULT timerID=0;
int DIStateSwitch=0;
LARGE_INTEGER queryTimeFre;

//前向声明
bool GetKey(int myKeycode);
bool GetKeyDown(int myKeycode);
void Start();
void CALLBACK Update(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
template<class Interface>
inline void SafeRelease(
	Interface **ppInterfaceToRelease
)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

class dictrl_initquit : public initquit {
public:
	void on_init() {
		Start();
	}
	void on_quit() {
	}
};
static initquit_factory_t<dictrl_initquit> initquit_factory;

class dictrl_mainmenu_commands: public mainmenu_commands {
public:

	t_uint32 get_command_count() {
		return 1;
	}
	GUID get_command(t_uint32 p_index) {
		static const GUID guid_refresh= { 0xb1fe4853, 0x3dda, 0x4666,{ 0xb1, 0x46, 0x57, 0x3b, 0x9a, 0xd8, 0x74, 0xd7 } };
		switch (p_index) {
		case 0: return guid_refresh;
		default: uBugCheck();
		}
	}
	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case 0: p_out = "Refresh DInputCtrl"; break;
		default: uBugCheck(); 
		}
	}
	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case 0: p_out = "Restart the DInputCtrl thread to get a device."; return true;
		default: uBugCheck(); 
		}
	}
	GUID get_parent() {
		return mainmenu_groups::playback;
	}
	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		switch (p_index) {
		case 0:
		{
			Start();
		}
			break;

		default:
			uBugCheck(); 
		}
	}
};
static mainmenu_commands_factory_t<dictrl_mainmenu_commands> g_mainmenu_commands_factory;

class playback_ctrl :public main_thread_callback {
public:
	static_api_ptr_t<playback_control> m_playback_control;
	int cmd;
	void callback_run()
	{
		switch (cmd)
		{
		case Playback_Next:
			m_playback_control->start(playback_control::track_command_next); break;
		case Playback_Prev:
			m_playback_control->start(playback_control::track_command_prev); break;
		case Playback_Rand:
			m_playback_control->start(playback_control::track_command_rand); break;
		case Playback_Stop:
			m_playback_control->stop(); break;
		case Playback_Start_or_Pause:
			m_playback_control->play_or_pause(); break;
		case Playback_Vol_Up:
			m_playback_control->volume_up(); break;
		case Playback_Vol_Down:
			m_playback_control->volume_down(); break;
		case Playback_Kick:
		{
			metadb_handle_ptr target;
			static_api_ptr_t<playlist_manager>m_playlist;
			t_size playlist; t_size location;
			if (m_playlist->get_playing_item_location(&playlist, &location))
			{
				m_playlist->playlist_remove_items(playlist,bit_array_one(location));
				m_playback_control->next();
			}
		}
		break;
		default:
			break;
		}
	}
	playback_ctrl(int cmd) 
	{
		this->cmd = cmd;
	}
};

BOOL CALLBACK DIEnumDevicesCallback(
	LPCDIDEVICEINSTANCE lpddi,
	LPVOID pvRef
)
{
	if (SUCCEEDED(pDI->CreateDevice(lpddi->guidInstance, &pDIDeviceWC, 0)))
	{
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

void Start() 
{
	HRESULT hr;
	if (timerID)
	{
		timeKillEvent(timerID);
		timerID = 0;
		SafeRelease(&pDIDeviceWC);
		SafeRelease(&pDI);
	}
	QueryPerformanceFrequency(&queryTimeFre);
	m_hwnd = GetActiveWindow();
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), 0x0800, IID_IDirectInput8, (void**)&pDI, NULL)))
		return ;
	if (FAILED(hr = pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, DIEnumDevicesCallback, 0, DIEDFL_ATTACHEDONLY)))
		return ;

	if (pDIDeviceWC)
	{
		hr = pDIDeviceWC->SetDataFormat(&c_dfDIJoystick2);
		hr = pDIDeviceWC->SetCooperativeLevel(m_hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
		if (FAILED(hr))return;
		popup_message::g_show("Successfully got a contoller.", "DInputCtrl");
		timerID= timeSetEvent(100,50,Update,0, TIME_PERIODIC);
	}
}

void CALLBACK Update(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	HRESULT hr;
	static static_api_ptr_t<main_thread_callback_manager> foo_m;
	DIStateSwitch ^= 1;
	hr = pDIDeviceWC->Acquire();
	hr = pDIDeviceWC->GetDeviceState(sizeof(DIJOYSTATE2), &DIStateWC[DIStateSwitch]);

	if (GetKeyDown(Keycode_Button_R1))
	{
		foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Next));
	}
	if (GetKeyDown(Keycode_Button_L1))
	{
		foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Prev));
	}
	if (GetKeyDown(Keycode_Button_Home))
	{
		foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Stop));
	}
	if (GetKeyDown(Keycode_Button_Circle))
	{
		foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Start_or_Pause));
	}

	//音量部分
	static int volChangeFre = 2;
	static bool checkButtonUp = false;
	static LARGE_INTEGER checkButtonUpTime = { 0 };
	if (GetKeyDown(Keycode_Button_Up))
	{
		foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Vol_Up));
		QueryPerformanceCounter(&checkButtonUpTime);
		checkButtonUp = true;
	}
	if (checkButtonUp)
	{
		if (GetKey(Keycode_Button_Up))
		{
			LARGE_INTEGER tempTime;
			QueryPerformanceCounter(&tempTime);
			if (tempTime.QuadPart - checkButtonUpTime.QuadPart > queryTimeFre.QuadPart / volChangeFre)
			{
				foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Vol_Up));
				checkButtonUpTime = tempTime;
				volChangeFre++;
			}
		}
		else
		{
			checkButtonUp = false;
			volChangeFre = 2;
		}
	}
	static bool checkButtonDown = false;
	static LARGE_INTEGER checkButtonDownTime = { 0 };
	if (GetKeyDown(Keycode_Button_Down))
	{
		foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Vol_Down));
		QueryPerformanceCounter(&checkButtonDownTime);
		checkButtonDown = true;
	}
	if (checkButtonDown)
	{
		if (GetKey(Keycode_Button_Down))
		{
			LARGE_INTEGER tempTime;
			QueryPerformanceCounter(&tempTime);
			if (tempTime.QuadPart - checkButtonDownTime.QuadPart > queryTimeFre.QuadPart / volChangeFre)
			{
				foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Vol_Down));
				checkButtonDownTime = tempTime;
				volChangeFre++;
			}
		}
		else
		{
			volChangeFre = 2;
			checkButtonDown = false;
		}
	}
	//双击踢歌
	if (GetKeyDown(Keycode_Button_Triangle))
	{
		static LARGE_INTEGER lastDownTime = { 0 }, downTime = { 0 };
#define PRESS_TIME 1
		if (downTime.QuadPart == 0)QueryPerformanceCounter(&downTime);
		else
		{
			lastDownTime = downTime;
			QueryPerformanceCounter(&downTime);
			if ((downTime.QuadPart - lastDownTime.QuadPart)<queryTimeFre.QuadPart*PRESS_TIME)
			{
				foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Kick));
			}
		}
	}
}

bool GetKey(int myKeycode)
{

	if (myKeycode >= 4500)
	{
		if (DIStateWC[DIStateSwitch].rgdwPOV[0] == (myKeycode - 4500) * 4500
			|| DIStateWC[DIStateSwitch].rgdwPOV[0] == ((myKeycode - 4500 + 1) & 7) * 4500
			|| DIStateWC[DIStateSwitch].rgdwPOV[0] == ((myKeycode - 4500 - 1) & 7) * 4500
			)
			return true;
		else
			return false;
	}
	if (DIStateWC[DIStateSwitch].rgbButtons[myKeycode])
		return true;


	return  false;
}
bool GetKeyDown(int myKeycode)
{
	bool r = false;
	if (GetKey(myKeycode)) 
	{
		DIStateSwitch ^= 1;
		if (GetKey(myKeycode))  
			r= false;
		else
			r = true;
		DIStateSwitch ^= 1;
	}
	return r;
}