/*
 * name:	cg_uid.c - IRATA [*]
 *
 * desc:	Create a key to store XP on server
 * 			(... and maybe identify player for levels?!) if PB is not enabled
 *			For now this untrusted (currently not very safe) key is sent via messages to the server.
 *			Key is silently created and read with no output, issues are logged only.
 *
 * NQQS:	WIP/TODO
 * 			- add to win project files
 *			- additional checks for NQKEY (if read key is a valid nq key?)
 *			- test time stamps in CG_GenerateNQKey
 *			- general test!
 *			- optimize
 *			- clean up
 *
 */

/*
 PB-KEY INFO (we don't use this)
1 - A 12-byte header common to all etkeys: 000000100220 in ASCII (0-9 ASCII = 48-57 decimal = $30-$39 hex).
2 - 6 bytes with the date of the file creation in ASCII in the format: YYMMDD (ex.: 111007 for 07 Oct 2011).
3 - 2 bytes with 0's.
4 - 8 bytes with values between 0 and 9. I'm guessing this might be the time of the file creation but I couldn't figure out how it was encoded (didn't give it much thought anyways). If anybody knows about this block's meaning, please PM'me.
5 - 39 bytes with totally random values.

Total = 67 bytes

The local PB server on the game server host generates the PB GUID based on a sequence of 39 random bytes in the etkey file.
A byte (= 8 bits) can assume 256 different values, so with 39 bytes you have 256 ^ 39 (256 to the power of 39) possible unique combinations.
Try to compute that number if you can. The PB GUID itself is formed by a sequence of 32 hex characters (0 to 9 plus A to F = 16 possible values),
which allows for 16 ^ 32 unique combinations, another astronomic number. The probability of 2 equal GUID's being generated is infinitesimal.
As to GUID spoofing, the possibility of doing it with these custom generated GUID's is the same as with the old GUID's.
*/

#include "cg_local.h"
#include "md5.h"

// file full of random crap
#define NQKEY_FILE "nqkey.cfg" // file extension is required to find file in path!
#define NQKEY_SIZE 28

#define NQ_KEY_PACKET_SIZE 		PACKET_OFFSET + PB_GUID_LENGTH

// Our key, put into cgs ?
char* NQKey; // calculated, same size as PB_GUID_LENGTH
char nqkey[NQKEY_SIZE]; // file data FIXME: CalculateGUID() adds some bytes atm!

#define DEBUG_NQKEY 1

