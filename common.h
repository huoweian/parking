#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <assert.h>
#include <fcntl.h> 
#include <unistd.h>
#include <termios.h> 
#include <sys/types.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include "sqlite3.h"

#define DEV_PATH1   "/dev/ttySAC1"
#define DEV_PATH2   "/dev/ttySAC2"


bool stop_charging(sqlite3 *db, int cardid, char **licence, char **time_in);
bool start_charging(sqlite3 *db, char *licence, int cardid, char *photo_path);


#endif