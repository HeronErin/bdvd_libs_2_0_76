/*
 * subst.c --- substitution program
 *
 * Subst is used as a quicky program to do @ substitutions
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern char *optarg;
extern int optind;
#endif

int	verbose = 0;

#ifdef isspace
/*
 * THT Somehow isspace is not working correctly, returning 0
 * for all characters when run inside make (on the PC, RH7.3).
 */
#undef isspace
#endif

#define isspace(c) (((int) c>=9 && (int) c <=13) || (int) c == 32)



struct subst_entry {
	char *name;
	char *value;
	struct subst_entry *next;
};

struct subst_entry *subst_table = 0;

static int add_subst(char *name, char *value)
{
	struct subst_entry	*ent = 0;
	int	retval;
	
	retval = ENOMEM;
	ent = (struct subst_entry *) malloc(sizeof(struct subst_entry));
	if (!ent)
		goto fail;
	ent->name = (char *) malloc(strlen(name)+1);
	if (!ent->name)
		goto fail;
	ent->value = (char *) malloc(strlen(value)+1);
	if (!ent->value)
		goto fail;
	strcpy(ent->name, name);
	strcpy(ent->value, value);
	ent->next = subst_table;
	subst_table = ent;
	return 0;
fail:
	if (ent) {
		if (ent->name)
			free(ent->name);
		if (ent->value)
			free(ent->value);
		free(ent);
	}
	return retval;
}

static struct subst_entry *fetch_subst_entry(char *name)
{
	struct subst_entry *ent;
	int len1 = strlen(name);


	for (ent = subst_table; ent; ent = ent->next) {
int len2 = strlen(ent->name);
		if (strncmp(name, ent->name, len1) == 0 && 
			(len1 == len2 || isspace(ent->name[len1])))
			break;
if (verbose) fprintf(stderr,
"looking for <%s>[%d], in table=<%s>[%d], strncmp=%d, isspace=%d, ent->name[len]=%c\n",
name, len1, ent->name, len2, strncmp(name, ent->name, len1), isspace(ent->name[len1]), ent->name[len1]);
	}
	return ent;
}

/*
 * Given the starting and ending position of the replacement name,
 * check to see if it is valid, and pull it out if it is.
 */
static char *get_subst_symbol(const char *begin, int len, char prefix)
{
	static char replace_name[128];
	char *cp, *start;

	start = replace_name;
	if (prefix)
		*start++ = prefix;

	if (len > sizeof(replace_name)-2)
		return NULL;
	memcpy(start, begin, len);
	start[len] = 0;
	
	/*
	 * The substitution variable must all be in the of [0-9A-Za-z_].
	 * If it isn't, this must be an invalid symbol name.
	 */
	for (cp = start; *cp; cp++) {
		if (!(*cp >= 'a' && *cp <= 'z') &&
		    !(*cp >= 'A' && *cp <= 'Z') &&
		    !(*cp >= '0' && *cp <= '9') &&
		    !(*cp == '_'))
			return NULL;
	}
	return (replace_name);
}

static void replace_string(char *begin, char *end, char *newstr)
{
	int	replace_len, len;

	replace_len = strlen(newstr);
	len = end - begin;
	if (replace_len == 0)
		memmove(begin, end+1, strlen(end)+1);
	else if (replace_len != len+1)
		memmove(end+(replace_len-len-1), end,
			strlen(end)+1);
	memcpy(begin, newstr, replace_len);
}

static void substitute_line(char *line)
{
	char	*ptr, *name_ptr, *end_ptr;
	struct subst_entry *ent;
	char	*replace_name;
	int	len;

	/*
	 * Expand all @FOO@ substitutions
	 */
	ptr = line;
	while (ptr) {
		name_ptr = strchr(ptr, '@');
		if (!name_ptr)
			break;	/* No more */
		if (*(++name_ptr) == '@') {
			/*
			 * Handle tytso@@mit.edu --> tytso@mit.edu
			 */
			memmove(name_ptr-1, name_ptr, strlen(name_ptr)+1);
			ptr = name_ptr+1;
			continue;
		}
		end_ptr = strchr(name_ptr, '@');
		if (!end_ptr)
			break;
		len = end_ptr - name_ptr;
		replace_name = get_subst_symbol(name_ptr, len, 0);
		if (!replace_name) {
			ptr = name_ptr;
			continue;
		}
		ent = fetch_subst_entry(replace_name);
		if (!ent) {
			fprintf(stderr, "Unfound expansion: '%s'\n",
				replace_name);
			ptr = end_ptr + 1;
			continue;
		}
#if 1
		if (verbose) fprintf(stderr, "Replace name = '%s' with '%s'\n",
		       replace_name, ent->value);
#endif
		ptr = name_ptr-1;
		replace_string(ptr, end_ptr, ent->value);
	}
	/*
	 * Now do a second pass to expand ${FOO}
	 */
	ptr = line;
	while (ptr) {
		name_ptr = strchr(ptr, '$');
		if (!name_ptr)
			break;	/* No more */
		if (*(++name_ptr) != '{') {
			ptr = name_ptr;
			continue;
		}
		name_ptr++;
		end_ptr = strchr(name_ptr, '}');
		if (!end_ptr)
			break;
		len = end_ptr - name_ptr;
		replace_name = get_subst_symbol(name_ptr, len, '$');
		if (!replace_name) {
			ptr = name_ptr;
			continue;
		}			
		ent = fetch_subst_entry(replace_name);
		if (!ent) {
			ptr = end_ptr + 1;
			continue;
		}
#if 1
		if (verbose) fprintf(stderr, "Replace name = '%s' with '%s'\n",
		       replace_name, ent->value);
#endif
		ptr = name_ptr-2;
		replace_string(ptr, end_ptr, ent->value);
	}
}

