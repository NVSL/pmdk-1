/*
 * Copyright 2014-2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * log_basic.c -- unit test for pmemlog_*
 *
 * usage: log_basic file operation:...
 *
 * operations are 'n' or 'a' or 'v' or 't' or 'r' or 'w'
 *
 */

#include "unittest.h"

/*
 * do_nbyte -- call pmemlog_nbyte() & print result
 */
static void
do_nbyte(PMEMlogpool *plp)
{
	size_t nbyte = pmemlog_nbyte(plp);
	UT_OUT("usable size: %zu", nbyte);
}

/*
 * do_append -- call pmemlog_append() & print result
 */
static void
do_append(PMEMlogpool *plp)
{
	const char *str[6] = {
		"1st test string\n",
		"2nd test string\n",
		"3rd test string\n",
		"4th test string\n",
		"5th test string\n",
		"6th test string\n"
	};

	for (int i = 0; i < 6; ++i) {
		int rv = pmemlog_append(plp, str[i], strlen(str[i]));
		switch (rv) {
		case 0:
			UT_OUT("append   str[%i] %s", i, str[i]);
			break;
		case -1:
			UT_OUT("!append   str[%i] %s", i, str[i]);
			break;
		default:
			UT_OUT("!append: wrong return value");
			break;
		}
	}
}

/*
 * do_appendv -- call pmemlog_appendv() & print result
 */
static void
do_appendv(PMEMlogpool *plp)
{
	struct iovec iov[9] = {
		{
			.iov_base = "1st test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "2nd test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "3rd test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "4th test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "5th test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "6th test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "7th test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "8th test string\n",
			.iov_len = 16
		},
		{
			.iov_base = "9th test string\n",
			.iov_len = 16
		}
	};

	int rv = pmemlog_appendv(plp, iov, 9);
	switch (rv) {
	case 0:
		UT_OUT("appendv");
		break;
	case -1:
		UT_OUT("!appendv");
		break;
	default:
		UT_OUT("!appendv: wrong return value");
		break;
	}

	rv = pmemlog_appendv(plp, iov, 0);
	UT_ASSERTeq(rv, 0);

	errno = 0;
	rv = pmemlog_appendv(plp, iov, -3);
	UT_ASSERTeq(errno, EINVAL);
	UT_ASSERTeq(rv, -1);
}

/*
 * do_tell -- call pmemlog_tell() & print result
 */
static void
do_tell(PMEMlogpool *plp)
{
	os_off_t tell = pmemlog_tell(plp);
	UT_OUT("tell %zu", tell);
}

/*
 * do_rewind -- call pmemlog_rewind() & print result
 */
static void
do_rewind(PMEMlogpool *plp)
{
	pmemlog_rewind(plp);
	UT_OUT("rewind");
}

/*
 * printit -- print out the 'buf' of length 'len'.
 *
 * It is a walker function for pmemlog_walk
 */
static int
printit(const void *buf, size_t len, void *arg)
{
	char *str = MALLOC(len + 1);

	strncpy(str, buf, len);
	str[len] = '\0';
	UT_OUT("%s", str);

	FREE(str);

	return 1;
}

/*
 * do_walk -- call pmemlog_walk() & print result
 *
 * pmemlog_walk() is called twice: for chunk size 0 and 16
 */
static void
do_walk(PMEMlogpool *plp)
{
	pmemlog_walk(plp, 0, printit, NULL);
	UT_OUT("walk all at once");
	pmemlog_walk(plp, 16, printit, NULL);
	UT_OUT("walk by 16");
}

int
main(int argc, char *argv[])
{
	PMEMlogpool *plp;
	int result;

	START(argc, argv, "log_basic");

	if (argc < 3)
		UT_FATAL("usage: %s file-name op:n|a|v|t|r|w", argv[0]);

	const char *path = argv[1];

	if ((plp = pmemlog_create(path, 0, S_IWUSR | S_IRUSR)) == NULL)
		UT_FATAL("!pmemlog_create: %s", path);

	/* go through all arguments one by one */
	for (int arg = 2; arg < argc; arg++) {
		/* Scan the character of each argument. */
		if (strchr("navtrw", argv[arg][0]) == NULL ||
				argv[arg][1] != '\0')
			UT_FATAL("op must be n or a or v or t or r or w");

		switch (argv[arg][0]) {
		case 'n':
			do_nbyte(plp);
			break;

		case 'a':
			do_append(plp);
			break;

		case 'v':
			do_appendv(plp);
			break;

		case 't':
			do_tell(plp);
			break;

		case 'r':
			do_rewind(plp);
			break;

		case 'w':
			do_walk(plp);
			break;
		}
	}

	pmemlog_close(plp);

	/* check consistency again */
	result = pmemlog_check(path);
	if (result < 0)
		UT_OUT("!%s: pmemlog_check", path);
	else if (result == 0)
		UT_OUT("%s: pmemlog_check: not consistent", path);

	DONE(NULL);
}
