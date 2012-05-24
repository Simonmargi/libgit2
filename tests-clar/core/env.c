#include "clar_libgit2.h"
#include "fileops.h"
#include "path.h"

#ifdef GIT_WIN32

#include "win32/utf-conv.h"

static char *cl_getenv(const char *name)
{
	wchar_t *name_utf16 = gitwin_to_utf16(name);
	DWORD value_len, alloc_len;
	wchar_t *value_utf16;
	char *value_utf8;

	cl_assert(name_utf16);
	alloc_len = GetEnvironmentVariableW(name_utf16, NULL, 0);
	if (alloc_len <= 0)
		return NULL;

	cl_assert(value_utf16 = git__calloc(alloc_len, sizeof(wchar_t)));

	value_len = GetEnvironmentVariableW(name_utf16, value_utf16, alloc_len);
	cl_assert_equal_i(value_len, alloc_len - 1);

	cl_assert(value_utf8 = gitwin_from_utf16(value_utf16));

	git__free(value_utf16);

	return value_utf8;
}

static int cl_setenv(const char *name, const char *value)
{
	wchar_t *name_utf16 = gitwin_to_utf16(name);
	wchar_t *value_utf16 = value ? gitwin_to_utf16(value) : NULL;

	cl_assert(name_utf16);
	cl_assert(SetEnvironmentVariableW(name_utf16, value_utf16));

	git__free(name_utf16);
	git__free(value_utf16);

	return 0;

}
#else

#include <stdlib.h>
#define cl_getenv(n)   getenv(n)
#define cl_setenv(n,v) (v) ? setenv((n),(v),1) : unsetenv(n)

#endif

static char *env_home = NULL;
static char *env_userprofile = NULL;

void test_core_env__initialize(void)
{
#ifdef GIT_WIN32
	env_userprofile = cl_getenv("USERPROFILE");
#else
	env_home = cl_getenv("HOME");
#endif
}

void test_core_env__cleanup(void)
{
#ifdef GIT_WIN32
	cl_setenv("USERPROFILE", env_userprofile);
	git__free(env_userprofile);
#else
	cl_setenv("HOME", env_home);
#endif
}

void test_core_env__0(void)
{
	static char *home_values[] = {
		"fake_home",
		"fáke_hõme", /* all in latin-1 supplement */
		"fĀke_Ĥome", /* latin extended */
		"fακε_hοmέ",  /* having fun with greek */
		"faงe_นome", /* now I have no idea, but thai characters */
		"f\xe1\x9cx80ke_\xe1\x9c\x91ome", /* tagalog characters */
		"\xe1\xb8\x9fẢke_hoṁe", /* latin extended additional */
		"\xf0\x9f\x98\x98\xf0\x9f\x98\x82", /* emoticons */
		NULL
	};
	git_buf path = GIT_BUF_INIT, found = GIT_BUF_INIT;
	char **val;
	char *check;

	for (val = home_values; *val != NULL; val++) {

		if (p_mkdir(*val, 0777) == 0) {
			/* if we can't make the directory, let's just assume
			 * we are on a filesystem that doesn't support the
			 * characters in question and skip this test...
			 */
			cl_git_pass(git_path_prettify(&path, *val, NULL));

#ifdef GIT_WIN32
			cl_git_pass(cl_setenv("USERPROFILE", path.ptr));

			/* do a quick check that it was set correctly */
			check = cl_getenv("USERPROFILE");
			cl_assert_equal_s(path.ptr, check);
			git__free(check);
#else
			cl_git_pass(cl_setenv("HOME", path.ptr));

			/* do a quick check that it was set correctly */
			check = cl_getenv("HOME");
			cl_assert_equal_s(path.ptr, check);
#endif

			cl_git_pass(git_buf_puts(&path, "/testfile"));
			cl_git_mkfile(path.ptr, "find me");

			cl_git_pass(git_futils_find_global_file(&found, "testfile"));
		}
	}

	git_buf_free(&path);
	git_buf_free(&found);
}