/*
==================

SanitizeString - just a copy from g_cmds.c

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out, qboolean fToLower ) {
	while(*in) {
		if(*in == 27 || *in == '^') {
			in++;		// skip color code
			if(*in) in++;
			continue;
		}
		if(*in < 32) {
			in++;
			continue;
		}
		*out++ = (fToLower) ? tolower(*in++) : *in++;
	}
	*out = 0;
}

#define RANDOM01 3
#define RANDOM02 RANDOM01 - 1
#define RANDOM03 4

/*
===============
CG_GenerateNQKey
 a new one
Check for a valid NQKEY_FILE - if not, try to generate

Returns qtrue if there is a valid key
===============
*/
static qboolean CG_GenerateNQKey(void){
	fileHandle_t f;
	int len = trap_FS_FOpenFile(NQKEY_FILE, &f, FS_READ);

#ifdef DEBUG_NQKEY
	CG_Printf( S_COLOR_YELLOW "NQKEY found - current size is: %d\n", len ); // DEBUG
#endif
	// valid
	if( len == NQKEY_SIZE ) {

		// TODO: additional checks ...

		trap_FS_Read( nqkey, len, f );
		nqkey[len] = 0;
		trap_FS_FCloseFile( f );
#ifdef DEBUG_NQKEY
		CG_Printf( S_COLOR_YELLOW "NQKEY data: *%s*\n", nqkey ); // DEBUG
#endif

		NQKey = CalculateGUID(nqkey);

#ifdef DEBUG_NQKEY
		CG_Printf( S_COLOR_YELLOW "NQKEY data after calc: *%s*\n", nqkey );
#endif

	}
	else { // invalid/new key
		struct tm *ptr_time;
		time_t current_time;
	    int one,two,three,four;
	    int five,six,seven,eight;
	    char *header = "000000100220",*end = "00",*YY = "12",*MM = "01",*DD = "01";
		char *buff;
		qboolean write = qtrue;

		trap_FS_FCloseFile( f ); // reuse file handle

		// CG_Printf( "NQKEY building ...\n" );

		// print issues ...
		if (len > 0 && len < NQKEY_SIZE ) {
			CG_Printf(S_COLOR_YELLOW  "NQKEY found but invalid.\n");
		}
		else if( len <= 0 ) {
			CG_Printf(S_COLOR_YELLOW  "NQKEY not found.\n");
		}

		// simple NQ KEY structure:
		// 0000001002 (base for ET)
		// 99 (to ensure evenbalance never gave this key)
		// xxxxxxxxxx (unix timestamp, reversed)
		// YYYYYY (6 random numbers)
		// NOTE ! the following code differs in structure

		// timestamps ...

		time(&current_time);
		ptr_time = localtime( &current_time );

		// note: key fails if there are not 2 digits at the end which shouldn't happen if system time is correct (>2000)
		YY = va( "%i", ptr_time->tm_year - 100 ); // simple trick to remove century

		// make sure we have 2 digits here month starts with 0
		if(ptr_time->tm_mon+1 < 10) {
				MM = va("0%i", ptr_time->tm_mon+1);
		}
		else {
				MM = va("%i", ptr_time->tm_mon+1);
		}

		if(ptr_time->tm_mday < 10) {
				DD = va("0%i", ptr_time->tm_mday);
		}
		else {
			DD = va("%i", ptr_time->tm_mday);
		}

		// CG_Printf( "NQKEY Y:%s D:%s M:%s.\n", YY, DD, MM);
		// end timestamps

		one = (rand() % 9);
		two = (rand() % 9);
		if(one > two) {
			three = (rand() % 7) - qtrue;
		}
		else {
			three = (rand() % 3) + qfalse;
		}
		four = (rand() % 9) / 2;
		switch( four ) {
			case 1:
				five = RANDOM03 * (rand() % 10) / 4 - RANDOM03;
				break;
			case 5:
				five = (rand() % 3) * RANDOM01;
				break;
			default:
				five = (rand() % 7) + RANDOM02;
				break;
		}
		if(five > 4 || five < 10) {
			six = (rand() % 8);
		}
		else {
			six = (rand() % RANDOM03);
		}
		seven = (rand() % 9);
		eight = (rand() % 9);

		// simple ET KEY end ...

		// FIXME: va isn't buffer size save
		buff = va("%s%s%s%s%s%i%i%i%i%i%i%i%i",
			header,
			YY,
			MM,
			DD,
			end,
			one,
			two,
			three,
			four,
			five,
			six,
			seven,
			eight
			);
#ifdef DEBUG_NQKEY
		CG_Printf( "NQKEY BUFFER: %s\n", buff ); // DEBUG
#endif
		if (trap_FS_FOpenFile( NQKEY_FILE, &f, FS_WRITE ) < 0) {
			char homepath[MAX_OSPATH];

			trap_Cvar_VariableStringBuffer("fs_homepath", homepath, sizeof(homepath));
			CG_Printf( S_COLOR_RED "NQKEY ERROR: can't write NQKEY in %s.\n", homepath );
			// trap_FS_FCloseFile( f ); // is there a need to close the file if we can't open it?

			buff = va("%s", "1111111111111111111111111111"); // this is our invalid 'key'

			//  and case not to crash things
			memcpy(nqkey, buff, NQKEY_SIZE);

#ifdef DEBUG_NQKEY
			CG_Printf(S_COLOR_RED "NQKEY before CalculateGUID *%s*\n", nqkey);
#endif
			NQKey = CalculateGUID(buff);

			write = qfalse;
		}

		if (write == qtrue) {
			trap_FS_Write( buff, strlen(buff), f );
			trap_FS_FCloseFile( f );

			memcpy(nqkey, buff, NQKEY_SIZE);

			NQKey = CalculateGUID(buff);
		}
#ifdef DEBUG_NQKEY
		// new key !
		CG_Printf(S_COLOR_YELLOW "NQKEYDATA memory. *%s*\n", nqkey);
#endif
	}


#ifdef DEBUG_NQKEY
	CG_Printf( S_COLOR_YELLOW "NQKEY send: *%s*\n", NQKey);
#endif

	// send the key ...
	{
		int i;
		unsigned char	string[NQ_KEY_PACKET_SIZE];

		*(unsigned char *)&string[0]	= PACKET_C_SEND_NQKEY;	// set packet type..
		for (i = PACKET_OFFSET; i < NQ_KEY_PACKET_SIZE; ++i) {
			*(int *)&string[i] = NQKey[i-PACKET_OFFSET]; // we are in range < 127
			//CG_Printf("%i ",string[i]);
		}
#ifdef DEBUG_NQKEY
		CG_Printf("\nSENDING INFO: %s\n", string);
#endif
		trap_SendMessage( (char*)&string, NQ_KEY_PACKET_SIZE );
	}

	return qtrue;
}

