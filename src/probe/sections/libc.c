/*
 * The C runtime.
 *
 * Placed above memory and threads because the allocator sits on both, and below the
 * filesystem because a title formats a path before it opens one.
 *
 * # This section is almost entirely positive checks
 *
 * Everywhere else in this program leans on negative checks, because struct layouts
 * are unknown and calling from the failure side needs none. Here the whole interface
 * is ISO C, so every signature is certain and the checks can ask the question that
 * actually matters: does it *do the right thing*.
 *
 * That is the point of the section. An implementation that fails everything passes
 * every negative check in the suite; not one of these. `calloc` must return zeroed
 * memory, `realloc` must preserve contents, `qsort` must actually sort - and a stub
 * returning success for all three is caught by all three.
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/sections.h"

/*
 * Declared here rather than in platform.h because these names are in the mined corpus,
 * which corpus.h declares as opaque extern const char in surface.c/bulk.c.
 */
OBS_WEAK const void *_Getpctype(void);
OBS_WEAK const void *_Getptolower(void);
OBS_WEAK const void *_Getptoupper(void);
OBS_WEAK const void *_Getwctype(void);
OBS_WEAK const void *_Getwctolower(void);
OBS_WEAK const void *_Getwctoupper(void);

/* The x86-64 ABI requires allocations to suit any scalar type. */
#define OBS_MALLOC_ALIGN 16

static obs_result check_strlen(void) {
    if (strlen("") != 0) {
        return obs_fail("the empty string did not measure zero");
    }
    if (strlen("obscene") != 7) {
        return obs_fail("a known string measured wrong");
    }
    /* An embedded NUL must terminate. Getting this wrong usually means a length was
     * taken from an allocation size rather than by scanning. */
    if (strlen("ab\0cd") != 2) {
        return obs_fail("the scan ran past an embedded NUL");
    }
    return obs_pass();
}

static obs_result check_strcmp(void) {
    if (strcmp("same", "same") != 0) {
        return obs_fail("equal strings did not compare equal");
    }
    if (strcmp("a", "b") >= 0 || strcmp("b", "a") <= 0) {
        return obs_fail("ordering is wrong or the sign is inverted");
    }
    /* Comparison is on unsigned char. A signed implementation orders high bytes below
     * ASCII, which sorts non-ASCII text wrongly and only ever shows up with real
     * data. */
    if (strcmp("\x80", "a") <= 0) {
        return obs_partial("high bytes compare as signed rather than unsigned");
    }
    return obs_pass();
}

static obs_result check_memcmp(void) {
    static const unsigned char a[] = {1, 2, 3, 4};
    static const unsigned char b[] = {1, 2, 9, 4};
    if (memcmp(a, a, sizeof(a)) != 0) {
        return obs_fail("identical buffers did not compare equal");
    }
    if (memcmp(a, b, 2) != 0) {
        return obs_fail("a length-limited comparison read past its length");
    }
    if (memcmp(a, b, sizeof(a)) >= 0) {
        return obs_fail("ordering is wrong or the sign is inverted");
    }
    return obs_pass();
}

static obs_result check_strchr(void) {
    static const char text[] = "obscene";
    const char *found = strchr(text, 'c');
    if (found != text + 3) {
        return obs_fail("a present character was not found at the right offset");
    }
    if (strchr(text, 'z') != NULL) {
        return obs_fail("an absent character was reported found");
    }
    /* Searching for NUL finds the terminator, not nothing. A surprising number of
     * implementations get this edge wrong. */
    if (strchr(text, '\0') != text + 7) {
        return obs_partial("searching for NUL did not find the terminator");
    }
    return obs_pass();
}

static obs_result check_snprintf(void) {
    OBS_REQUIRE(&strcmp);
    char buffer[16];
    int written = snprintf(buffer, sizeof(buffer), "%d-%s", 42, "ok");
    if (written != 5) {
        return obs_fail_code("the return value is not the length written",
                             (uint64_t)(uint32_t)written);
    }
    if (strcmp(buffer, "42-ok") != 0) {
        return obs_fail("the formatted text is wrong");
    }

    /* C99: the return is what *would* have been written, so a caller can size a
     * buffer from it. Returning the truncated length instead is a real and common
     * defect, and it makes every "measure then allocate" call site allocate short. */
    char small[4];
    written = snprintf(small, sizeof(small), "%s", "0123456789");
    if (written != 10) {
        return obs_partial_value("a truncated write did not report the full length",
                                 (uint64_t)(uint32_t)written);
    }
    if (small[3] != '\0') {
        return obs_fail("a truncated write did not terminate the buffer");
    }
    return obs_pass();
}

