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

DECLARE_COMPONENT_VERSION(
"DInputCtrl",
"0.0.1",
"DirectInput Contrllor\n"
);
VALIDATE_COMPONENT_FILENAME("foo_DInputCtrl.dll");

//全局变量
LPDIRECTINPUT8         pDI = 0;//DirectInput
IDirectInputDevice8* pDIDeviceWC = 0;//DirectInput Wireless Controller
DIJOYSTATE2 DIStateWC[2];//DirectInput Wireless Controller
HWND m_hwnd;
int DIStateSwitch=0;
HANDLE hThread = 0;


//前向声明
DWORD WINAPI DICtrlMain(LPVOID pParam);
bool GetKey(int myKeycode);
bool GetKeyDown(int myKeycode);

class dictrl_initquit : public initquit {
public:
	void on_init() {
		m_hwnd = GetActiveWindow();
		if(!hThread)hThread=CreateThread(0,0,DICtrlMain,0,0,0);
	}
	void on_quit() {
	}
};

static initquit_factory_t<dictrl_initquit> initquit_factory;

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
		default:
			break;
		}
	}
	playback_ctrl(int cmd) 
	{
		this->cmd = cmd;
	}
};

//see main_thread_callback
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

DWORD WINAPI DICtrlMain(LPVOID pParam) 
{
	HRESULT hr;
	if (FAILED(hr=DirectInput8Create(GetModuleHandle(NULL), 0x0800, IID_IDirectInput8, (void**)&pDI, NULL)))
		return FALSE;
	if (FAILED(hr=pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, DIEnumDevicesCallback, 0, DIEDFL_ATTACHEDONLY)))
		return FALSE;

	if (pDIDeviceWC)
	{
		hr=pDIDeviceWC->SetDataFormat(&c_dfDIJoystick2);
		hr=pDIDeviceWC->SetCooperativeLevel(m_hwnd, DISCL_BACKGROUND| DISCL_NONEXCLUSIVE);
		popup_message::g_show("Successfully got a contoller.", "DInputCtrl");
		while (TRUE)
		{
			static_api_ptr_t<main_thread_callback_manager> foo_m;
			hr = pDIDeviceWC->Acquire();
			hr = pDIDeviceWC->GetDeviceState(sizeof(DIJOYSTATE2), &DIStateWC[DIStateSwitch]);
			DIStateSwitch ^= 1;

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
			if (GetKeyDown(Keycode_Button_Up)) 
			{
				foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Vol_Up));
			}
			if (GetKeyDown(Keycode_Button_Down))
			{
				foo_m->add_callback(new service_impl_t<playback_ctrl>(Playback_Vol_Down));
			}

		}
	}
	return 0;
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