/*
 * Inits the NQKEY and sends it to server
 *
 */
void CG_InitNQGUID() {
	CG_GenerateNQKey();
}



//
// Delete following lines one day ...
//

/* remove ?
int byte2hex(unsigned char *dst, unsigned char *data, int len) {
    static const char   hex[] = "0123456789ABCDEF";
    int     i;

    for(i = 0; i < len; i++) {
        dst[i << 1]       = hex[data[i] >> 4];
        dst[(i << 1) + 1] = hex[data[i] & 15];
    }
    dst[i << 1] = 0;
    return(i);
}
*/

/* remove ?
void PB_GUIDFROMETKEY(unsigned char *res, unsigned char *key) {
	MD5_CTX     ctx;
	MD5Init(&ctx, 0x00b684a3);
    MD5Update(&ctx, key, 18);
    MD5Final(&ctx);
    byte2hex(res, ctx.digest, 16);  // the md5 is performed on the hex string
    MD5Init(&ctx, 0x00051a56);
    MD5Update(&ctx, res, 32);
    MD5Final(&ctx);
    memcpy(res, ctx.digest, 16);
}
*/

/*
 * simple pb guid_check
 * returns qtrue if the given guid is a valid one
 * char  *pb_guid

static qboolean isPBGUID(char *pb_guid) {
	char guids[PB_GUID_LENGTH+1];
	int i;

	// CG_Printf ("NQKEY pb check: %s\n", pb_guid);

	if(!pb_guid)  { // should never happen
		//CG_Printf ("1 ...\n");
		return qfalse;
	}
	if (strlen(pb_guid) <= 0) {
		//CG_Printf ("2 ...\n");
		return qfalse;
	}
	if (!pb_guid[0]) {
		//CG_Printf ("3 ...\n");
		return qfalse;
	}

	// checks for invalid chars in GUID
	for (i = 0 ; i < PB_GUID_LENGTH ; i++) {
		if ( pb_guid[i] < 48 || ( pb_guid[i] > 57 && pb_guid[i] < 65) || pb_guid[i] > 70) {
			//CG_Printf ("4 ...\n");
			return qfalse;
		}
	}

	// memset( guids, 0, sizeof( guids ) ); // fill memory ??
	SanitizeString(pb_guid, guids, qfalse);

	if(!Q_stricmp(guids, "")) { // should never happen
		//CG_Printf ("5 ...\n");
		return qfalse;
	}

	if(!Q_stricmp(guids, "unknown")) { // case punkbuster 0 = fresh install
		// CG_Printf ("6 ...\n");
		return qfalse;
	}
	if(!Q_stricmp(guids, "NO_GUID")) { // case punkbuster 1 = but no GUID
		//CG_Printf ("7 ...\n");
		return qfalse;
	}
	if(!Q_stricmp(guids, "NOGUID")) { // ?
		//CG_Printf ("8 ...\n");
		return qfalse;
	}

	// yay .. a valid one
	return qtrue;
}
 */

/*
====================
CG_UpdateGUID

update cl_guid using NQKEY_FILE and optional prefix
====================

void CG_UpdateGUID( const char *prefix, int prefix_len ) {

	fileHandle_t f;
	int len = trap_FS_FOpenFile( NQKEY_FILE, &f, FS_READ);


	// TODO read buffer

	trap_FS_FCloseFile( f );

	if( len != NQKEY_SIZE ) {
		trap_Cvar_Set( "cl_guid", va("%s","NO_NQKEY"));
	}
	else {
		unsigned char res[33];
		char *guid = CG_MD5FileETCompat(NQKEY_FILE);

		PB_GUIDFROMETKEY(res, uid);				//Compute GUID by md5 computation
		byte2hex(guid,res,16);

		if (guid) {
			trap_Cvar_Set("cl_guid", res);
		}
		else {
			trap_Cvar_Set( "cl_guid", va("%s","NO_NQGUID"));  // 2 different keys to figure out what's going on
		}
	}
}
*/


//
// code snipplets for curl key dl & md5 parts
//

