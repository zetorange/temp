/* This is for testing the local getopt. */


#ifdef GETOPT_FROM_SYSTEM
#define dwgetopt  getopt
#define dwopterr  opterr
#define dwoptind  optind
#define dwoptopt  optopt
#define dwopterr  opterr
#define dwoptarg  optarg
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcmp */
#include "dwgetopt.h" /* for dwgetopt */
char *argv1[20];

/*  Use for testing dwgetopt only.
    Not a standard function. */
static void
dwgetoptresetfortestingonly(void)
{
   dwopterr   = 1;
   dwoptind   = 1;
   dwoptopt   = 0;
   dwoptreset = 0;
   dwoptarg   = 0;
}



/* for 'extra', read 'dwoptarg', for expextra read expected-optarg
   for optnum read dwoptind, for expnum read expected-optind
*/
static void
chkval(int ct,int c,int expchar,char *extra,char *expextra,
    int optnum,int expnum,const char *testid)
{
    int err = 0;
    if (c != expchar) {
        err++;
        printf("Mismatch %d %s: got char %c 0x%x exp char %c 0x%x\n",
            ct,testid,c,c,expchar,expchar);
    }
    if (extra != expextra) {
        /* pointers non-match */
        if (extra && expextra && (!strcmp(extra,expextra))) {
            /* strings match. */
        } else {
            err++;
            printf("Mismatch %d %s: got dwoptarg %s 0x%x exp optarg %s 0x%x\n",
                ct,testid,
                extra?extra:"",(unsigned)extra,
                expextra?expextra:"", (unsigned)expextra);
        }
    }
    if (optnum != expnum) {
        err++;
        printf("Mismatch %d %s: got dwoptind %d 0x%x exp optind %d 0x%x\n",
            ct,testid,optnum,optnum,expnum,expnum);
    }
    if (err > 0) {
        printf("FAIL getopttest %s\n",testid);
        exit(1);
    }
}

/* for 'extra', read 'dwoptarg', for expextra read expected-optarg
   for optnum read dwoptind, for expnum read expected-optind
chkval(int ct,int c,int expchar,char *extra,char *expextra,
    int optnum,int expnum,const char *testid) */

static int
test3(void)
{
    int ct = 1;
    int c = 0;
    int argct = 8;
    argv1[0]="a.out";
    argv1[1]="-a";
    argv1[2]="-#bx";
    argv1[3]="-b";
    argv1[4]="-c";
    argv1[5]="-cfoo";
    argv1[6]="-d";
    argv1[7]="progtoread";
    argv1[8]=0;
    for ( ;(c = dwgetopt(argct, argv1, "#:abc::d")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,2,"test31");
        break;
    case 2:
        chkval(ct,c,'#',dwoptarg,"bx",dwoptind,3,"test32");
        break;
    case 3:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,4,"test33");
        break;
    case 4:
        chkval(ct,c,'c',dwoptarg,0,dwoptind,5,"test34");
        break;
    case 5:
        chkval(ct,c,'c',dwoptarg,"foo",dwoptind,6,"test35");
        break;
    case 6:
        chkval(ct,c,'d',dwoptarg,0,dwoptind,7,"test36");
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    default:
        printf("FAIL test3 unexpected ct %d\n",ct);
    }
    }
#if 0
    printf(" final check: ct %d dwoptind %d\n",ct,optind);
#endif
    if (strcmp(argv1[dwoptind],"progtoread")) {
        printf("FAIL test3 on non-dash dwoptind %d arg got %s exp %s\n",
            dwoptind,argv1[dwoptind],"progtoread");
        exit(1);
    }
    printf("PASS getopt test 3\n");
    return 0;
}


