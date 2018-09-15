// ZipLib.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <Shlwapi.h>
#include <Shlobj.h>
#include <atlconv.h>
#include <stdio.h>
#include <string>
#include "unzip.h"
#include "zip.h"
#include "iowin32.h"
#include "ZipLib.h"

#pragma comment( lib, "Shlwapi.lib" )

/* calculate the CRC32 of a file,
   because to encrypt a file, we need known the CRC32 of the file before */
int getFileCrc(const char* filenameinzip,void*buf,unsigned long size_buf,unsigned long* result_crc)
{
   unsigned long calculate_crc=0;
   int err=ZIP_OK;
   FILE * fin = fopen(filenameinzip,"rb");
   unsigned long size_read = 0;
   unsigned long total_read = 0;
   if (fin==NULL)
   {
       err = ZIP_ERRNO;
   }
   if (err == ZIP_OK)
   {
        do
        {
            err = ZIP_OK;
            size_read = (int)fread(buf,1,size_buf,fin);
            if (size_read < size_buf)
			{
                if (feof(fin)==0)
				{
					err = ZIP_ERRNO;
				}
			}

            if (size_read>0)
			{
                calculate_crc = crc32(calculate_crc,(const Bytef *)buf,size_read);
			}
            total_read += size_read;

        } while ((err == ZIP_OK) && (size_read>0));
	}
    if (fin) fclose(fin);

    *result_crc=calculate_crc;
    return err;
}

uLong filetime(
const char *f,               /* name of file to get info on */
tm_zip *tmzip,             /* return value: access, modific. and creation times */
uLong* attr)             /* dostime */
{
	int ret = 0;
	{
		FILETIME ftLocal;
		HANDLE hFind;
		WIN32_FIND_DATA  ff32;
		SYSTEMTIME st;

		hFind = FindFirstFile(f,&ff32);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			*attr = ff32.dwFileAttributes;
			FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
			FileTimeToSystemTime(&ftLocal,&st);
			tmzip->tm_sec  = st.wSecond;
			tmzip->tm_min  = st.wMinute;
			tmzip->tm_hour = st.wHour;
			tmzip->tm_mday = st.wDay;
			tmzip->tm_mon  = st.wMonth;
			tmzip->tm_year = st.wYear;
			//FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
			FindClose(hFind);
			ret = 1;
		}
	}
	return ret;
}

