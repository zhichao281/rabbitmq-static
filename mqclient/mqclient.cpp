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
	BaseSignal sigal1;
	sigal1.Create();
	sigal1.joinSignalChannel("wssig://wsrtc.vpclient_test.com/vpclient_test/zzc001", "zzc001", onMessage);


	sigal1.quitSignalChannel();

	while (1)
	{
		int i;
		cin >> i;
		BaseSignal sigal2;
		sigal2.Create();
		sigal1.sendMessage("wssig://wsrtc.vpclient_test.com/vpclient_test/zzc001", "zzc001", "test");
		sigal2.quitSignalChannel();
		system("pause");
	}


    return 0;
}