static obs_result check_malloc_free(void) {
    OBS_REQUIRE(&free);
    void *p = malloc(64);
    if (p == NULL) {
        return obs_fail("a small allocation returned null");
    }
    if ((uintptr_t)p % OBS_MALLOC_ALIGN != 0) {
        /* Under-aligned memory works until something stores a wide type in it. */
        free(p);
        return obs_partial_value("the allocation is not suitably aligned",
                                 (uint64_t)(uintptr_t)p);
    }
    volatile unsigned char *bytes = (volatile unsigned char *)p;
    bytes[0] = 0xa5;
    bytes[63] = 0x5a;
    if (bytes[0] != 0xa5 || bytes[63] != 0x5a) {
        free(p);
        return obs_fail("the allocation does not hold what was written to it");
    }
    free(p);

    /* Two live allocations must not overlap. An allocator that returns the same block
     * twice passes every single-allocation check ever written. */
    void *first = malloc(32);
    void *second = malloc(32);
    if (first == NULL || second == NULL) {
        free(first);
        free(second);
        return obs_fail("a second small allocation returned null");
    }
    int overlapping = first == second;
    free(first);
    free(second);
    if (overlapping) {
        return obs_fail("two live allocations returned the same address");
    }
    return obs_pass();
}

static obs_result check_calloc_zeroes(void) {
    OBS_REQUIRE(&free);
    /* The whole reason calloc exists. An implementation that forwards to malloc
     * without clearing produces bugs that depend on what ran before. */
    unsigned char *p = (unsigned char *)calloc(128, 1);
    if (p == NULL) {
        return obs_fail("a small zeroed allocation returned null");
    }
    for (size_t i = 0; i < 128; i++) {
        if (p[i] != 0) {
            free(p);
            return obs_fail_code("the allocation was not zeroed", (uint64_t)i);
        }
    }
    free(p);
    return obs_pass();
}

static obs_result check_realloc_preserves(void) {
    OBS_REQUIRE(&free, &malloc);
    unsigned char *p = (unsigned char *)malloc(16);
    if (p == NULL) {
        return obs_fail("the initial allocation returned null");
    }
    for (unsigned char i = 0; i < 16; i++) {
        p[i] = (unsigned char)(i + 1);
    }
    unsigned char *grown = (unsigned char *)realloc(p, 256);
    if (grown == NULL) {
        free(p);
        return obs_fail("growing the allocation returned null");
    }
    for (unsigned char i = 0; i < 16; i++) {
        if (grown[i] != (unsigned char)(i + 1)) {
            free(grown);
            return obs_fail_code("the original contents did not survive the move",
                                 (uint64_t)i);
        }
    }
    free(grown);
    return obs_pass();
}

/* Ordering callback, shared by the sort and search checks. Worth having for its own
 * sake beyond the ordering: it makes the platform call back into guest code, which is
 * a path an emulator has to get right and which nothing else here exercises. */
static int compare_ints(const void *a, const void *b) {
    int left = *(const int *)a;
    int right = *(const int *)b;
    if (left < right) {
        return -1;
    }
    return left > right ? 1 : 0;
}

static obs_result check_qsort(void) {
    int values[] = {5, 3, 9, 1, 7, 3};
    qsort(values, 6, sizeof(values[0]), compare_ints);
    for (unsigned int i = 1; i < 6; i++) {
        if (values[i - 1] > values[i]) {
            /* A do-nothing implementation returns void and looks like success. Only
             * inspecting the result catches it. */
            return obs_fail_code("the array is not in order", (uint64_t)i);
        }
    }
    if (values[0] != 1 || values[5] != 9) {
        return obs_fail("the sorted array does not hold the original values");
    }
    return obs_pass();
}

static obs_result check_numeric_conversions(void) {
    OBS_REQUIRE(&abs, &atoi);
    if (abs(-5) != 5 || abs(5) != 5) {
        return obs_fail("absolute value is wrong");
    }
    if (atoi("123") != 123 || atoi("-7") != -7) {
        return obs_fail("decimal conversion is wrong");
    }
    char *end = NULL;
    long parsed = strtol("ff", &end, 16);
    if (parsed != 255) {
        return obs_fail_code("hexadecimal conversion is wrong", (uint64_t)parsed);
    }
    if (end == NULL) {
        return obs_partial("the end pointer was not written");
    }
    return obs_pass();
}

static obs_result check_strcpy_strcat(void) {
    OBS_REQUIRE(&strcat, &strcmp);
    char buffer[16];
    if (strcpy(buffer, "ob") != buffer) {
        return obs_fail("strcpy did not return its destination");
    }
    if (strcmp(buffer, "ob") != 0) {
        return obs_fail("strcpy did not copy correctly");
    }
    if (strcat(buffer, "scene") != buffer) {
        return obs_fail("strcat did not return its destination");
    }
    if (strcmp(buffer, "obscene") != 0) {
        return obs_fail("strcat appended to the wrong place or did not terminate");
    }
    return obs_pass();
}

static obs_result check_strncpy_padding(void) {
    /* strncpy has two behaviours everyone forgets, and both bite. */
    char padded[8];
    for (unsigned int i = 0; i < 8; i++) {
        padded[i] = 'x';
    }
    strncpy(padded, "ab", 8);
    if (padded[0] != 'a' || padded[1] != 'b') {
        return obs_fail("the prefix was not copied");
    }
    /* It pads the whole remaining buffer with NUL, not just one terminator. */
    for (unsigned int i = 2; i < 8; i++) {
        if (padded[i] != '\0') {
            return obs_partial_value("the tail was not NUL-padded to the full length",
                                     (uint64_t)i);
        }
    }

    /* And when the source fills the buffer it does *not* terminate. An
     * implementation that helpfully terminates anyway hides a caller bug on this
     * platform that will surface on another. */
    char exact[4];
    exact[3] = 'z';
    strncpy(exact, "abcd", 4);
    if (exact[3] != 'd') {
        return obs_partial("a full-length copy terminated instead of filling");
    }
    return obs_pass();
}

