// RMsgDrawHWND.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <vector>
#include "../RMsgHWND/PbDrawHWND.pb.h"
#include "Session.h"
#include "Server.h"
#include "utility.h"
#include <Windows.h>


using namespace RMsg;
using namespace std;

class Worker
{
    Session* m_pSession;
    HWND m_hwnd;
    vector<POINT> points;
    bool m_dragging;

public:
    Worker(Session* pSession)
        : m_pSession(pSession)
        , m_hwnd(NULL)
        , m_dragging(false)
    {
    }

    void Process(const Message& msg, const std::shared_ptr<PbHWND>& pPbRequest)
    {
        HWND hwnd = (HWND)pPbRequest->hwnd();
        cout << "PbHWND 0x" << hex << hwnd << dec << endl;
        m_hwnd = hwnd;
    }

    void Process(const Message& msg, const std::shared_ptr<PbDrawFrame>& pPbRequest)
    {
        cout << "PbDrawFrame" << endl;
        PAINTSTRUCT ps;
        //HWND hwnd = (HWND)pPbRequest->hwnd();
        HDC hdc = BeginPaint(m_hwnd, &ps);
        const char* text = "--Draw From another process!!!";
        TextOutA(hdc, 10, 10, text, strlen(text));
        Polyline(hdc, points.data(), points.size());
        EndPaint(m_hwnd, &ps);
    }

    void Process(const Message& msg, const std::shared_ptr<PbMouseEvent>& pPbRequest)
    {
        POINT pt = { pPbRequest->x(), pPbRequest->y() };
        switch (pPbRequest->kind())
        {
        case RMsg::MouseEventType::LDOWN:
        cout << "LBUTTON Down" << endl;
            m_dragging = true;
            points.clear();
            points.push_back(pt);
            break;
        case RMsg::MouseEventType::LUP:
            cout << "LBUTTON UP" << endl;
            m_dragging = false;
            break;
        case RMsg::MouseEventType::MOVE:
            if (m_dragging)
            {
                points.push_back(pt);
                InvalidateRect(m_hwnd, NULL, TRUE);
            }
            break;
        }
    }

    void ProcessFinish(const Message& msg, const std::shared_ptr<PbFinish>& pPbRequest)
    {
        cout << "PbFinish" << endl;
        m_pSession->Disconnect();
    }

    static void OnDisconnect()
    {
        std::cout << "Socket disconnected." << std::endl;
    }
};

int main()
{
    // Disable to improve performance, Enable to debug communication error.
    SetDebugInfo("RMsgDrawHWND.txt");
    EnableDebugInfo(true);
    Session s;
    s.RegisterDisconnect(&Worker::OnDisconnect);
    Worker w(&s);
    s.RegisterMessageHandler<Worker, PbHWND>(&Worker::Process, &w);
    s.RegisterMessageHandler<Worker, PbDrawFrame>(&Worker::Process, &w);
    s.RegisterMessageHandler<Worker, PbMouseEvent>(&Worker::Process, &w);
    s.RegisterMessageHandler(&Worker::ProcessFinish, &w);
    s.Connect("127.0.0.1", 9876);
    s.Run();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
