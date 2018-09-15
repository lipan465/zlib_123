#ifndef _H_FF60B39C_AA1E_46F4_9B79_C72947ECCDBF_H
#define _H_FF60B39C_AA1E_46F4_9B79_C72947ECCDBF_H

#ifndef AFX_ZIPLIB_API
#define AFX_ZIPLIB_API __declspec(dllimport) 
#endif

#ifdef __cplusplus
extern "C" {
#endif
	AFX_ZIPLIB_API BOOL CompressDirToZipA(const char* lpszSrcPath, const char* lpszZipName, const char* lpszPassword);
	AFX_ZIPLIB_API BOOL CompressDirToZipW(const wchar_t* lpszSrcPath, const wchar_t* lpszZipName, const wchar_t* lpszPassword);

	AFX_ZIPLIB_API BOOL UnZipFilesToDirA(const char* lpszZipName, const char* lpszDesDir, const char* lpszPassword);
	AFX_ZIPLIB_API BOOL UnZipFilesToDirW(const wchar_t* lpszZipName, const wchar_t* lpszDesDir, const wchar_t* lpszPassword);

#ifdef UNICODE
#define CompressDirToZip CompressDirToZipW
#define UnZipFilesToDir UnZipFilesToDirW
#else
#define CompressDirToZip CompressDirToZipA
#define UnZipFilesToDir UnZipFilesToDirA
#endif

#ifdef __cplusplus
}
#endif


#endif