static obs_result check_strstr(void) {
    static const char text[] = "obscene";
    if (strstr(text, "scene") != text + 2) {
        return obs_fail("a present substring was not found at the right offset");
    }
    if (strstr(text, "absent") != NULL) {
        return obs_fail("an absent substring was reported found");
    }
    /* The empty needle matches at the start. Returning NULL here is a common and
     * silent defect - callers treat it as "not found" and skip work they should do. */
    if (strstr(text, "") != text) {
        return obs_partial("an empty needle did not match at the start");
    }
    return obs_pass();
}

static obs_result check_memchr_strrchr(void) {
    OBS_REQUIRE(&strrchr);
    static const char text[] = "banana";
    if (memchr(text, 'n', 6) != text + 2) {
        return obs_fail("memchr found the wrong occurrence");
    }
    /* The length bound must be honoured even when the byte is present just past it. */
    if (memchr(text, 'n', 2) != NULL) {
        return obs_fail("memchr read past its length");
    }
    if (strrchr(text, 'n') != text + 4) {
        return obs_fail("strrchr found the first occurrence rather than the last");
    }
    if (strrchr(text, 'z') != NULL) {
        return obs_fail("strrchr reported an absent character found");
    }
    return obs_pass();
}

static obs_result check_strspn(void) {
    OBS_REQUIRE(&strcspn);
    if (strspn("abcde", "abc") != 3) {
        return obs_fail("the leading accepted span is wrong");
    }
    if (strspn("xyz", "abc") != 0) {
        return obs_fail("a string with no accepted prefix did not span zero");
    }
    if (strcspn("abcde", "cd") != 2) {
        return obs_fail("the leading rejected span is wrong");
    }
    if (strcspn("abc", "xyz") != 3) {
        return obs_fail("a string with no rejected byte did not span its length");
    }
    return obs_pass();
}

static obs_result check_strtok(void) {
    OBS_REQUIRE(&strcmp);
    /* Destructive and stateful. The state is the interesting part: it lives in the
     * platform library across calls, so this also checks that library-side static
     * storage survives - which under an emulator means .bss and TLS are behaving. */
    char buffer[] = "a,b,,c";
    const char *expected[] = {"a", "b", "c"};
    char *token = strtok(buffer, ",");
    for (unsigned int i = 0; i < 3; i++) {
        if (token == NULL) {
            return obs_fail_code("tokenising stopped early", (uint64_t)i);
        }
        if (strcmp(token, expected[i]) != 0) {
            return obs_fail_code("a token is wrong", (uint64_t)i);
        }
        token = strtok(NULL, ",");
    }
    if (token != NULL) {
        return obs_partial("more tokens were produced than the string contains");
    }
    return obs_pass();
}

/* Shared by the sort and search checks. */
static int compare_ints(const void *a, const void *b);

static obs_result check_bsearch(void) {
    static const int sorted[] = {1, 3, 5, 7, 9, 11};
    int wanted = 7;
    const void *found = bsearch(&wanted, sorted, 6, sizeof(sorted[0]), compare_ints);
    if (found != &sorted[3]) {
        return obs_fail("a present value was not found at the right element");
    }
    wanted = 8;
    if (bsearch(&wanted, sorted, 6, sizeof(sorted[0]), compare_ints) != NULL) {
        return obs_fail("an absent value was reported found");
    }
    return obs_pass();
}

static obs_result check_ctype(void) {
    OBS_REQUIRE(&isalpha, &isdigit, &isspace, &isupper, &tolower);
    if (toupper('a') != 'A' || tolower('A') != 'a') {
        return obs_fail("case conversion is wrong");
    }
    /* Characters with no case must pass through unchanged rather than being mangled. */
    if (toupper('5') != '5' || tolower('5') != '5') {
        return obs_fail("a caseless character was altered");
    }
    if (!isdigit('5') || isdigit('x')) {
        return obs_fail("digit classification is wrong");
    }
    if (!isalpha('x') || isalpha('5')) {
        return obs_fail("alphabetic classification is wrong");
    }
    if (!isspace(' ') || isspace('x')) {
        return obs_fail("whitespace classification is wrong");
    }
    if (!isupper('A') || isupper('a')) {
        return obs_fail("uppercase classification is wrong");
    }
    return obs_pass();
}

