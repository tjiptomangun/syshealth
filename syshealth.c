/*
 * xsyshealth.c
 *
 *  Created on: Mar 17, 2011
 *      Author: henky
 */
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MINIMUM_SLEEP_SECS 3600
int FileSize(FILE *FInFp);
int AppendTempToLog(FILE *fpLog,FILE *fpTemp,int nLogType,char *szSrc,char *szDst);
int AppendFileToLog(FILE *fpLog,char *fname,int nLogType,char *szSrc,char *szDst);
int ExecuteAndLog(char *system,char *tempfile,FILE *logptr,int logType, char *src,char *dst);
int reprocess(char *procname);
#define MAX_BUFFSIZE 80
#define MAX_SYSTEMSIZE 280
int        bHelp=0,opt;
int        zombie_check;
int        uptime_check;
int        df_H_check;
int        free_check;
int        vmstat_check;
int        users_check;
int        lsof_check;
int        loadavg_check;
int        diskstat_check;
int        netstat_s_check;
int        netstat_naptu_check;
int	   top_check;
int        iotop_check;
int        ps_axl_check;
int        ps_axu_check;
char       logdir[MAX_BUFFSIZE];
int        days_to_compress;
int        days_to_delete;
int        random_sleep;
unsigned   xseed;
unsigned   *seed = &xseed;

void initrand(){
	  *seed = time(NULL);
}

int randomnumber(){
	 *seed *= 1103515245;
	 *seed += 12345;
	 return ((*seed) / 65536) % 32768;
}

int wakeup(char *procname) {
	int nextsleep = 0;
	reprocess(procname);
	nextsleep = randomnumber(*seed) % MINIMUM_SLEEP_SECS;
	sleep(MINIMUM_SLEEP_SECS + nextsleep);
	wakeup(procname);
	return 0;
}

