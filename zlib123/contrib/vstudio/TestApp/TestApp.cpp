// TestApp.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>
#include "..\ZipLib\ZipLib.h"


int _tmain(int argc, _TCHAR* argv[])
{
	UnZipFilesToDir("E:\\Zlib\\212123.zip","E:\\Zlib\\212123",0);
	CompressDirToZip("E:\\Zlib\\212123","E:\\Zlib\\1.zip",0);
	return 0;
}

