/**
 * Vector Lab
 * CS 241 - Spring 2019
 */
 
#include "sstring.h"
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    // TODO create some tests

	sstring* sstr = cstr_to_sstring("This is a test.");
	sstring* sstr2 = cstr_to_sstring(" This is another test. With a trailing test");
	char* str2 = sstring_to_cstr(sstr);
	printf("%s \n", str2);
	int length = sstring_append(sstr, sstr2);
	char* str = sstring_to_cstr(sstr);
	printf("New length %d with contents: %s \n", length, str);
	printf("Length by strlen is %lu \n", strlen(str));
	free(str);
	str = NULL;
	sstring* sstr3 = cstr_to_sstring("1234567890");
	char* slice = sstring_slice(sstr3, 2, 5);
	printf("Sliced: %s \n", slice);

	sstring_substitute(sstr, 16, "test", "banana");
	char* replace = sstring_to_cstr(sstr);
	printf("Replaced: %s \n", replace);

	sstring_destroy(sstr);
	sstring_destroy(sstr2);
	sstring_destroy(sstr3);
	free(slice);
	free(str2);
	free(replace);

	sstring *split = cstr_to_sstring("abcdeeawfe");
	vector *vec_sp = sstring_split(split, 'e');
	printf("%s \n", (char*)vector_get(vec_sp, 0));
	printf("%s \n", (char*)vector_get(vec_sp, 1));
	printf("%s \n", (char*)vector_get(vec_sp, 2));

    return 0;
}