int reprocess(char *procname){
	time_t     now;
	time_t     then;
	struct tm  *ts;
	struct tm  *tsthen;
	char       buf[MAX_BUFFSIZE];
	char       fname[MAX_BUFFSIZE];
	char       tname[MAX_BUFFSIZE];
	char       aname[MAX_BUFFSIZE];
	char       gname[MAX_BUFFSIZE];
	FILE       * fp=NULL; 
	struct     stat st;
	
	memset(buf, 0, MAX_BUFFSIZE);
	memset(fname, 0, MAX_BUFFSIZE);
	memset(tname, 0, MAX_BUFFSIZE);

	now = time(NULL);
	ts = localtime(&now);
	tname[0]='\0';

	if (!strcmp(".", logdir))
		logdir[0]=0;

	sprintf(&tname[0],"%s%s.tmp",logdir, procname);
	sprintf(buf,"%s%s_%%m%%d",logdir, procname);
	strftime(fname, sizeof(buf), buf, ts);

	fp = fopen((const char *)fname,"a");

	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
	fprintf(fp,"************ Executed %s ************\n", buf);
	if (uptime_check)
		ExecuteAndLog("uptime",tname,fp,0,NULL,NULL);

	if (zombie_check){
		fprintf(fp,"\ncheck for zombies");
		ExecuteAndLog("ps -ef|grep -i defun | grep -v grep",tname,fp,0,NULL,NULL);
	}

	if (df_H_check){

#ifdef _RHEL_AS4_
		ExecuteAndLog("df -H",tname,fp,0,NULL,NULL);
#endif
#ifdef _aix_51_
		ExecuteAndLog("df -k",tname,fp,0,NULL,NULL);
#endif
	}

	if (free_check){
#ifdef _RHEL_AS4_
		ExecuteAndLog("free",tname,fp,0,NULL,NULL);
#endif
#ifdef _aix_51_
		ExecuteAndLog("lsattr -El `lsdev -C|grep mem | sed -e \'s/^\\([[:alnum:]][[:alnum:]]*\\) *.*/\\1 /g\'`",tname,fp,0,NULL,NULL);
#endif
	}

	if (vmstat_check){
		ExecuteAndLog("vmstat",tname,fp,0,NULL,NULL);
	}

	if (users_check){
		ExecuteAndLog("users",tname,fp,0,NULL,NULL);
	}

	if (lsof_check){
		ExecuteAndLog("lsof | wc -l",tname,fp,0,NULL,NULL);
	}
#ifdef _RHEL_AS4_
	if (loadavg_check){
		ExecuteAndLog("cat /proc/loadavg",tname,fp,0,NULL,NULL);
	}

	if (diskstat_check){
		ExecuteAndLog("cat /proc/diskstats",tname,fp,0,NULL,NULL);
		//in aix this value has been achieved in uptime syscal
	}
#endif
	if (netstat_s_check){
		ExecuteAndLog("netstat -s",tname,fp,0,NULL,NULL);
	}

	if (netstat_naptu_check){
		ExecuteAndLog("netstat -naptu",tname,fp,0,NULL,NULL);
	}

	if (top_check)
		ExecuteAndLog("top -bn1",tname,fp,0,NULL,NULL);

	if (iotop_check)
		ExecuteAndLog("iotop -bn1",tname,fp,0,NULL,NULL);

	if (ps_axl_check)
		ExecuteAndLog("ps axl",tname,fp,0,NULL,NULL);

	if (ps_axu_check)
		ExecuteAndLog("ps axu",tname,fp,0,NULL,NULL);


	
	if(days_to_compress){
		then = now - (days_to_compress * 86400);
		memset(aname, 0, MAX_BUFFSIZE);
		memset(gname, 0, MAX_BUFFSIZE);
		memset(buf, 0, MAX_BUFFSIZE);
		tsthen = localtime(&then);
		sprintf(buf,"%s%s_%%m%%d",logdir, procname);
		strftime(aname, sizeof(buf), buf, tsthen);
		if (-1 != stat(aname, &st)){
			sprintf(gname, "%s.tar.gz", aname);
			fprintf(stdout, "%s\n", aname);
			memset(buf, 0, MAX_BUFFSIZE);
			sprintf(buf, "tar -czf %s %s", gname, aname);
			ExecuteAndLog(buf,tname,fp,0,NULL,NULL);
			memset(buf, 0, MAX_BUFFSIZE);
			sprintf(buf, "rm  %s", aname);
			ExecuteAndLog(buf,tname,fp,0,NULL,NULL);		  
		}
	}
			
	if(days_to_delete){
		then = now - (days_to_delete * 86400);
		memset(aname, 0, MAX_BUFFSIZE);
		memset(gname, 0, MAX_BUFFSIZE);
		memset(buf, 0, MAX_BUFFSIZE);
		tsthen = localtime(&then);
		sprintf(buf,"%s%s_%%m%%d",logdir, procname);
		strftime(aname, sizeof(buf), buf, tsthen);
		if (-1 != stat(aname, &st)){
			sprintf(gname, "%s.tar.gz", aname);
			fprintf(stdout, "%s\n", aname);
			memset(buf, 0, MAX_BUFFSIZE);
			sprintf(buf, "rm %s", gname);
			ExecuteAndLog(buf,tname,fp,0,NULL,NULL);
		}
	}
			
	now = time(NULL);
	ts = localtime(&now);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
	fprintf(fp,"************ End on %s ************\n",buf);

	fclose(fp);
	remove (tname);
	return 0;

}


/*
 * NAME		: Daemonize
 * DESCRIPTION	: daemonize this process
 */
