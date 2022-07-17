/* xsh_echo.c - xsh_echo */

#include <xinu.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * xhs_echo - write argument strings to stdout
 *------------------------------------------------------------------------
 */
shellcmd xsh_echo(int nargs, char *args[])
{
	int32	i;			/* walks through args array	*/

	if (nargs > 1) {
		syscall_fprintf(CONSOLE, "%s", args[1]);

		for (i = 2; i < nargs; i++) {
			syscall_fprintf(CONSOLE, " %s", args[i]);
		}
	}
	syscall_fprintf(CONSOLE, "\n");

	return 0;
}
