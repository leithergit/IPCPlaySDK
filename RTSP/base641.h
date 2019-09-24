#ifndef _BASE64_H
#define _BASE64_H

int ez_base64decode(char *bufplain, const char *bufcoded);
int ez_base64encode(char *encoded, const char *string, int len);

#endif