static obs_result check_rand_is_seeded(void) {
    OBS_REQUIRE(&srand);
    /* Reproducibility is the whole contract. A generator that ignores its seed breaks
     * every replay, deterministic simulation and seeded test a title has - silently,
     * because the numbers still look random. */
    srand(42);
    int first = rand();
    int second = rand();
    srand(42);
    int repeat = rand();
    if (first != repeat) {
        return obs_fail("the same seed did not reproduce the same sequence");
    }
    if (first == second) {
        return obs_partial_value("two consecutive draws were identical",
                                 (uint64_t)(uint32_t)first);
    }
    if (first < 0 || second < 0) {
        return obs_partial("a draw was negative, which the range forbids");
    }
    return obs_pass();
}

static obs_result check_strtoul_bases(void) {
    if (strtoul("42", NULL, 10) != 42u) {
        return obs_fail("decimal conversion is wrong");
    }
    if (strtoul("ff", NULL, 16) != 255u) {
        return obs_fail("explicit hexadecimal conversion is wrong");
    }
    /* Base zero means "detect from the prefix". Treating it as base ten instead turns
     * every configuration value written in hex into a silent zero. */
    if (strtoul("0x1f", NULL, 0) != 31u) {
        return obs_partial("base detection did not recognise a hexadecimal prefix");
    }
    if (strtoul("017", NULL, 0) != 15u) {
        return obs_partial("base detection did not recognise an octal prefix");
    }
    return obs_pass();
}

/* ---- more of the C runtime -------------------------------------------------
 *
 * Every one of these was censused and never called. ISO C settles each answer, so a
 * failure here is not negotiable - which is what makes them worth the space compared to
 * one more vendor call whose correct behaviour nobody has written down.
 */

static obs_result check_strncat_bounds(void) {
    char buf[16];
    buf[0] = 'a';
    buf[1] = '\0';
    /* Appends at most n characters and always terminates. The bound is the part
     * implementations get wrong: it counts source characters, not the total. */
    char *out = strncat(buf, "bcdef", 3);
    if (out != buf) {
        return obs_fail("strncat did not return its destination");
    }
    if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != 'c' || buf[3] != 'd' ||
        buf[4] != '\0') {
        return obs_fail("strncat appended the wrong number of characters");
    }
    return obs_pass();
}

static obs_result check_strpbrk_and_case(void) {
    OBS_REQUIRE(&strcasecmp);
    const char *text = "hello world";
    const char *hit = strpbrk(text, "ow");
    if (hit != text + 4) {
        /* 'o' at index 4 comes before 'w' at 6. A search that returns the first
         * character of the accept set rather than the first match in the string is a
         * classic inversion, and it looks right on many inputs. */
        return obs_fail("strpbrk found the wrong first occurrence");
    }
    if (strpbrk(text, "xyz") != NULL) {
        return obs_fail("strpbrk found a character that is not there");
    }
    if (strcasecmp("HeLLo", "hello") != 0) {
        return obs_fail("strcasecmp says two strings differing only in case differ");
    }
    if (strcasecmp("hello", "hellp") >= 0) {
        return obs_fail("strcasecmp got the ordering of two different strings wrong");
    }
    return obs_pass();
}

static obs_result check_integer_conversion(void) {
    OBS_REQUIRE(&atol, &labs);
    if (atol("-12345") != -12345L) {
        return obs_fail("atol got a negative value wrong");
    }
    if (atol("  42abc") != 42L) {
        /* Leading space skipped, trailing rubbish ignored. Both are specified, and an
         * implementation that rejects either looks stricter and is wrong. */
        return obs_fail("atol did not stop at the first non-digit");
    }
    char *end = NULL;
    long long value = strtoll("7fff", &end, 16);
    if (value != 0x7fffLL) {
        return obs_fail_code("strtoll got a hexadecimal value wrong", (uint64_t)value);
    }
    if (end == NULL || *end != '\0') {
        return obs_fail("strtoll did not consume the whole number");
    }
    if (labs(-7L) != 7L || labs(7L) != 7L) {
        return obs_fail("labs is wrong");
    }
    return obs_pass();
}

static obs_result check_character_classes(void) {
    OBS_REQUIRE(&isalnum, &islower, &ispunct);
    /* Each of these has a boundary an implementation can put one place out. Space is
     * printable and not punctuation; a digit is alphanumeric and not lowercase. */
    if (!islower('q') || islower('Q') || islower('4')) {
        return obs_fail("islower is wrong");
    }
    if (!isalnum('7') || !isalnum('z') || isalnum('-')) {
        return obs_fail("isalnum is wrong");
    }
    if (!isprint(' ') || !isprint('~') || isprint('\n')) {
        return obs_fail("isprint is wrong");
    }
    if (!ispunct('-') || ispunct(' ') || ispunct('a')) {
        return obs_fail("ispunct is wrong");
    }
    return obs_pass();
}

static obs_result check_wide_strings(void) {
    static const obs_wchar text[] = {'w', 'i', 'd', 'e', 0};
    size_t n = wcslen(text);
    if (n != 4) {
        return obs_fail_code("wcslen counted the wrong number of wide characters",
                             (uint64_t)n);
    }
    return obs_pass_value((uint64_t)n);
}

