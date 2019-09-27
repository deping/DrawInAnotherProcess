// RMsgHWND.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "RMsgHWND.h"
#include "PbDrawHWND.pb.h"
#include "Session.h"
#include "Server.h"
#include "utility.h"
#include <Windowsx.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RMSGHWND, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RMSGHWND));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RMSGHWND));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RMSGHWND);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void StartDrawProcess(HWND hWnd);
//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   StartDrawProcess(hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

RMsg::Session s;
RMsg::Server ser;
std::thread msgThread;
bool connected = false;
void StartDrawProcess(HWND hWnd)
{
    msgThread = std::thread([&, hWnd]() {
        // Disable to improve performance, Enable to debug communication error.
        RMsg::SetDebugInfo("RMsgHWND.txt");
        RMsg::EnableDebugInfo(true);
        s.RegisterConnect([&, hWnd]() {
            RMsg::PbHWND pbHwnd;
            auto value = google::protobuf::int64(intptr_t(hWnd));
            //char buf[48] = "HWND=0x";
            //_itoa_s(value, buf + strlen(buf),sizeof(buf), 16);
            //MessageBoxA(NULL, buf, "", MB_OK);
            pbHwnd.set_hwnd(value);
            s.EnqueuePbNotice(pbHwnd);
            connected = true;
        });
        s.RegisterDisconnect([&]() {
            connected = false;
            // This will stop this session
            s.Stop();
        });
        // TODO: ���ܹ�Ӳ����˿ں�
        ser.Listen(0, s);

        //if (connected)
            s.RunForever();
    });

    // Wait for msgThread to listen, so the port is effective.
    while (!ser.listened())
    {
        SwitchToThread();
    }

    char RMsgDrawHWND[MAX_PATH + 128];
    *RMsgDrawHWND = '\"';
    GetModuleFileNameA(NULL, RMsgDrawHWND+1, MAX_PATH);
    char* p = strrchr(RMsgDrawHWND, '\\');
    *p = 0;
    strcat_s(RMsgDrawHWND, "\\RMsgDrawHWND.exe\" ");
    auto len = strlen(RMsgDrawHWND);
    _itoa_s(ser.port(), RMsgDrawHWND + len, sizeof(RMsgDrawHWND) - len, 10);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    //����һ���½���  
    if (CreateProcessA(
        NULL,
        RMsgDrawHWND,
        NULL, //    ָ��һ��SECURITY_ATTRIBUTES�ṹ�壬����ṹ������Ƿ񷵻صľ�����Ա��ӽ��̼̳С�  
        NULL, //    ���lpProcessAttributes����Ϊ�գ�NULL������ô������ܱ��̳С�<ͬ��>  
        false,//    ָʾ�½����Ƿ�ӵ��ý��̴��̳��˾����   
        0,  //  ָ�����ӵġ���������������ͽ��̵Ĵ����ı�  
            //  CREATE_NEW_CONSOLE  �¿���̨���ӽ���  
            //  CREATE_SUSPENDED    �ӽ��̴��������ֱ������ResumeThread����  
        NULL, //    ָ��һ���½��̵Ļ����顣����˲���Ϊ�գ��½���ʹ�õ��ý��̵Ļ���  
        NULL, //    ָ���ӽ��̵Ĺ���·��  
        &si, // �����½��̵������������ʾ��STARTUPINFO�ṹ��  
        &pi  // �����½��̵�ʶ����Ϣ��PROCESS_INFORMATION�ṹ��  
    ))
    {
        //�������йرվ������������̺��½��̵Ĺ�ϵ����Ȼ�п��ܲ�С�ĵ���TerminateProcess�����ص��ӽ���  
      CloseHandle(pi.hProcess);  
      CloseHandle(pi.hThread);  
    }
    else {
        // failed to create process
        s.Stop();
    }
}

void SendMouseEvent(RMsg::MouseEventType type, LPARAM lp)
{
    RMsg::PbMouseEvent pbME;
    pbME.set_kind(type);
    pbME.set_x(GET_X_LPARAM(lp));
    pbME.set_y(GET_Y_LPARAM(lp));
    s.EnqueuePbNotice(pbME);
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            if (!connected)
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                const char* text = "Draw by SELF";
                TextOutA(hdc, 10, 10, text, strlen(text));
                EndPaint(hWnd, &ps);
            }
            else
            {
                RMsg::PbDrawFrame pbDraw;
                pbDraw.set_hwnd(google::protobuf::int64(hWnd));
                s.EnqueuePbNotice(pbDraw);
            }
        }
        break;
    case WM_LBUTTONDOWN:
        if (connected)
        {
            SendMouseEvent(RMsg::MouseEventType::LDOWN, lParam);
        }
        break;
    case WM_LBUTTONUP:
        if (connected)
        {
            SendMouseEvent(RMsg::MouseEventType::LUP, lParam);
        }
        break;
    case WM_MOUSEMOVE:
        if (connected)
        {
            SendMouseEvent(RMsg::MouseEventType::MOVE, lParam);
        }
        break;
    case WM_DESTROY:
    {
        if (connected)
        {
            connected = false;
            RMsg::PbFinish pbFinish;
            s.EnqueuePbNotice(pbFinish);
            msgThread.join();
            PostQuitMessage(0);
        }
    }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
