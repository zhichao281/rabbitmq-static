// mqclient.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../BaseSignal.h"

#ifdef _DEBUG
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"E:/study/rabbitmq-static/lib/Debug/librabbitmq.lib")
#else
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"E:/study/rabbitmq-static/lib/Release/librabbitmq.lib")
#endif
#include <string>
#include <iostream>
using namespace  std;

static void onMessage(string str1, string  str2)
{

	cout <<str2 << endl;;

}


int main()
{
	string str1 = "中文";
	cout << "a中文";
	BaseSignal sigal1;
	sigal1.Create("180.97.246.16",6841, "vpclient_test+wsrtc.vpclient_test.com+dwk001", "1524051262655_6f1c9c55c1a9edd17995a7a07131e01e0871289f","wsrtc.vpclient_test.com");
	sigal1.joinSignalChannel("wssig://wsrtc.vpclient_test.com/vpclient_test/dwk", "dwk102", onMessage);
	BaseSignal sendSignal;
	sendSignal.Create("180.97.246.16", 6841, "vpclient_test+wsrtc.vpclient_test.com+dwk001", "1524051262655_6f1c9c55c1a9edd17995a7a07131e01e0871289f", "wsrtc.vpclient_test.com");

	while (1)
	{
		int i;
		cin >> i;

	
		sendSignal.sendMessage("wssig://wsrtc.vpclient_test.com/vpclient_test/dwk", "dwk000", "{\"from\":\"dwk000\",\"to\":\"\",\"type\":1,\"content\":\"test\"}");
		system("pause");
	}


    return 0;
}

