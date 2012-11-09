/* parse a number (dec or hex) from a string */
/* parse_num from redboot sources */

#include <stdbool.h>

bool parse_num(char *s, unsigned int *val);
__inline__ static bool _is_hex(char c);
__inline__ static int _from_hex(char c);
__inline__ static char _tolower(char c);


bool parse_num(char *s, unsigned int *val)
{
	bool first = true;
	char c;
	unsigned int digit, radix = 10, result = 0;

	while (*s == ' ') s++;
	while (*s) {
		if (first && (s[0] == '0') && (_tolower(s[1]) == 'x')) {
			radix = 16;
			s += 2;
			}
        	first = false;
        	c = *s++;
        	if (_is_hex(c) && ((digit = _from_hex(c)) < radix)) {
			/* Valid digit */
			result = (result * radix) + digit;
			}
		else {
			/* Malformatted number */
			return false;
			}
		}
	*val = result;
	return true;
}

/* Validate a hex character */
__inline__ static bool
_is_hex(char c)
{
    return (((c >= '0') && (c <= '9')) ||
            ((c >= 'A') && (c <= 'F')) ||            
            ((c >= 'a') && (c <= 'f')));
}

/* Convert a single hex nibble */
__inline__ static int
_from_hex(char c) 
{
    int ret = 0;

    if ((c >= '0') && (c <= '9')) {
        ret = (c - '0');
    } else if ((c >= 'a') && (c <= 'f')) {
        ret = (c - 'a' + 0x0a);
    } else if ((c >= 'A') && (c <= 'F')) {
        ret = (c - 'A' + 0x0A);
    }
    return ret;
}

/* Convert a character to lower case */
__inline__ static char
_tolower(char c)
{
    if ((c >= 'A') && (c <= 'Z')) {
        c = (c - 'A') + 'a';
    }
    return c;
}


