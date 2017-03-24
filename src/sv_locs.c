
#ifdef SERVERONLY
#include "qwsvdef.h"
#else
#include "common.h"
#endif

static locdata_t *locdata = NULL;
static int loc_count = 0;
cvar_t	tp_name_someplace = {"tp_name_someplace", "someplace"};

static void TP_ClearLocs(void)
{
	locdata_t *node, *temp;

	for (node = locdata; node; node = temp) {
		Q_free(node->name);
		temp = node->next;
		Q_free(node);
	}

	locdata = NULL;
	loc_count = 0;
}

void TP_ClearLocs_f (void)
{
	int num_locs = 0;

	if (Cmd_Argc() > 1) {
		Con_Printf ("clearlocs : Clears all locs in memory.\n");
		return;
	}

	num_locs = loc_count;
	TP_ClearLocs ();

	Con_Printf ("Cleared %d locs.\n", num_locs);
}

static void TP_AddLocNode(vec3_t coord, char *name)
{
	locdata_t *newnode, *node;

	newnode = (locdata_t *) Q_malloc(sizeof(locdata_t));
	newnode->name = Q_strdup(name);
	newnode->next = NULL;
	memcpy(newnode->coord, coord, sizeof(vec3_t));

	if (!locdata) {
		locdata = newnode;
		loc_count++;
		return;
	}

	for (node = locdata; node->next; node = node->next)
		;

	node->next = newnode;
	loc_count++;
}

#define SKIPBLANKS(ptr) while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r') ptr++
#define SKIPTOEOL(ptr) {while (*ptr != '\n' && *ptr != 0) ptr++; if (*ptr == '\n') ptr++;}

qbool TP_LoadLocFile (char *path, qbool quiet)
{
	char *buf, *p, locname[MAX_OSPATH] = {0}, location[MAX_LOC_NAME];
	int i, n, sign, line, nameindex, mark, overflow;
	vec3_t coord;

	if (!*path)
		return false;

	strlcpy (locname, "locs/", sizeof (locname));
	if (strlen (path) + strlen(locname) + 2 + 4 > MAX_OSPATH) {
		Con_Printf ("TP_LoadLocFile: path name > MAX_OSPATH\n");
		return false;
	}

	strlcat (locname, path, sizeof (locname) - strlen (locname));
	COM_DefaultExtension(locname, ".loc");

	mark = Hunk_LowMark ();
	if (!(buf = (char *) FS_LoadHunkFile (locname, NULL))) {
		if (!quiet)
			Con_Printf ("Could not load %s\n", locname);
		TP_ClearLocs();
		return false;
	}

	TP_ClearLocs();

	// Parse the whole file now
	p = buf;
	line = 1;

	while (1) {
		SKIPBLANKS(p);

		if (!*p) {
			goto _endoffile;
		} else if (*p == '\n') {
			p++;
			goto _endofline;
		} else if (*p == '/' && p[1] == '/') {
			SKIPTOEOL(p);
			goto _endofline;
		}

		// parse three ints
		for (i = 0; i < 3; i++)	{
			n = 0;
			sign = 1;
			while (1) {
				switch (*p++) {
						case ' ': case '\t':
						goto _next;
						case '-':
						if (n) {
							Con_Printf ("Locfile error (line %d): unexpected '-'\n", line);
							SKIPTOEOL(p);
							goto _endofline;
						}
						sign = -1;
						break;
						case '0': case '1': case '2': case '3': case '4':
						case '5': case '6': case '7': case '8': case '9':
						n = n * 10 + (p[-1] - '0');
						break;
						default:	// including eol or eof
						Con_Printf ("Locfile error (line %d): couldn't parse coords\n", line);
						SKIPTOEOL(p);
						goto _endofline;
				}
			}
		_next:
			n *= sign;
			coord[i] = n / 8.0;

			SKIPBLANKS(p);
		}

		// parse location name
		overflow = nameindex = 0;
		while (1) {
			switch (*p)	{
					case '\r':
					p++;
					break;
					case '\n':
					case '\0':
					location[nameindex] = 0;
					TP_AddLocNode(coord, location);
					if (*p == '\n')
						p++;
					goto _endofline;
					default:
					if (nameindex < MAX_LOC_NAME - 1) {
						location[nameindex++] = *p;
					} else if (!overflow) {
						overflow = 1;
						Con_Printf ("Locfile warning (line %d): truncating loc name to %d chars\n", line, MAX_LOC_NAME - 1);
					}
					p++;
			}
		}
	_endofline:
		line++;
	}
_endoffile:

	Hunk_FreeToLowMark (mark);

	//if (loc_numentries) {
	if(loc_count) {
		if (!quiet)
			Con_Printf ("Loaded locfile \"%s\" (%i loc points)\n", locname, loc_count); // loc_numentries);
	} else {
		TP_ClearLocs();
		if (!quiet)
			Con_Printf("Locfile \"%s\" was empty\n", locname);
	}

	return true;
}