bool AddfiletoZip(zipFile zfile, const char* fileNameinZip, const char* srcfile,const char* lpszPassword)
{
	if (NULL == zfile || 0 == strlen(fileNameinZip)/* || srcfile.empty()为空代表空目录*/)
	{
		return 0;
	}

	int nErr = 0;
	int opt_compress_level = Z_DEFAULT_COMPRESSION;
	tm_zip tmz = { 0 };
	zip_fileinfo zinfo = {0};
	unsigned long crcFile=0;
	char tmpBuf[16*1024] = {0};

	zinfo.tmz_date = tmz;
	zinfo.dosDate = 0;
	zinfo.internal_fa = 0;
	zinfo.external_fa = 0;

	filetime(srcfile,&zinfo.tmz_date,&zinfo.external_fa);

	char sznewfileName[MAX_PATH] = { 0 };
	memset(sznewfileName, 0x00, sizeof(sznewfileName));
	strcat_s(sznewfileName, fileNameinZip);

	if (0 == strlen(srcfile))
	{
		strcat_s(sznewfileName, "\\");
	}

	if (lpszPassword != NULL)
	{
		nErr = getFileCrc(fileNameinZip,tmpBuf,sizeof(tmpBuf),&crcFile);
		if(ZIP_OK != nErr) return false;
		nErr = zipOpenNewFileInZip3(zfile,sznewfileName,&zinfo,
			NULL,0,NULL,0,NULL /* comment*/,
			(opt_compress_level != 0) ? Z_DEFLATED : 0,
			opt_compress_level,0,
			/* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
			-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
			lpszPassword,crcFile);
	}
	else
	{
		nErr = zipOpenNewFileInZip(zfile, sznewfileName, &zinfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
	}

	if (nErr != ZIP_OK)
	{
		return false;
	}

	if (0 == ( zinfo.external_fa & FILE_ATTRIBUTE_DIRECTORY ))
	{
		//打开源文件
		FILE* srcfp;
		fopen_s(&srcfp,srcfile, "rb");
		if (NULL == srcfp)
		{
			zipCloseFileInZip(zfile);
			return false;
		}
		//读入源文件写入zip文件
		int numBytes = 0;
		while (!feof(srcfp))
		{
			memset(tmpBuf, 0x00, sizeof(tmpBuf));
			numBytes = fread(tmpBuf, 1, sizeof(tmpBuf), srcfp);
			nErr = zipWriteInFileInZip(zfile, tmpBuf, numBytes);
			if (ferror(srcfp))
			{
				break;
			}
		}
		fclose(srcfp);
	}
	zipCloseFileInZip(zfile);
	return true;
}

BOOL ListFileToZip(const char* lpszSrcPath,const char* lpszZipPath,zipFile zf,const char* lpszPassword)
{
	char srcPath[MAX_PATH]={0};
	char zipPath[MAX_PATH]={0};
	int err;
	strcat_s(srcPath,lpszSrcPath);
	strcat_s(srcPath,"\\*.*");
	WIN32_FIND_DATA fileData;
	HANDLE file = FindFirstFile(srcPath, &fileData);
	if(file == INVALID_HANDLE_VALUE) return FALSE;
	while (FindNextFile(file, &fileData))
	{
		if( strcmp(fileData.cFileName,"..") == 0 || strcmp(fileData.cFileName,".") == 0 )continue;
		if( fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			sprintf_s(srcPath,"%s\\%s",lpszSrcPath,fileData.cFileName);
			if(strlen(lpszZipPath) == 0)
			{
				sprintf_s(zipPath,"%s",fileData.cFileName);
			}
			else
			{
				sprintf_s(zipPath,"%s\\%s",lpszZipPath,fileData.cFileName);
			}
			if(false == AddfiletoZip(zf,zipPath,srcPath,lpszPassword)) return false;
			if(!ListFileToZip(srcPath,zipPath,zf,lpszPassword)) return FALSE;
		}
		else
		{
			sprintf_s(srcPath,"%s\\%s",lpszSrcPath,fileData.cFileName);
			if(strlen(lpszZipPath) == 0)
			{
				sprintf_s(zipPath,"%s",fileData.cFileName);
			}
			else
			{
				sprintf_s(zipPath,"%s\\%s",lpszZipPath,fileData.cFileName);
			}
			//printf("%s\\%s\n",lpszSrcPath,fileData.cFileName);
			if(false == AddfiletoZip(zf,zipPath,srcPath,lpszPassword)) return false;
		}
	}
	return TRUE;
}

BOOL CompressDirToZipW(const wchar_t* lpszSrcPath, const wchar_t* lpszZipName, const wchar_t* lpszPassword)
{
	USES_CONVERSION;
	return CompressDirToZipA(W2A(lpszSrcPath),W2A(lpszZipName),W2A(lpszPassword));
}

BOOL CompressDirToZipA(const char* lpszSrcPath, const char* lpszZipName, const char* lpszPassword)
{
	char desDir[MAX_PATH]={0};
	char tempDir[MAX_PATH]={0};
	strcpy_s(desDir,lpszZipName);
	char *p = strrchr(desDir,'\\');*p=0;
	if(!PathFileExists(desDir) )
	{
		if( ERROR_SUCCESS != SHCreateDirectoryEx(NULL, desDir ,NULL) ) return FALSE;
	}

	GetTempPath(MAX_PATH,tempDir);
	GetTempFileName(tempDir,"ZIP",0,tempDir);

	zipFile zf;
	int errclose;
	zlib_filefunc_def ffunc;
	fill_win32_filefunc(&ffunc);
	zf = zipOpen2(tempDir,APPEND_STATUS_CREATE,NULL,&ffunc);

	if(!ListFileToZip(lpszSrcPath,"",zf,lpszPassword))
	{
		zipClose(zf,NULL);
		return FALSE;
	}

	zipClose(zf,NULL);

	if(!MoveFileEx(tempDir,lpszZipName,MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED))return FALSE;
	return TRUE;
}

BOOL UnZipFilesToDirA(const char* lpszZipName, const char* lpszDesDir, const char* lpszPassword)
{
	if(!PathFileExists(lpszDesDir) )
	{
		if( ERROR_SUCCESS != SHCreateDirectoryEx(NULL, lpszDesDir ,NULL) ) return FALSE;
	}
	unzFile uf=NULL;
	zlib_filefunc_def ffunc;
	int err=0;
	unz_global_info gi;
	char buf[8192] = {0};

	fill_win32_filefunc(&ffunc);
	uf = unzOpen2(lpszZipName,&ffunc);
	if(!uf)return FALSE;

	err = unzGetGlobalInfo (uf,&gi);
	if (err!=UNZ_OK)
	{
		goto FAIL;
	}
	for (int i=0;i<gi.number_entry;i++)
	{
		unz_file_info file_info;
		uLong ratio=0;
		char filename_inzip[256];
		char filename[256];
		char dirname[256];
		char *filename_withoutpath,*p,*q;
		FILE* fp = NULL;
		err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
		if (err!=UNZ_OK) goto FAIL;

		filename_withoutpath = p = filename_inzip;
		q = dirname;
		while(*p != 0)
		{
			if (((*p)=='/') || ((*p)=='\\'))
			{
				filename_withoutpath = p+1;
				*q=0;
				sprintf_s(filename,"%s\\%s",lpszDesDir,dirname);
				if(!PathFileExists(filename)) CreateDirectory(filename,0);
			}
			*q++ = *p++;
		}
		if( 0 == ( file_info.external_fa & FILE_ATTRIBUTE_DIRECTORY ) || *filename_withoutpath != '\0')
		{
			sprintf_s(filename,"%s\\%s",lpszDesDir,filename_inzip);
			fopen_s(&fp,filename,"wb");
			if(!fp) goto FAIL;

			err = unzOpenCurrentFilePassword(uf,lpszPassword);
			if (err!=UNZ_OK) goto FAIL;

			do{
				err = unzReadCurrentFile(uf,buf,sizeof(buf));
				if (err<0)
				{
					unzCloseCurrentFile (uf);
					goto FAIL;
				}
				if (err>0)
				{
					if (fwrite(buf,err,1,fp)!=1)
					{
						unzCloseCurrentFile (uf);
						goto FAIL;
					}
				}
			}while(err>0);
			fclose(fp);

			err = unzCloseCurrentFile (uf);
			if (err!=UNZ_OK) goto FAIL;
		}

		if ( (i+1) <gi.number_entry)
		{
			err = unzGoToNextFile(uf);
			if (err!=UNZ_OK) goto FAIL;
		}
	}
	unzClose(uf);
	return true;
FAIL:
	unzClose(uf);
	return FALSE;
}

BOOL UnZipFilesToDirW(const wchar_t* lpszZipName, const wchar_t* lpszDesDir, const wchar_t* lpszPassword)
{
	USES_CONVERSION;
	return UnZipFilesToDirA(W2A(lpszZipName),W2A(lpszDesDir),W2A(lpszPassword));
}