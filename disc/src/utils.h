/*
 * $Id: utils.h,v 1.2 2003/03/18 17:29:11 bogdan Exp $
 *
 * 2003-01-28 created by bogdan
 *
 */



#ifndef _AAA_DIAMETER_UTILS_H
#define _AAA_DIAMETER_UTILS_H



#define trim_lr(_s,_sf) \
	do {\
		(_sf).s = (_s).s;\
		(_sf).len = (_s).len;\
		while( *((_sf).s)==' ') {\
			(_sf).s++;\
			(_sf).len--;\
		}\
		while( ((_sf).s[(_sf).len-1])==' ' )\
			(_sf).len--;\
	}while(0)


#define to_32x_len( _len_ ) \
	( (_len_)+(((_len_)&3)?4-((_len_)&3):0) )


#define set_bit_in_mask( _mask_, _pos_) \
	do{ (_mask_)|=1<<(_pos_); }while(0)



/*
 * returns the number of chars writen by converting the number l into a
 * string. This string will no exceed max_len chars
 */
inline static int int2str(unsigned int l, char* buf, int max_len)
{
	int n = 1000000000;
	int buf_len = 0;

	for( ;n && buf_len<max_len; n=(int)n/10 ) {
		buf[buf_len] = (int)(l/n) + '0';
		l = l % n;
		buf_len += (buf[buf_len]!='0' || buf_len);
	}
	return buf_len?buf_len:1;
}



/*
 * returns the number of chars writen by converting the number l into a
 * string in hexadecimal format. This string will no exceed max_len chars
 */
inline static int int2hexstr(unsigned int l, char* buf, int max_len)
{
	int n = 28;
	int buf_len = 0;
	unsigned char c;

	for( ;n>=0 && buf_len<max_len; n-=4 ) {
		c = (unsigned char)((l>>n)&15);
		buf[buf_len] = c + ((c<10)?'0':('A'-10));
		buf_len += (buf[buf_len]!='0' || buf_len);
	}
	return buf_len?buf_len:1;
}



inline static int hexstr2int( char *buf, unsigned int len)
{
	int n=0;

	for(;len;len--,buf++) {
		if (*buf>='0' && *buf<='9') {
			n = (n<<4) + *buf - '0';
		} else if (*buf>='A' && *buf<='F') {
			n = (n<<4) + 10 + *buf - 'A';
		} else if (*buf>='a' && *buf<='f') {
			n = (n<<4) + 10 + *buf - 'a';
		} else
			return -1;
	}
	return n;
}



inline static char hexchar2int( char c )
{
	if (c>='0' && c<='9') {
		return (c - '0');
	} else if (c>='A' && c<='F') {
		return (10 + c - 'A');
	} else if (c>='a' && c<='f') {
		return (10 + c - 'a');
	}

	return -1;
}


#endif