static int
test2(void)
{
    int ct = 1;
    int c = 0;
    int argct = 5;
    argv1[0]="a.out";
    argv1[1]="-a";
    argv1[2]="-#pound";
    argv1[3]="-b";
    argv1[4]="-cfilename";
    argv1[5]=0;
    for ( ;(c = dwgetopt(argct, argv1,
        "#:abc::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,2,"test21");
        break;
    case 2:
        chkval(ct,c,'#',dwoptarg,"pound",dwoptind,3,"test22");
        break;
        break;
    case 3:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,4,"test23");
        break;
    case 4:
        chkval(ct,c,'c',dwoptarg,"filename",dwoptind,5,"test24");
        break;
    default:
        printf("FAIL test2 unexpected ct %d\n",ct);
        exit(1);
    }
    }
#if 0
    printf(" final check: ct %d dwoptind %d\n",ct,optind);
#endif
    if (argv1[dwoptind]) {
        printf("FAIL test2 on non-dash arg dwoptind %d got 0x%x exp NULL\n",
            dwoptind,(unsigned)argv1[dwoptind]);
        exit(1);
    }
    printf("PASS getopt test 2\n");
    return 0;
}


static int
test1(void)
{
    int ct = 1;
    int c = 0;
    int argct = 13;
    argv1[0]="a.out";
    argv1[1]="-a";
    argv1[2]="-#";
    argv1[3]="6";
    argv1[4]="-H1";
    argv1[5]="-H";
    argv1[6]="2";
    argv1[7]="-ka";
    argv1[8]="-l";
    argv1[9]="-lv";
    argv1[10]="-x";
    argv1[11]="path=./foo";
    argv1[12]="progtoread";
    argv1[13]=0;
    for ( ;(c = dwgetopt(argct, argv1,
        "#:abc::CdDeE::fFgGhH:iIk:l::mMnNo::O:pPqQrRsS:t:u:UvVwW::x:yz"))
        != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,2,"test11");
        break;
    case 2:
        chkval(ct,c,'#',dwoptarg,"6",dwoptind,4,"test12");
        break;
    case 3:
        chkval(ct,c,'H',dwoptarg,"1",dwoptind,5,"test13");
        break;
    case 4:
        chkval(ct,c,'H',dwoptarg,"2",dwoptind,7,"test14");
        break;
    case 5:
        chkval(ct,c,'k',dwoptarg,"a",dwoptind,8,"test15");
        break;
    case 6:
        chkval(ct,c,'l',dwoptarg,0,dwoptind,9,"test16");
        break;
    case 7:
        chkval(ct,c,'l',dwoptarg,"v",dwoptind,10,"test17");
        break;
    case 8:
        chkval(ct,c,'x',dwoptarg,"path=./foo",dwoptind,12,"test18");
        break;
    default:
        printf("FAIL test1 unexpected ct %d in test1\n",ct);
        exit(1);
    }
    }
#if 0
    printf(" final check: ct %d dwoptind %d\n",ct,optind);
#endif
    if (strcmp(argv1[dwoptind],"progtoread")) {
        printf("FAIL test1 on non-dash arg dwoptind %d got %s exp %s\n",
            dwoptind,argv1[dwoptind],"progtoread");
        exit(1);
    }
    printf("PASS getopt test1\n");
    return 0;
}

static int
test5(void)
{
    int ct = 1;
    int c = 0;
    int argct = 8;
    argv1[0]="a.out";
    argv1[1]="-ab";
    argv1[2]="-a";
    argv1[3]="-cx";
    argv1[4]="-c";
    argv1[5]="y";
    argv1[6]="-d";
    argv1[7]="-da";
    argv1[8]=0;
    for ( ;(c = dwgetopt(argct, argv1, "abc:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,1,"test51");
        break;
    case 2:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,2,"test52");
        break;
    case 3:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,3,"test53");
        break;
    case 4:
        chkval(ct,c,'c',dwoptarg,"x",dwoptind,4,"test54");
        break;
    case 5:
        chkval(ct,c,'c',dwoptarg,"y",dwoptind,6,"test55");
        break;
    case 6:
        chkval(ct,c,'d',dwoptarg,0,dwoptind,7,"test56");
        break;
    case 7:
        chkval(ct,c,'d',dwoptarg,"a",dwoptind,8,"test17");
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char 0x%x %c\n",ct,c,c);
        exit(1);
    }
    }