int Daemonize()
{
	pid_t pid;

	/* create new process */
	pid = fork ();
	if (pid == -1)
		exit (-1);
	else if (pid != 0)
		exit (EXIT_SUCCESS);
	
	/* create new session and process group */
	if (setsid() == -1)
		exit (-1); 

	/* close all open files, NR_OPEN and getdtablesize are neither posix standard */ 
	close(0);


	return 0;

} /* Daemonize */
int main(int argc, char *argv[])
{
	while ((opt = getopt(argc, argv, "hd:zUHfVuovsnNtTlxg:rD:")) != -1) {
		switch (opt) {
		case 'h':
			bHelp = 1;
			break;
		case 'd':
			logdir[0]='\0';
			strcpy(logdir,optarg);
			break;
		case 'z':
			zombie_check = 1;
			break;
		case 'U':
			uptime_check = 1;
			break;
		case 'H':
			df_H_check = 1;
			break;
		case 'f':
			free_check = 1;
			break;
		case 'V':
			vmstat_check = 1;
			break;
		case 'u':
			users_check = 1;
			break;
		case 'o':
			lsof_check = 1;
			break;
		case 'v':
			loadavg_check = 1;
			break;
		case 's':
			diskstat_check = 1;
			break;
		case 'n':
			netstat_s_check = 1;
			break;
		case 'N':
			netstat_naptu_check = 1;
			break;
		case 't':
			top_check = 1;
			break;
		case 'T':
			iotop_check = 1;
			break;
		case 'l':
			ps_axl_check = 1;
			break;
		case 'x':
			ps_axu_check = 1;
			break;
		case 'g':
			days_to_compress = atoi(optarg);
			break;
		case 'r':
			random_sleep = 1;
			break;
		case 'D':
			days_to_delete= atoi(optarg);
			break;
			
		
	        default: /* '?' */
	            fprintf(stderr, "Usage: %s [-h] for help\n", argv[0]);
	            exit(EXIT_FAILURE);
	        }
	}
	if(bHelp==1)
	{
#ifdef _RHEL_AS4_
		printf("usage: %s -[Options]\n", argv[0]);
		printf("Options:\n");
		printf("h : print this check\n");
		printf("d : result directory, default pwd\n");
		printf("z : check for zombies\n");
		printf("U : check for uptime\n");
		printf("H : check df -H\n");
		printf("f : check free mem\n");
		printf("V : check vmstat\n");
		printf("u : check users\n");
		printf("o : check lsof\n");
		printf("v : check loadavg\n");
		printf("s : check diskstat\n");
		printf("n : check netstat -S\n");
		printf("N : check netstat -naptu\n"); 
		printf("t : check top\n"); 
		printf("T : check iotop\n"); 
		printf("l : check ps axl\n"); 
		printf("x : check ps axu\n"); 
		printf("g : days to compress\n");
		printf("D : days to delete\n");
		printf("r : random sleep\n"); 
	
		printf("vmstat legend:\n");
		printf("r: The number of processes waiting for run time\n");
		printf("b: The number of processes in uninterruptible sleep.\n");
		printf("swpd: the amount of virtual memory used.\n");
		printf("free: the amount of idle memory.\n");
		printf("buff: the amount of memory used as buffers.\n");
		printf("cache: the amount of memory used as cache.");
		printf("si: Amount of memory swapped in from disk (/s).\n");
		printf("so: Amount of memory swapped to disk (/s).\n");
		printf("bi: Blocks received from a block device (blocks/s).\n");
		printf("bo: Blocks sent to a block device (blocks/s).\n");
		printf("in: The number of interrupts per second, including the clock.\n");
		printf("cs: The number of context switches per second.\n");
		printf("us: Time spent running non-kernel code. (user time, including nice time)\n");
		printf("sy: Time spent running kernel code. (system time)\n");
		printf("id: Time spent idle. Prior to Linux 2.5.41, this includes IO-wait time.\n");
		printf("wa: Time spent waiting for IO. Prior to Linux 2.5.41, included in idle.\n");
		printf("st: Time stolen from a virtual machine. Prior to Linux 2.6.11, unknown.\n");
		printf("\n\n");
#endif
#ifdef _aix_51_
		printf("usage: %s -[Options]\n", argv[0]);
		printf("Options:\n");
		printf("h : print this check\n");
		printf("d : result directory, default pwd\n");
		printf("z : check for zombies\n");
		printf("U : check for uptime\n");
		printf("H : check df -H\n");
		printf("f : check free mem\n");
		printf("V : check vmstat\n");
		printf("u : check users\n");
		printf("o : check lsof\n");
		printf("v : check loadavg\n");
		printf("s : check diskstat\n");
		printf("n : check netstat -S\n");
		printf("N : check netstat -naptu\n"); 
		printf("t : check top\n"); 
		printf("T : check iotop\n"); 
		printf("l : check ps axl\n"); 
		printf("x : check ps axu\n"); 
		printf("g : days to compress\n"); 
		printf("D : days to delete\n");
		printf("r : random sleep\n"); 
	
		printf("vmstat legend:\n");
		printf("kthr: kernel thread state changes per second over the sampling interval.\n");
		printf("r Number of kernel threads placed in run queue.\n");
		printf("b Number of kernel threads placed in wait queue (awaiting resource, awaiting input/output).");
		printf("Memory: information about the usage of virtual and real memory. Virtual pages are considered active if they have been accessed. A page is 4096 bytes.");
		printf("avm Active virtual pages.\n");
		printf("fre Size of the free list.");
		printf("Page: information about page faults and paging activity. These are averaged over the interval and given in units per second.");
		printf("re Pager input/output list.\n");
		printf("pi Pages paged in from paging space.\n");
		printf("po Pages paged out to paging space.\n");
		printf("fr Pages freed (page replacement).\n");
		printf("sr Pages scanned by page-replacement algorithm.\n");
		printf("cy Clock cycles by page-replacement algorithm.\n");
		printf("Faults: trap and interrupt rate averages per second over the sampling interval.");
		printf("in Device interrupts.\n");
		printf("sy System calls.\n");
		printf("cs Kernel thread context switches.\n");
		printf("Cpu: breakdown of percentage usage of CPU time.\n");
		printf("us User time.\n");
		printf("sy System time.\n");
		printf("id CPU idle time\n");
		printf("wa CPU idle time during which the system had outstanding disk/NFS I/O request(s). See detailed description above.\n");
		printf("\n");
#endif
	}
	else if (random_sleep){
		Daemonize();
		initrand(&seed);
		wakeup(argv[0]);
	}
	else 
		reprocess(argv[0]);

	return 0; 
}