static obs_result check_wide_integer_text(void) {
    OBS_REQUIRE(&strtoull);
    if (atoll("-1234") != -1234LL) {
        return obs_fail_code("atoll of a negative number is wrong",
                             (uint64_t)atoll("-1234"));
    }
    if (atoll("0") != 0LL || atoll("7") != 7LL) {
        return obs_fail("atoll is wrong on a small number");
    }
    /* Wider than 32 bits, which is the point: a wide conversion that truncates to int
     * is right about every value a narrower check would use. */
    if (atoll("4294967296") != 4294967296LL) {
        return obs_fail("atoll truncates to 32 bits");
    }
    char *end = 0;
    if (strtoull("ff", &end, 16) != 255ULL) {
        return obs_fail("strtoull is wrong in base 16");
    }
    if (end == 0 || *end != 0) {
        return obs_fail("strtoull did not consume the whole number");
    }
    /* Base zero means "read the prefix", so 0x is hexadecimal and a leading 0 is
     * octal. An implementation that treats base zero as base ten answers 0 here. */
    if (strtoull("0x10", 0, 0) != 16ULL) {
        return obs_fail("strtoull ignores the base prefix when the base is zero");
    }
    if (strtoull("010", 0, 0) != 8ULL) {
        return obs_fail("strtoull does not read a leading zero as octal");
    }
    return obs_pass();
}

static obs_result check_wide_absolute(void) {
    if (llabs(-5LL) != 5LL || llabs(5LL) != 5LL || llabs(0LL) != 0LL) {
        return obs_fail("llabs is wrong");
    }
    /* Larger than an int can hold. A version that forwards to abs returns a truncated
     * value here and the right answer everywhere a narrower check would look. */
    if (llabs(-4294967296LL) != 4294967296LL) {
        return obs_fail("llabs truncates to 32 bits");
    }
    return obs_pass();
}

static obs_result check_bounded_case_compare(void) {
    if (strncasecmp("HELLO", "hello", 5) != 0) {
        return obs_fail("strncasecmp does not ignore case");
    }
    /* The bound is what separates this from strcasecmp: the strings differ at the
     * third character, and a comparison of the first two must not see it. */
    if (strncasecmp("abc", "abd", 2) != 0) {
        return obs_fail("strncasecmp reads past its bound");
    }
    if (strncasecmp("abc", "abd", 3) >= 0) {
        return obs_fail("strncasecmp does not order within its bound");
    }
    if (strncasecmp("abd", "abc", 3) <= 0) {
        return obs_fail("strncasecmp is not antisymmetric");
    }
    /* A bound of zero compares nothing, so everything is equal. An implementation
     * that reads one character first gets this wrong. */
    if (strncasecmp("a", "b", 0) != 0) {
        return obs_fail("strncasecmp with a bound of zero compared something");
    }
    return obs_pass();
}

static obs_result check_strdup(void) {
    OBS_REQUIRE(&free, &strcmp);
    const char *source = "duplicate me";
    char *copy = strdup(source);
    if (copy == 0) {
        return obs_fail("strdup returned nothing");
    }
    if (copy == source) {
        free(copy);
        return obs_fail("strdup returned the original rather than a copy");
    }
    if (strcmp(copy, source) != 0) {
        free(copy);
        return obs_fail("strdup made a copy that does not match");
    }
    /* Writing to it is the part that matters: a "copy" sharing storage with a string
     * literal compares equal and faults on the first write, which would show up much
     * later and somewhere else. */
    copy[0] = 'D';
    if (copy[0] != 'D' || source[0] != 'd') {
        free(copy);
        return obs_fail("strdup shares storage with the original");
    }
    free(copy);
    return obs_pass();
}

static obs_result check_sprintf(void) {
    OBS_REQUIRE(&strcmp);
    char buf[32];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = 0x7f;
    }
    int written = sprintf(buf, "%d-%s", 42, "ok");
    if (written != 5) {
        return obs_fail_code("sprintf returned the wrong length", (uint64_t)written);
    }
    if (strcmp(buf, "42-ok") != 0) {
        return obs_fail("sprintf produced the wrong text");
    }
    /* The terminator is not counted in the return value but must be written. Checking
     * the byte after it catches a version that writes one too many. */
    if (buf[5] != 0) {
        return obs_fail("sprintf did not terminate the string");
    }
    if (buf[6] != 0x7f) {
        return obs_fail("sprintf wrote past the terminator");
    }
    return obs_pass();
}

