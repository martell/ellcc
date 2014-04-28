/** Parse a string for "words".
 * If argv is != NULL, save the pointer to the first character in argv[count]
 * otherwise just count words.
 * If no substitution is done, point to the original word.
 * If substitution is done, point to a newly allocated string.
 */
static int parse_words(char *string, char **argv)
{
    int count = 0;              // The word counter.

    // Skip leading whitespace.
    while (isspace(*string)) {
        ++string;
    }

    // Find the next word.
    int delim = 0;              // Default to whitespace.
    switch (*string) {
    case '\'':
        delim = '\'';
        ++string;
        break;
    case '"':
        delim = '"';
        ++string;
        break;
    }

    return count;
}

static int parse(char *string, char **argv)
{

}