void TP_LoadLocFile_f (void)
{
	if (Cmd_Argc() != 2) {
		Con_Printf ("loadloc <filename> : load a loc file\n");
		return;
	}
	TP_LoadLocFile (Cmd_Argv(1), false);
}

typedef struct locmacro_s
{
	char *macro;
	char *val;
} locmacro_t;

static locmacro_t locmacros[] = {
                                    {"ssg", "ssg"},
                                    {"ng", "ng"},
                                    {"sng", "sng"},
                                    {"gl", "gl"},
                                    {"rl", "rl"},
                                    {"lg", "lg"},
                                    {"separator", "-"},
                                    {"ga", "ga"},
                                    {"ya", "ya"},
                                    {"ra", "ra"},
                                    {"quad", "quad"},
                                    {"pent", "pent"},
                                    {"ring", "ring"},
                                    {"suit", "suit"},
                                    {"mh", "mega"},
                                };

#define NUM_LOCMACROS	(sizeof(locmacros) / sizeof(locmacros[0]))

void TP_LocationName_F(void) {
	if (Cmd_Argc() != 2) {
		Con_Printf ("loadloc <filename> : load a loc file\n");
		return;
	}
	float coord = strtof(Cmd_Argv(1),NULL);
	Con_Printf("%s",TP_LocationName (&coord));

}

char *TP_LocationName(vec3_t location)
{
	char *in, *out, *value;
	int i;
	float dist, mindist;
	vec3_t vec;
	static locdata_t *node, *best;
	cvar_t *cvar;
	static qbool recursive;
	static char	buf[1024], newbuf[MAX_LOC_NAME];

	if (!locdata)
		return tp_name_someplace.string;

	if (recursive)
		return "";

	best = NULL;
	mindist = 0;

	for (node = locdata; node; node = node->next) {
		VectorSubtract(location, node->coord, vec);
		dist = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
		if (!best || dist < mindist) {
			best = node;
			mindist = dist;
		}
	}


	newbuf[0] = 0;
	out = newbuf;
	in = best->name;
	while (*in && out - newbuf < sizeof(newbuf) - 1) {
		if (!strncasecmp(in, "$loc_name_", 10)) {
			in += 10;
			for (i = 0; i < NUM_LOCMACROS; i++) {
				if (!strncasecmp(in, locmacros[i].macro, strlen(locmacros[i].macro))) {
					if ((cvar = Cvar_Find(va("loc_name_%s", locmacros[i].macro))))
						value = cvar->string;
					else
						value = locmacros[i].val;
					if (out - newbuf >= sizeof(newbuf) - strlen(value) - 1)
						goto done_locmacros;
					strcpy(out, value);
					out += strlen(value);
					in += strlen(locmacros[i].macro);
					break;
				}
			}
			if (i == NUM_LOCMACROS) {
				if (out - newbuf >= sizeof(newbuf) - 10 - 1)
					goto done_locmacros;
				strcpy(out, "$loc_name_");
				out += 10;
			}
		}
		else {
			*out++ = *in++;
		}
	}
done_locmacros:
	*out = 0;

	buf[0] = 0;
	recursive = true;
	Cmd_ExpandString(newbuf, buf);
	recursive = false;

	return buf;
}

void TP_Init (void)
{
	Cmd_AddCommand ("loadloc", TP_LoadLocFile_f);
}