int AppendTempToLog(FILE *fpLog,FILE *fpTemp,int nLogType,char *szSrc,char *szDst)
{
	int nSize=0;
	char cTemp;
	if(fpTemp)
	{
		nSize=FileSize(fpTemp);
		if(!nSize)
		{
			fprintf(fpLog,"****>>\tNo Log Found");
		}
		while(nSize--)
		{
			cTemp=getc(fpTemp);
			if((unsigned char)cTemp==0xff)
				continue;

			putc((int)cTemp,fpLog);
			if(cTemp=='\n' && (nSize-1))
			{

			}
		}

	}
	return (int)fpTemp;
}
int AppendFileToLog(FILE *fpLog,char *fname,int nLogType,char *szSrc,char *szDst)
{
	FILE *fpTmp=(FILE *)fopen(fname,"r");
	int nRet;
	if((nRet = AppendTempToLog(fpLog,fpTmp,nLogType,szSrc,szDst)))
	{
		fclose(fpTmp);
	}
	return nRet;
}
int FileSize(FILE *FInFp)
{
	int nPos,nFSize;
	nPos=ftell(FInFp);
	fseek(FInFp,0L,SEEK_END);
	nFSize=ftell(FInFp);
	fseek(FInFp,nPos,SEEK_SET);
	return nFSize;
}
int ExecuteAndLog(char *systemcalled,char *tempfile,FILE *logptr,int logType, char *src,char *dst)
{
	char       strsystem[MAX_SYSTEMSIZE];
	fprintf(logptr,"\n>%s :\n",systemcalled);
	memset(strsystem,'0',MAX_SYSTEMSIZE);
	sprintf(strsystem,"%s>%s",systemcalled,tempfile);
	system(strsystem);
	return AppendFileToLog(logptr,tempfile,logType,src,dst);
}