static obs_result check_getpctype(void) {
    const void *(*fn_getpctype)(void) = (const void *(*)(void))_Getpctype;
    if (fn_getpctype == NULL || !obs_address_is_callable((const void *)fn_getpctype)) {
        fn_getpctype = (const void *(*)(void))obs_module_symbol(1, "_Getpctype");
    }
    if (!obs_address_is_callable((const void *)fn_getpctype)) {
        return obs_skip("_Getpctype is not callable");
    }

    const void *(*fn_getptolower)(void) = (const void *(*)(void))_Getptolower;
    if (fn_getptolower == NULL ||
        !obs_address_is_callable((const void *)fn_getptolower)) {
        fn_getptolower = (const void *(*)(void))obs_module_symbol(1, "_Getptolower");
    }

    const void *(*fn_getptoupper)(void) = (const void *(*)(void))_Getptoupper;
    if (fn_getptoupper == NULL ||
        !obs_address_is_callable((const void *)fn_getptoupper)) {
        fn_getptoupper = (const void *(*)(void))obs_module_symbol(1, "_Getptoupper");
    }

    const void *pct_ptr = fn_getpctype();
    obs_report_measure("035-libc/getpctype", "_Getpctype", "pointer",
                       (uint64_t)(uintptr_t)pct_ptr, "address");
    if (pct_ptr == NULL) {
        return obs_fail("_Getpctype returned NULL");
    }

    /* Dump 544 bytes of the raw classification table starting from pct_ptr - 16 bytes
     * (covering index -1 EOF through index 255). */
    const unsigned char *pct_raw = (const unsigned char *)pct_ptr - 16;
    for (unsigned int off = 0; off < 544; off += 16) {
        obs_report_bytes("035-libc/getpctype", "_Getpctype", "table_raw_neg16", off,
                         &pct_raw[off], 16);
    }

    /* Sample classification masks across known ASCII character classes */
    const uint16_t *ctable = (const uint16_t *)pct_ptr;
    const int16_t *ctable_s = (const int16_t *)pct_ptr;
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_eof_neg1",
                       (uint64_t)(uint16_t)ctable_s[-1], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_nul_0",
                       (uint64_t)ctable[0], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_tab_9",
                       (uint64_t)ctable['\t'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_space_32",
                       (uint64_t)ctable[' '], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_0_48",
                       (uint64_t)ctable['0'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_9_57",
                       (uint64_t)ctable['9'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_A_65",
                       (uint64_t)ctable['A'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_F_70",
                       (uint64_t)ctable['F'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_Z_90",
                       (uint64_t)ctable['Z'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_a_97",
                       (uint64_t)ctable['a'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_f_102",
                       (uint64_t)ctable['f'], "mask");
    obs_report_measure("035-libc/getpctype", "_Getpctype", "mask_z_122",
                       (uint64_t)ctable['z'], "mask");

    /* Sibling _Getptolower */
    if (fn_getptolower == NULL) {
        fn_getptolower = (const void *(*)(void))_Getptolower;
    }
    if (fn_getptolower != NULL &&
        obs_address_is_callable((const void *)fn_getptolower)) {
        const void *ptolower_ptr = fn_getptolower();
        obs_report_measure("035-libc/getpctype", "_Getptolower", "pointer",
                           (uint64_t)(uintptr_t)ptolower_ptr, "address");
        if (ptolower_ptr != NULL) {
            const unsigned char *lower_raw = (const unsigned char *)ptolower_ptr - 16;
            for (unsigned int off = 0; off < 544; off += 16) {
                obs_report_bytes("035-libc/getpctype", "_Getptolower",
                                 "table_raw_neg16", off, &lower_raw[off], 16);
            }
            const int16_t *ltable_s = (const int16_t *)ptolower_ptr;
            obs_report_measure("035-libc/getpctype", "_Getptolower", "entry_eof_neg1",
                               (uint64_t)(uint16_t)ltable_s[-1], "val");
            obs_report_measure("035-libc/getpctype", "_Getptolower", "entry_A_65",
                               (uint64_t)(uint16_t)ltable_s['A'], "val");
            obs_report_measure("035-libc/getpctype", "_Getptolower", "entry_Z_90",
                               (uint64_t)(uint16_t)ltable_s['Z'], "val");
            obs_report_measure("035-libc/getpctype", "_Getptolower", "entry_a_97",
                               (uint64_t)(uint16_t)ltable_s['a'], "val");
            obs_report_measure("035-libc/getpctype", "_Getptolower", "entry_0_48",
                               (uint64_t)(uint16_t)ltable_s['0'], "val");
        }
    }

    /* Sibling _Getptoupper */
    if (fn_getptoupper == NULL) {
        fn_getptoupper = (const void *(*)(void))_Getptoupper;
    }
    if (fn_getptoupper != NULL &&
        obs_address_is_callable((const void *)fn_getptoupper)) {
        const void *ptoupper_ptr = fn_getptoupper();
        obs_report_measure("035-libc/getpctype", "_Getptoupper", "pointer",
                           (uint64_t)(uintptr_t)ptoupper_ptr, "address");
        if (ptoupper_ptr != NULL) {
            const unsigned char *upper_raw = (const unsigned char *)ptoupper_ptr - 16;
            for (unsigned int off = 0; off < 544; off += 16) {
                obs_report_bytes("035-libc/getpctype", "_Getptoupper",
                                 "table_raw_neg16", off, &upper_raw[off], 16);
            }
            const int16_t *utable_s = (const int16_t *)ptoupper_ptr;
            obs_report_measure("035-libc/getpctype", "_Getptoupper", "entry_eof_neg1",
                               (uint64_t)(uint16_t)utable_s[-1], "val");
            obs_report_measure("035-libc/getpctype", "_Getptoupper", "entry_a_97",
                               (uint64_t)(uint16_t)utable_s['a'], "val");
            obs_report_measure("035-libc/getpctype", "_Getptoupper", "entry_z_122",
                               (uint64_t)(uint16_t)utable_s['z'], "val");
            obs_report_measure("035-libc/getpctype", "_Getptoupper", "entry_A_65",
                               (uint64_t)(uint16_t)utable_s['A'], "val");
            obs_report_measure("035-libc/getpctype", "_Getptoupper", "entry_0_48",
                               (uint64_t)(uint16_t)utable_s['0'], "val");
        }
    }

    return obs_pass_value((uint64_t)(uintptr_t)pct_ptr);
}

static obs_result check_wctype_and_fpu(void) {
    /* 1. Hardware FPU / MXCSR status */
    uint32_t mxcsr = 0;
    __asm__ __volatile__("stmxcsr %0" : "=m"(mxcsr));
    obs_report_measure("035-libc/fpu-environment", "mxcsr", "raw", (uint64_t)mxcsr,
                       "flags");
    obs_report_measure("035-libc/fpu-environment", "mxcsr", "daz_denormals_are_zero",
                       (uint64_t)((mxcsr >> 6) & 1u), "flag");
    obs_report_measure("035-libc/fpu-environment", "mxcsr", "ftz_flush_to_zero",
                       (uint64_t)((mxcsr >> 15) & 1u), "flag");
    obs_report_measure("035-libc/fpu-environment", "mxcsr", "rounding_mode",
                       (uint64_t)((mxcsr >> 13) & 3u), "enum");
    obs_report_measure("035-libc/fpu-environment", "mxcsr", "exception_masks",
                       (uint64_t)((mxcsr >> 7) & 0x3fu), "mask");

    /* 2. Wide character classification tables */
    const void *(*fn_getwctype)(void) =
        (const void *(*)(void))obs_module_symbol(OBS_HANDLE_SELF, "_Getwctype");
    if (fn_getwctype == NULL) {
        fn_getwctype =
            (const void *(*)(void))obs_module_symbol(OBS_HANDLE_SELF, "_Getwctypetab");
    }
    if (fn_getwctype != NULL && obs_address_is_callable((const void *)fn_getwctype)) {
        const void *wctype_ptr = fn_getwctype();
        obs_report_measure("035-libc/wctype", "_Getwctype", "pointer",
                           (uint64_t)(uintptr_t)wctype_ptr, "address");
        if (wctype_ptr != NULL) {
            const unsigned char *wctype_raw = (const unsigned char *)wctype_ptr;
            for (unsigned int off = 0; off < 256; off += 16) {
                obs_report_bytes("035-libc/wctype", "_Getwctype", "table_raw", off,
                                 &wctype_raw[off], 16);
            }
        }
    }

    const void *(*fn_getwctolower)(void) =
        (const void *(*)(void))obs_module_symbol(OBS_HANDLE_SELF, "_Getwctolower");
    if (fn_getwctolower != NULL &&
        obs_address_is_callable((const void *)fn_getwctolower)) {
        const void *wlower_ptr = fn_getwctolower();
        obs_report_measure("035-libc/wctype", "_Getwctolower", "pointer",
                           (uint64_t)(uintptr_t)wlower_ptr, "address");
        if (wlower_ptr != NULL) {
            const unsigned char *wlower_raw = (const unsigned char *)wlower_ptr;
            for (unsigned int off = 0; off < 256; off += 16) {
                obs_report_bytes("035-libc/wctype", "_Getwctolower", "table_raw", off,
                                 &wlower_raw[off], 16);
            }
        }
    }

    const void *(*fn_getwctoupper)(void) =
        (const void *(*)(void))obs_module_symbol(OBS_HANDLE_SELF, "_Getwctoupper");
    if (fn_getwctoupper != NULL &&
        obs_address_is_callable((const void *)fn_getwctoupper)) {
        const void *wupper_ptr = fn_getwctoupper();
        obs_report_measure("035-libc/wctype", "_Getwctoupper", "pointer",
                           (uint64_t)(uintptr_t)wupper_ptr, "address");
        if (wupper_ptr != NULL) {
            const unsigned char *wupper_raw = (const unsigned char *)wupper_ptr;
            for (unsigned int off = 0; off < 256; off += 16) {
                obs_report_bytes("035-libc/wctype", "_Getwctoupper", "table_raw", off,
                                 &wupper_raw[off], 16);
            }
        }
    }

    return obs_pass_value((uint64_t)mxcsr);
}

static const obs_check libc_checks[] = {
    {"035-libc/strlen", "libSceLibcInternal", "strlen", OBS_CAP_NONE, OBS_CAP_LIBC,
     (const void *)&strlen, check_strlen, OBS_FROM_SPEC},
    {"035-libc/strcmp", "libSceLibcInternal", "strcmp", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&strcmp, check_strcmp, OBS_FROM_SPEC},
    {"035-libc/memcmp", "libSceLibcInternal", "memcmp", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&memcmp, check_memcmp, OBS_FROM_SPEC},
    {"035-libc/strchr", "libSceLibcInternal", "strchr", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&strchr, check_strchr, OBS_FROM_SPEC},
    {"035-libc/snprintf", "libSceLibcInternal", "snprintf", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&snprintf, check_snprintf, OBS_FROM_SPEC},
    {"035-libc/malloc-free", "libSceLibcInternal", "malloc", OBS_CAP_NONE, OBS_CAP_HEAP,
     (const void *)&malloc, check_malloc_free, OBS_FROM_SPEC},
    {"035-libc/calloc-zeroes", "libSceLibcInternal", "calloc", OBS_CAP_HEAP,
     OBS_CAP_NONE, (const void *)&calloc, check_calloc_zeroes, OBS_FROM_SPEC},
    {"035-libc/realloc-preserves", "libSceLibcInternal", "realloc", OBS_CAP_HEAP,
     OBS_CAP_NONE, (const void *)&realloc, check_realloc_preserves, OBS_FROM_SPEC},
    {"035-libc/qsort", "libSceLibcInternal", "qsort", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&qsort, check_qsort, OBS_FROM_SPEC},
    {"035-libc/bsearch", "libSceLibcInternal", "bsearch", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&bsearch, check_bsearch, OBS_FROM_SPEC},
    {"035-libc/numeric", "libSceLibcInternal", "strtol", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&strtol, check_numeric_conversions, OBS_FROM_SPEC},
    {"035-libc/strtoul-bases", "libSceLibcInternal", "strtoul", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&strtoul, check_strtoul_bases, OBS_FROM_SPEC},
    {"035-libc/strcpy-strcat", "libSceLibcInternal", "strcpy", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&strcpy, check_strcpy_strcat, OBS_FROM_SPEC},
    {"035-libc/strncpy-padding", "libSceLibcInternal", "strncpy", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&strncpy, check_strncpy_padding, OBS_FROM_SPEC},
    {"035-libc/strstr", "libSceLibcInternal", "strstr", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&strstr, check_strstr, OBS_FROM_SPEC},
    {"035-libc/memchr-strrchr", "libSceLibcInternal", "memchr", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)&memchr, check_memchr_strrchr, OBS_FROM_SPEC},
    {"035-libc/strspn", "libSceLibcInternal", "strspn", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&strspn, check_strspn, OBS_FROM_SPEC},
    {"035-libc/strtok", "libSceLibcInternal", "strtok", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&strtok, check_strtok, OBS_FROM_SPEC},
    {"035-libc/ctype", "libSceLibcInternal", "toupper", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&toupper, check_ctype, OBS_FROM_SPEC},
    {"035-libc/rand-seeded", "libSceLibcInternal", "rand", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&rand, check_rand_is_seeded, OBS_FROM_SPEC},
    {"035-libc/strncat-bounds", "libSceLibcInternal", "strncat", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&strncat, check_strncat_bounds, OBS_FROM_SPEC},
    {"035-libc/strpbrk-case", "libSceLibcInternal", "strpbrk", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&strpbrk, check_strpbrk_and_case, OBS_FROM_SPEC},
    {"035-libc/integer-conversion", "libSceLibcInternal", "strtoll", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&strtoll, check_integer_conversion, OBS_FROM_SPEC},
    {"035-libc/character-classes", "libSceLibcInternal", "isprint", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&isprint, check_character_classes, OBS_FROM_SPEC},
    {"035-libc/wide-strings", "libSceLibcInternal", "wcslen", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&wcslen, check_wide_strings, OBS_FROM_SPEC},
    {"035-libc/wide-integer-text", "libSceLibcInternal", "atoll", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&atoll, check_wide_integer_text, OBS_FROM_SPEC},
    {"035-libc/wide-absolute", "libSceLibcInternal", "llabs", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&llabs, check_wide_absolute, OBS_FROM_SPEC},
    {"035-libc/bounded-case-compare", "libSceLibcInternal", "strncasecmp", OBS_CAP_LIBC,
     OBS_CAP_NONE, (const void *)&strncasecmp, check_bounded_case_compare,
     OBS_FROM_SPEC},
    {"035-libc/strdup", "libSceLibcInternal", "strdup", OBS_CAP_HEAP, OBS_CAP_NONE,
     (const void *)&strdup, check_strdup, OBS_FROM_SPEC},
    {"035-libc/sprintf", "libSceLibcInternal", "sprintf", OBS_CAP_LIBC, OBS_CAP_NONE,
     (const void *)&sprintf, check_sprintf, OBS_FROM_SPEC},
    {"035-libc/getpctype", "libSceLibcInternal", "_Getpctype", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_getpctype, check_getpctype, OBS_FROM_SPEC},
    {"035-libc/fpu-wctype", "libSceLibcInternal", "_Getwctype", OBS_CAP_NONE,
     OBS_CAP_NONE, (const void *)check_wctype_and_fpu, check_wctype_and_fpu,
     OBS_FROM_SPEC},
};

const obs_section obs_section_libc = {
    "035-libc",
    "C runtime",
    "The standard library a title leans on hardest, checked for behaviour rather than "
    "for mere presence.",
    libc_checks,
    OBS_COUNT(libc_checks),
};