/*
  Automatic give guid file

This file by calling GUID_test(),
cl_guid cvars is verified,
if the case of content is unvalid,
	an etkey file is search on the disk
	if an etkey file is not found a new one is downlaoded from etkey.org site
    a guid is computed and the cl_guid cvar is updated

In all case, the computed guid is pb one like (computed in the same way)
 Y are free to use and modify this code
 Do not alter this notice
!Grats to schnogg for give the way to found info about guid compuattion
!Grats to 7killer to have code that

IRATA: refactored
FIXME; 77: warning: passing argument 1 of ‘SanitizeString’ discards qualifiers from pointer target type
FIXME: use fs_basepath to write into only! - not fs_homepath
FIXME: add an alternative URL
*/

/* UNUSED
int hex2byte(unsigned char *dst, unsigned char *data, int len) {
    int     i, t;

    if(len < 0) {
    	len = strlen(data);
    }
    len >>= 1;

    for(i = 0; i < len; i++) {
        sscanf(data + (i << 1), "%02x", &t);
        dst[i] = t;
    }
    return(i);
}
*/

/*
int byte2hex(unsigned char *dst, unsigned char *data, int len) {
    static const char   hex[] = "0123456789ABCDEF";
    int     i;

    for(i = 0; i < len; i++) {
        dst[i << 1]       = hex[data[i] >> 4];
        dst[(i << 1) + 1] = hex[data[i] & 15];
    }
    dst[i << 1] = 0;
    return(i);
}

void PB_GUIDFROMETKEY(unsigned char *res, unsigned char *key) {
	MD5_CTX     ctx;
	MD5Init(&ctx, 0x00b684a3);
    MD5Update(&ctx, key, 18);
    MD5Final(&ctx);
    byte2hex(res, ctx.digest, 16);  // the md5 is performed on the hex string
    MD5Init(&ctx, 0x00051a56);
    MD5Update(&ctx, res, 32);
    MD5Final(&ctx);
    memcpy(res, ctx.digest, 16);
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}
*/

// TODO:
// Move code to UI/connect? return chars to use for error messages ... admins should have control of auto guid creation ... move GUID_test() in init() after NQparse
// param cl_guid ?
// param fs_homepath ?
/* CURL version dl pb key
void GUID_test() {
	char *url = "www.etkey.org/etkey.php";	// URL to get the etkey file TODO: add alternative www.etkey.net and move into local scope
	char buff_tmp[128];

	memset( buff_tmp, 0, sizeof( buff_tmp ) );
	trap_Cvar_VariableStringBuffer( "cl_guid", buff_tmp, sizeof( buff_tmp ) );		//Copy actual guid to tempory buffer

	CG_Printf ("GUID check ...\n");
	if(!guid_check(buff_tmp)) {	//GUID is invalid

		unsigned char res[33];
		unsigned char uid[19] = "";
		unsigned char guid[33]= "";
		char homepath[MAX_OSPATH];
		static char	path[MAX_OSPATH];
		static char Data[67];

		//Fix ME WE need to search in the correct $USER/pb/folder
		//On win7, program use a dtat space different than the program installation
		// I have no idea how make it automatically
		CG_Printf ("Searching etkey file...\n");

		trap_Cvar_VariableStringBuffer("fs_homepath", homepath, sizeof(homepath));
		G_BuildFilePath(homepath, "/etmain/etkey","", path, MAX_OSPATH);
		if(!G_IsFile(path)) {

			trap_Cvar_VariableStringBuffer("fs_basepath", homepath, sizeof(homepath));
			G_BuildFilePath(homepath, "/etmain/etkey","", path, MAX_OSPATH);
			if(!G_IsFile(path)) {					// no local etkey f ound, get one from etkey.org
				CURL  *curl = curl_easy_init();

				if (curl) {
					CURLcode resc;
					// FIXME: use fs_basepath to write into only! - not fs_homepath
					FILE *fp = fopen(path,"wb");
					// TODO: error handling ?
					//if (fp) {

					CG_Printf ("Downloading etkey file ...\n");
					curl_easy_setopt(curl, CURLOPT_URL, url);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
					resc = curl_easy_perform(curl);
					// TODO: error handling ?
					// if (resc) {
					curl_easy_cleanup(curl);

					fclose(fp);
				}
				else {
					CG_Printf ( "You must have an ETKEY file to connect to this server. Automatic system fails, please visit etkey.org or etkey.net to obtain a key!\n");
					return;	
				}
				//trap_SendConsoleCommand("reconnect\n");
			} 

			// perform this action (new setting) only if we did download a new GUID
			CG_Printf ("ETkey file downloaded, setting new GUID ...\n");
			G_ReadDataFromFile(path, Data, sizeof(Data));
			memcpy(uid,Data + 10,18);
			uid[66] = 0;
			PB_GUIDFROMETKEY(res, uid);				//Compute GUID by md5 computation
			byte2hex(guid,res,16);
			trap_Cvar_Set("cl_guid",va("%s",guid));

			CG_Printf ("New GUID set.\n"); // no need to print the GUID for real
		}
		else {
			// do we ever have this case? if guid is valid it's taken from the key file
			CG_Printf ("Valid Key file in path.\n");
		}

	}
	else {
		CG_Printf ("Valid GUID found.\n");
	}
}
*/