#if 0
    printf(" final check: ct %d dwoptind %d\n",ct,optind);
#endif
    if (argv1[dwoptind]) {
        printf("FAIL test5 there is a non-dash arg dwoptind %d got 0x%x\n",
            dwoptind,(unsigned)argv1[dwoptind]);
        exit(1);
    }
    printf("PASS getopt test5\n");
    return 0;
}

static int
test6(void)
{
    int ct = 1;
    int c = 0;
    int argct = 2;
    argv1[0]="a.out";
    argv1[1]="-H";
    argv1[2]=0;
    for ( ;(c = dwgetopt(argct, argv1, "abH:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'?',dwoptarg,0,dwoptind,2,"test61");
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char 0x%x %c\n",ct,c,c);
        exit(1);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test6 there is a non-dash arg dwoptind %d got 0x%x\n",
            dwoptind,(unsigned)argv1[dwoptind]);
        exit(1);
    }
    printf("PASS getopt test6\n");
    return 0;
}
/*  Leading : in opt string */
static int
test7(void)
{
    int ct = 1;
    int c = 0;
    int argct = 2;
    argv1[0]="a.out";
    argv1[1]="-H";
    argv1[2]=0;
    for ( ;(c = dwgetopt(argct, argv1, ":abH:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,':',dwoptarg,0,dwoptind,2,"test71");
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char 0x%x %c\n",ct,c,c);
        exit(1);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test7 there is a non-dash arg dwoptind %d got 0x%x\n",
            dwoptind,(unsigned)argv1[dwoptind]);
        exit(1);
    }
    printf("PASS getopt test7\n");
    return 0;
}
static int
test8(void)
{
    int ct = 1;
    int c = 0;
    int argct = 2;
    argv1[0]="a.out";
    argv1[1]="-x";
    argv1[2]=0;
    for ( ;(c = dwgetopt(argct, argv1, "abH:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'?',dwoptarg,0,dwoptind,2,"test81");
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char 0x%x %c\n",ct,c,c);
        exit(1);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test8 there is a non-dash arg dwoptind %d got 0x%x\n",
            dwoptind,(unsigned)argv1[dwoptind]);
        exit(1);
    }
    printf("PASS getopt test8\n");
    return 0;
}




int main(int argc, const char **argv)
{
    int ct = 0;
    int failct = 0;
    printf("argc: %d\n",argc);
    for( ct = 0; ct < argc ; ++ct) {
        printf("argv[%d] = %s\n",ct,argv[ct]);
    }
    if ( argc == 3) {
        int num = 0;
        if (strcmp(argv[1],"-c")) {
            printf("FAIL: invalid arg list\n");
            exit(1);
        }
        num = atoi(argv[2]);
        printf("Run one test, number %d\n",num);
        switch(num) {
        case 1:
            failct = test1();
            break;
        case 2:
            failct = test2();
            break;
        case 3:
            failct = test3();
            break;
        case 5:
            failct = test5();
            break;
        case 6:
            failct = test6();
            break;
        case 7:
            failct = test7();
            break;
        case 8:
            failct = test8();
            break;
        default:
            printf("FAIL: invalid test number %d\n",num);
            exit(1);
        }
        if ( failct) {
            printf("FAIL getopttest\n");
            exit(1);
        }
        printf("PASS getopttest\n");
        exit(0);
    } else if (argc != 1) {
        printf("FAIL: invalid arg list\n");
            exit(1);
    }
    failct += test5();
    dwgetoptresetfortestingonly();
    failct += test1();
    dwgetoptresetfortestingonly();
    failct += test2();
    dwgetoptresetfortestingonly();
    failct += test3();
    dwgetoptresetfortestingonly();
    failct += test6();
    dwgetoptresetfortestingonly();
    failct += test7();
    dwgetoptresetfortestingonly();
    failct += test8();
    if ( failct) {
        printf("FAIL getopttest\n");
        exit(1);
    }
    printf("PASS getopttest\n");
    return 0;
}