static void parse_config_file(FILE *f)
{
	char	line[2048];
	char	*cp, *ptr;

	while (!feof(f)) {
		memset(line, 0, sizeof(line));
		if (fgets(line, sizeof(line), f) == NULL)
			break;
		/*
		 * Strip newlines and comments.
		 */
		cp = strchr(line, '\n');
		if (cp)
			*cp = 0;
		cp = strchr(line, '#');
		if (cp)
			*cp = 0;
		/*
		 * Skip trailing and leading whitespace
		 */
		for (cp = line + strlen(line) - 1; cp >= line; cp--) {
			if (*cp == ' ' || *cp == '\t')
				*cp = 0;
			else
				break;
		}
		cp = line;
		while (*cp && isspace(*cp))
			cp++;
		ptr = cp;
		/*
		 * Skip empty lines
		 */
		if (*ptr == 0)
			continue;
		/*
		 * Ignore future extensions
		 */
		if (*ptr == '@')
			continue;
		/*
		 * Parse substitutions
		 */
		for (cp = ptr; *cp; cp++)
			if (isspace(*cp))
				break;
		*cp = 0;
		for (cp++; *cp; cp++)
			if (!isspace(*cp))
				break;
#if 1
		if (verbose)
		fprintf(stderr, "Substitute: '%s' for '%s'\n", ptr, cp ? cp : "<NULL>");
#endif
		add_subst(ptr, cp);
	}
}

/*
 * Return 0 if the files are different, 1 if the files are the same.
 */
static int compare_file(const char *outfn, const char *newfn)
{
	FILE	*old_f, *new_f;
	char	oldbuf[2048], newbuf[2048], *oldcp, *newcp;
	int	retval;

	old_f = fopen(outfn, "r");
	if (!old_f)
		return 0;
	new_f = fopen(newfn, "r");
	if (!new_f)
		return 0;

	while (1) {
		oldcp = fgets(oldbuf, sizeof(oldbuf), old_f);
		newcp = fgets(newbuf, sizeof(newbuf), new_f);
		if (!oldcp && !newcp) {
			retval = 1;
			break;
		}
		if (!oldcp || !newcp || strcmp(oldbuf, newbuf)) {
			retval = 0;
			break;
		}
	}
	fclose(old_f);
	fclose(new_f);
	return retval;
}

	
test_isspace(void)
{
	int c;

	fprintf(stderr, "%3s\t.\t\%s\n", "chr", "issp");
	for (c=0; c<256; c++) {
		fprintf(stderr, "%3d\t%c\t%2d\n", c, isprint(c)? (char) c : '.', isspace((char) c));
	}
}

int main(int argc, char **argv)
{
	char	line[2048];
	int	c;
	FILE	*in, *out;
	char	*outfn = NULL, *newfn = NULL;

	//test_isspace();
	
	while ((c = getopt (argc, argv, "f:v")) != EOF) {
		switch (c) {
		case 'f':
			in = fopen(optarg, "r");
			if (!in) {
				perror(optarg);
				exit(1);
			}
			parse_config_file(in);
			fclose(in);
			break;
		case 'v':
			verbose++;
			break;
		default:
			fprintf(stderr, "%s: [-f config-file] [file]\n", 
				argv[0]);
			break;
		}
	}
	if (optind < argc) {
		in = fopen(argv[optind], "r");
		if (!in) {
			perror(argv[optind]);
			exit(1);
		}
		optind++;
	} else
		in = stdin;
	
	if (optind < argc) {
		outfn = argv[optind];
		newfn = (char *) malloc(strlen(outfn)+20);
		if (!newfn) {
			fprintf(stderr, "Memory error!  Exiting.\n");
			exit(1);
		}
		strcpy(newfn, outfn);
		strcat(newfn, ".new");
		out = fopen(newfn, "w");
		if (!out) {
			perror(newfn);
			exit(1);
		}
	} else {
		out = stdout;
		outfn = 0;
	}
			
	while (!feof(in)) {
		if (fgets(line, sizeof(line), in) == NULL)
			break;
		substitute_line(line);
		fputs(line, out);
	}
	fclose(in);
	fclose(out);
	if (outfn) {
		if (compare_file(outfn, newfn)) {
			if (verbose)
				fprintf(stderr, "No change, keeping %s.\n", outfn);
			unlink(newfn);
		} else {
			if (verbose)
				fprintf(stderr, "Creating or replacing %s.\n", outfn);
			rename(newfn, outfn);
		}
	}
	return (0);
}


