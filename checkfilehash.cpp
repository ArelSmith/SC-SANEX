#include "main.h"
#include "vendor/hash/md5.h"
#include "CheckFileHash.h"

//#define GETFILESUM

bool CheckFile(mdFile* mdChkFile);

bool FileCheckSum()
{
	mdFile *mdCheckSumFile = new mdFile;

	// main.scm - 0 929787706 2474560731 774894726
	// script.img - 0 3843308668 4012600942 3327767609
	// vehicles.ide - 0 401823104 3800320153 1955768670
	// handling.cfg - 0 483133070 2164808380 3547226580
	// peds.ide - 0 3877381984 8810720 773282059
	// gta.dat - 0 3040247056 3529725244 1158560601
	// weapon.dat - 0 1112515121 870911676 3587992568

	sprintf(mdCheckSumFile->szFileLocation, "%sSAMP/main.scm", g_pszStorage);
	mdCheckSumFile->iCorrectDigestArray[0] = 0;
	mdCheckSumFile->iCorrectDigestArray[1] = 929787706;
	mdCheckSumFile->iCorrectDigestArray[2] = 2474560731;
	mdCheckSumFile->iCorrectDigestArray[3] = 774894726;
	#ifdef GETFILESUM
	CheckFile(mdCheckSumFile);
	#else
	if(!CheckFile(mdCheckSumFile)) return false;
	#endif

	sprintf(mdCheckSumFile->szFileLocation, "%sSAMP/script.img", g_pszStorage);
	mdCheckSumFile->iCorrectDigestArray[0] = 0;
	mdCheckSumFile->iCorrectDigestArray[1] = 3843308668;
	mdCheckSumFile->iCorrectDigestArray[2] = 4012600942;
	mdCheckSumFile->iCorrectDigestArray[3] = 3327767609;
	#ifdef GETFILESUM
	CheckFile(mdCheckSumFile);
	#else
	if(!CheckFile(mdCheckSumFile)) return false;
	#endif

	sprintf(mdCheckSumFile->szFileLocation, "%sSAMP/vehicles.ide", g_pszStorage);
	mdCheckSumFile->iCorrectDigestArray[0] = 0;
	mdCheckSumFile->iCorrectDigestArray[1] = 401823104;
	mdCheckSumFile->iCorrectDigestArray[2] = 3800320153;
	mdCheckSumFile->iCorrectDigestArray[3] = 1955768670;
	#ifdef GETFILESUM
	CheckFile(mdCheckSumFile);
	#else
	if(!CheckFile(mdCheckSumFile)) return false;
	#endif

	/*sprintf(mdCheckSumFile->szFileLocation, "%sSAMP/handling.cfg", g_pszStorage);
	mdCheckSumFile->iCorrectDigestArray[0] = 0;
	mdCheckSumFile->iCorrectDigestArray[1] = 483133070;
	mdCheckSumFile->iCorrectDigestArray[2] = 2164808380;
	mdCheckSumFile->iCorrectDigestArray[3] = 3547226580;
	#ifdef GETFILESUM
	CheckFile(mdCheckSumFile);
	#else
	if(!CheckFile(mdCheckSumFile)) return false;
	#endif*/

	sprintf(mdCheckSumFile->szFileLocation, "%sSAMP/peds.ide", g_pszStorage);
	mdCheckSumFile->iCorrectDigestArray[0] = 0;
	mdCheckSumFile->iCorrectDigestArray[1] = 3877381984;
	mdCheckSumFile->iCorrectDigestArray[2] = 8810720;
	mdCheckSumFile->iCorrectDigestArray[3] = 773282059;
	#ifdef GETFILESUM
	CheckFile(mdCheckSumFile);
	#else
	if(!CheckFile(mdCheckSumFile)) return false;
	#endif

	sprintf(mdCheckSumFile->szFileLocation, "%sSAMP/gta.dat", g_pszStorage);
	mdCheckSumFile->iCorrectDigestArray[0] = 0;
	mdCheckSumFile->iCorrectDigestArray[1] = 3040247056;
	mdCheckSumFile->iCorrectDigestArray[2] = 3529725244;
	mdCheckSumFile->iCorrectDigestArray[3] = 1158560601;
	#ifdef GETFILESUM
	CheckFile(mdCheckSumFile);
	#else
	if(!CheckFile(mdCheckSumFile)) return false;
	#endif

	sprintf(mdCheckSumFile->szFileLocation, "%sHAN/weapon.dat", g_pszStorage);
	mdCheckSumFile->iCorrectDigestArray[0] = 0;
	mdCheckSumFile->iCorrectDigestArray[1] = 1112515121;
	mdCheckSumFile->iCorrectDigestArray[2] = 870911676;
	mdCheckSumFile->iCorrectDigestArray[3] = 3587992568;
	#ifdef GETFILESUM
	CheckFile(mdCheckSumFile);
	#else
	if(!CheckFile(mdCheckSumFile)) return false;
	#endif

	return true;
}

bool CheckFile(mdFile* mdChkFile)
{
	FILE* fLocalFile;
	fLocalFile = fopen(mdChkFile->szFileLocation, "r");
	int x;
	unsigned char szBuffer[1024];

	if(!fLocalFile)
	{
		Log("Error trying to load %s", mdChkFile->szFileLocation);
		return false;
	}

	MD5_CTX mdCTX;
	MD5Init(&mdCTX);
	while((x = fread(szBuffer, 1, sizeof(szBuffer), fLocalFile) > 0)) MD5Update(&mdCTX, szBuffer, x);
	MD5Final(mdChkFile->szRawLocalDigest, &mdCTX);

	if(ferror(fLocalFile))
	{
		Log("CRC32 checksum failed: an error occurred checking \'%s\'.", mdChkFile->szFileLocation);
		fclose(fLocalFile);
		return false;
	}
		
	// use following output to retreive md5 uint32 array's in md5sums.html
	char path[255];
	sprintf(path, "%sSAMP/md5sums.html", g_pszStorage);
	FILE* md5sums = fopen(path, "a");
	fprintf(md5sums, "[%s]: mdCTX UINT32 buf[]: %u %u %u %u<br>\n", mdChkFile->szFileLocation, mdCTX.buf[0], mdCTX.buf[1], mdCTX.buf[2], mdCTX.buf[3]);
	fprintf(md5sums, "[%s]: mdChkFile->iCorrectDigestArray[]: %u %u %u %u<br>\n", mdChkFile->szFileLocation, mdChkFile->iCorrectDigestArray[0], mdChkFile->iCorrectDigestArray[1], mdChkFile->iCorrectDigestArray[2], mdChkFile->iCorrectDigestArray[3]);
	fclose(md5sums);

	if((mdChkFile->iCorrectDigestArray[0] != mdCTX.buf[0]) || (mdChkFile->iCorrectDigestArray[1] != mdCTX.buf[1]) || (mdChkFile->iCorrectDigestArray[2] != mdCTX.buf[2]) || (mdChkFile->iCorrectDigestArray[3] != mdCTX.buf[3]))
	{
		Log("CRC32 checksum failed: \'%s\'.", mdChkFile->szFileLocation);
		fclose(fLocalFile);
		return false;
	}

	fclose(fLocalFile);
	return true;
}