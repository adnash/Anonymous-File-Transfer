#ifndef __AWGET_H__
#define __AWGET_H__

// Project 2: Anonymous Web Get
// Author: Aaron Smith
// Date:   10/1/2015
// Class:  CS457
// Email:  acsmit@rams.colostate.edu

// The structure to hold all the information passed from SS to SS
typedef struct ss_info {

	char* URL;
	int ssCount;
	char** ssIP;
	int* ssPort;

} ss_info_t;

#endif