/*
void check_FOR_PBGUID() {
	char buff_tmp[128];

	memset( buff_tmp, 0, sizeof( buff_tmp ) );
	trap_Cvar_VariableStringBuffer( "cl_guid", buff_tmp, sizeof( buff_tmp ) );		//Copy actual guid to tempory buffer

	CG_Printf ("PB GUID check ...\n");
	if(!guid_check(buff_tmp)) {	//GUID is invalid

		unsigned char res[33];
		unsigned char uid[19] = "";
		unsigned char guid[33]= "";
		char homepath[MAX_OSPATH];
		static char	path[MAX_OSPATH];
		static char Data[67];

		// Fix ME WE need to search in the correct $USER/pb/folder
		// On win7, program use a dtat space different than the program installation
		// I have no idea how make it automatically
		CG_Printf ("Searching etkey file...\n");

		trap_Cvar_VariableStringBuffer("fs_homepath", homepath, sizeof(homepath));
		G_BuildFilePath(homepath, "/etmain/etkey","", path, MAX_OSPATH);
		if(!G_IsFile(path)) {

			trap_Cvar_VariableStringBuffer("fs_basepath", homepath, sizeof(homepath));
			G_BuildFilePath(homepath, "/etmain/etkey","", path, MAX_OSPATH);
			if(!G_IsFile(path)) {					// no local etkey f ound, get one from etkey.org

				CG_Printf ( "You must have an ETKEY file to connect to this server. Automatic system fails, please visit etkey.org or etkey.net to obtain a key!\n");
				return;
				//trap_SendConsoleCommand("reconnect\n");
			}

			// perform this action (new setting) only if we did download a new GUID
			CG_Printf ("ETkey file downloaded, setting new GUID ...\n");
			G_ReadDataFromFile(path, Data, sizeof(Data));
			memcpy(uid,Data + 10,18);
			uid[66] = 0;
			PB_GUIDFROMETKEY(res, uid);				//Compute GUID by md5 computation
			byte2hex(guid,res,16);
			trap_Cvar_Set("cl_guid",va("%s",guid));

			CG_Printf ("New GUID set.\n"); // no need to print the GUID for real
		}
		else {
			// do we ever have this case? if guid is valid it's taken from the key file
			CG_Printf ("Valid Key file in path.\n");
		}
	}
	else {
		CG_Printf ("Valid GUID found.\n");
	}
}
*/

// import syscalls and md5
/*
char *CG_MD5File( const char *fn, int length, const char *prefix, int prefix_len ) {
	static char final[33] = {""};
	unsigned char digest[16] = {""};
	fileHandle_t f;
	MD5_CTX md5;
	byte buffer[2048];
	int i;
	int filelen = 0;
	int r = 0;
	int total = 0;

	Q_strncpyz( final, "", sizeof( final ) );

	filelen = trap_FS_FOpenFile( fn, &f , FS_READ);

	if( !f ) {
		return final;
	}
	if( filelen < 1 ) {
		trap_FS_FCloseFile( f );
		return final;
	}
	if(filelen < length || !length) {
		length = filelen;
	}

	// FIXME/check
	MD5Init(&md5, 0); // 0x00051a56

	if( prefix_len && *prefix )
		MD5Update(&md5 , (unsigned char *)prefix, prefix_len);

	for(;;) {
		//r = trap_FS_Read2(buffer, sizeof(buffer), f);
		r = trap_FS_Read(buffer, sizeof(buffer), f);
		if(r < 1)
			break;
		if(r + total > length)
			r = length - total;
		total += r;
		MD5Update(&md5 , buffer, r);
		if(r < sizeof(buffer) || total >= length)
			break;
	}
	trap_FS_FCloseFile(f);
	MD5Final(&md5, digest);
	final[0] = '\0';
	for(i = 0; i < 16; i++) {
		Q_strcat(final, sizeof(final), va("%02X", digest[i]));
	}
	return final;
}
*/
