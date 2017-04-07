#include "pcre/pcre.h"
#define MAX_LOC_NAME		64
#define MAX_MACRO_STRING 	2048

typedef struct locdata_s {
	vec3_t coord;
	char *name;
	struct locdata_s *next;
} locdata_t;

char *TP_LocationName (vec3_t location);
void TP_LoadLocFile_f (void);
