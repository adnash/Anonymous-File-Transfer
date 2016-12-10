#ifndef __AWGET_H__
#define __AWGET_H__

// Anonymous Web Get
// Author: Aaron Smith, Alan Nash

// The structure to hold all the information passed from SS to SS
typedef struct ss_info {

	char* URL;
	int ssCount;
	char** ssIP;
	int* ssPort;

} ss_info_t;

#endif
