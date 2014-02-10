
#include <signal.h>       /* for signal() and atexit() */
#include <sys/stat.h>     /* for open() */
#include <fcntl.h>        /* for open() */


#include "sdvserver.h"
#include "servicegroup.h"
#include "main.h"

extern int abort_request;

#define NOSRCID 999999

stb_t  stb;

stb_t* stbs;

char*  version = "2.0.2a";

#define MAX_SGS 200
gint           sgcount;
servicegroup_t sgs[MAX_SGS];

gint     servercount;
server_t servers[MAX_SGS];

void
usage( gchar* appname ) {
    printf( "  %s { -f <filename> | svm-ip service-group [ options | source-id ] }\n\n", appname );
    printf( "    sourceId becomes optional and not used if -s or --srcids options are specified.\n" );
    printf( "    options:\n" );
    printf( "      -b  | --stbbase number        the base address for a range of settops\n" );
    printf( "                                    utilizes the second mac octet, 1 to 255 - default 1.\n" );
    printf( "      -c  | --stbcnt number         the number of settops to simulate\n" );
    printf( "                                    utilizes last 4 mac octets; default 1.\n" );
//    printf( "      -d  | --dwell number          the tuned channel dwell time milliseconds - default 250.\n" );
    printf( "      -f  | --file filename         a configuration file specifing multiple cli invocations\n" );
    printf( "      -h  | --hold timems           hold off time before task is started. ie inter-task gap\n" );
    printf( "      -l  | --loop                  round robin the stb and srcids.\n" );
    printf( "      -n  | --noinit                skip stb initialization.\n" );
    printf( "      -r  | --rate timems           milliseconds between channel changes - default 250.\n" );
    printf( "      -s  | --srcids number number  range of source IDs begining to ending.\n" );
    printf( "      -z  seconds                   dwell on channel for n seconds.\n" );
    printf( "      -vi                           verbose dsmcc input display.\n" );
    printf( "      -vo                           verbose dsmcc output display.\n" );
    printf( "      -ve                           verbose dsmcc error display.\n" );
    printf( "      -vf                           verbose dsmcc failure display.\n" );
    printf( "      -vs                           verbose stb fsm display.\n" );
    printf( "      -vx                           verbose stb fsm abnormal display.\n" );
    printf( "      -vd                           dump stb info on termination.\n" );
//    printf( "\n\n     the channel change rate will cause a backlog of requests if stb count is large\n" );
//    printf( "       and / or dwell is too small.\n" );
    printf( "\n    A source id = 0 will cause the stb to sign-off and free up OTS resources.\n" );
    printf( "\n    file format is currently one option sequence per line begining with cli;\n" );
    printf( "       i.e. cli sdv4 922 -b 2 -c 80 - r 400 -s 64001 64500 -n -l.\n\n" );
}


gboolean
process_files( gchar* fileptr ) {
#define PROCESS_BUFF_SIZE 200
    gint fd;
    gchar buff[ PROCESS_BUFF_SIZE ];
    gchar* argv[ PROCESS_BUFF_SIZE ];
    gint  argc;
    gint  rdcount;

    gint i;

    gchar* chrptr;

    if ( fileptr == NULL ) {
        return FALSE;
    }

    printf( " filename: %s\n", fileptr );
    fd = open( fileptr, O_RDONLY );
    if ( fd == -1 ) {
        perror( "process_file" );
        return FALSE;
    }

    const char* delims = " \r\n";

    do {
        /* clear buff for clean - NULL - testing */
        memset( buff, 0, PROCESS_BUFF_SIZE );

        /* prep the pointer to 1 less due to pre-inc */
        /* read one line at a time or the EOF */
        chrptr = buff - 1;
        do {
            rdcount = read( fd, ++chrptr, 1 );
        }
        while ( rdcount > 0 &&  *chrptr != '\r' && *chrptr != '\n'  );

        /* tokenize the newly stuffed buffer */
        argc = 0;
        argv[argc] = strtok( buff, delims );
        while ( argv[argc] !=  NULL ) {
            argv[++argc] = strtok( NULL, delims ) ;
        }

        /* check arg count before testing first arg for valid cli */
        if ( argc > 0 && strncmp( argv[0], "cli", 3 ) == 0 ) {
            if ( !init_and_parse_cli( argc, argv, sgcount ) ) {
                close( fd );
                return FALSE;
            }
        }
    }
    while ( rdcount != 0 );

    close( fd );
    printf( " file done\n" );
    return TRUE;
}

gboolean
init_and_parse_cli( gint argcnt, gchar* argstrs[], int sgndx ) {
    gint     i; 
    gint     j; 
    gint     len;
    guint32* p_i;

    sgs[sgndx].stbbase      = 1;
    sgs[sgndx].stbcnt       = 1;

    sgs[sgndx].holdoff      = 250000; /* usec */
    sgs[sgndx].rate         = 250000; /* usec */
    sgs[sgndx].dwell        = 0; /* sec */

    sgs[sgndx].srvrptr      = NULL;
    sgs[sgndx].servicegroup = 0;

    sgs[sgndx].srcidmin     = NOSRCID;
    sgs[sgndx].srcidmax     = NOSRCID;

    sgs[sgndx].flags        = 0;

    gint opt_parm;



    printf( "   " );
    for ( i = 0; i < argcnt; ++i ) {
        printf( " %s", argstrs[i] );
    }

    opt_parm = 2;
    for ( i = 1; i < argcnt; ++i ) {
        //printf( "ndx.i = %i.%i %s\n", sgndx, i, argstrs[i] );
        if ( strncmp( argstrs[i],  "-f", 2 ) == 0 || strncmp( argstrs[i],  "--file", 6 ) == 0 ) {
            /* here if using the file configuration option */
            {
                /* here if need to process a config file */
                if ( !process_files( argstrs[ opt_parm ] ) ) {
                    printf( "file error\n" );
                    return FALSE;
                }
                ++i;
                opt_parm += 2;
            }
        }
        else if ( strncmp( argstrs[i], "-b", 2 ) == 0 || strncmp( argstrs[i], "--stbbase", 9 ) == 0 ) {
            if ( i + 1 < argcnt ) {
                sgs[sgndx].stbbase = atoi( argstrs[++i] );
            }
            else {
                return FALSE;
            }
        }
        else if ( strncmp( argstrs[i],  "-c", 2 ) == 0 || strncmp( argstrs[i],  "--stbcnt", 8 ) == 0 ) {
            if ( i + 1 < argcnt ) {
                sgs[sgndx].stbcnt = atoi( argstrs[++i] );
            }
            else {
                return FALSE;
            }
        }
        else if ( strncmp( argstrs[i],  "-d", 2 ) == 0 || strncmp( argstrs[i],  "--dwell", 7 ) == 0 ) {
            if ( i + 1 < argcnt ) {
                sgs[sgndx].dwell = atoi( argstrs[++i] ) * 1000;
            }
            else {
                return FALSE;
            }
        }
        else if ( strncmp( argstrs[i],  "-l", 2 ) == 0 || strncmp( argstrs[i],  "--loop", 6 ) == 0 ) {
            sgs[sgndx].flags |= LOOPFLG;
        }
        else if ( strncmp( argstrs[i],  "-n", 2 ) == 0 || strncmp( argstrs[i],  "--noinit", 8 ) == 0 ) {
            sgs[sgndx].flags |= NOINITFLG;
        }
        else if ( strncmp( argstrs[i],  "-vi", 3 ) == 0 ) {
            sgs[sgndx].flags |= VERBOSEIN;
        }
        else if ( strncmp( argstrs[i],  "-vo", 3 ) == 0 ) {
            sgs[sgndx].flags |= VERBOSEOUT;
        }
        else if ( strncmp( argstrs[i],  "-ve", 3 ) == 0 ) {
            sgs[sgndx].flags |= VERBOSEERROR;
        }
        else if ( strncmp( argstrs[i],  "-vf", 3 ) == 0 ) {
            sgs[sgndx].flags |= VERBOSEFAIL;
        }
        else if ( strncmp( argstrs[i],  "-vs", 3 ) == 0 ) {
            sgs[sgndx].flags |= DBGFSMFULL;
        }
        else if ( strncmp( argstrs[i],  "-vx", 3 ) == 0 ) {
            sgs[sgndx].flags |= DBGFSMABN;
        }
        else if ( strncmp( argstrs[i],  "-vd", 3 ) == 0 ) {
            sgs[sgndx].flags |= STBDUMP;
        }
        else if ( strncmp( argstrs[i],  "-h", 2 ) == 0 || strncmp( argstrs[i],  "--hold", 6 ) == 0 ) {
            if ( i + 1 < argcnt ) {
                sgs[sgndx].holdoff = atoi( argstrs[++i] ) * 1000;
            }
            else {
                return FALSE;
            }
        }
        else if ( strncmp( argstrs[i],  "-s", 2 ) == 0 || strncmp( argstrs[i],  "--srcids", 8 ) == 0 ) {
            if ( i + 2 < argcnt ) {
                sgs[sgndx].srcidmin = atoi( argstrs[++i] );
                sgs[sgndx].srcidmax = atoi( argstrs[++i] );
            }
            else {
                return FALSE;
            }
        }
        else if ( strncmp( argstrs[i],  "-r", 2 ) == 0 || strncmp( argstrs[i],  "--rate", 6 ) == 0 ) {
            if ( i + 1 < argcnt ) {
                sgs[sgndx].rate = atoi( argstrs[++i] ) * 1000;    /* microseconds */
            }
            else {
                return FALSE;
            }
        }
        else if ( strncmp( argstrs[i],  "-z", 2 ) == 0 ) {
            if ( i + 1 < argcnt ) {
                sgs[sgndx].dwell = atoi( argstrs[++i] );    /* seconds */
            }
            else {
                return FALSE;
            }
        }
        else if ( sgs[sgndx].srvrptr == NULL ) {
            /* here if no options and server is not yet spec'd */
            /* resolve the server reference or add a new server reference */
            len = strlen( argstrs[i] );
            for ( j = 0; j < servercount; ++j ) {
                if ( strncmp( argstrs[i], servers[j].svmip , len ) == 0 ) {
                    break;
                }
            }

            if ( j == servercount ) {
                /* here if server not previously specified */
                if ( !set_ip( &servers[j], argstrs[i] ) ) {
                    return FALSE;
                }

                ++servercount;
            }

            sgs[sgndx].srvrptr = &servers[j];
        }
        else if ( sgs[sgndx].servicegroup == 0 ) {
            /* here if server is defined, no opt and no service group */
            sgs[sgndx].servicegroup = atoi( argstrs[i] );

            if ( sgs[sgndx].servicegroup == 0 ) {
                printf( "service group is illegal value (%i)\n",
                          sgs[sgndx].servicegroup );
                return FALSE;
            }

            ++sgcount;
        }
        else if ( sgs[sgndx].srcidmin == NOSRCID ) {
            /* here if svr and sg spec'd and no other option specified */
            sgs[sgndx].srcidmin = sgs[sgndx].srcidmax = atoi( argstrs[i] );
        }
        else {
            printf( "processing error in command line args\n" );
        }
    }

    if ( sgs[sgndx].srcidmin == NOSRCID || sgs[sgndx].srcidmax == NOSRCID ) {
        printf( ">>> No sourceId(s)\n\n" );
        return FALSE;
    }
    printf( "  validated\n" );


    return TRUE;
}

/* sighandler_t */
void
sig_int( int s ) {
    printf( "Abort received\n" );
    abort_request = 1;
}

void
cleanup( ) {
    gint i;

    for ( i = 0; i < sgcount; ++i ) {
        close_channel( sgs[i].srvrptr );
        /*sg_free(  &sgs[i] );*/ /* causes a double free */
    }

    if ( abort_request != 0 ) {
        printf( "aborted exit\n\n" );
    }
}

int
main( gint argcnt, char* argstrs[] ) {
    gint i;
    gint j;
    gint exitcode;

    gchar* appname;

    appname = argstrs[0];

    servercount = 0;
    sgcount = 0;

    abort_request = 0;

    /* register functions to capture ^c and exit */
    signal( SIGINT, sig_int );
    atexit( cleanup );

    printf( "\n  %s version %s.\n", appname, version );
    printf( "  Charles Donaldson charlesdonaldson@motorola.com.\n\n" );

    if ( !init_and_parse_cli( argcnt, argstrs, 0 ) ) {
        usage( appname );
        return 1;
    }

    //printf( "sgcount %i\n", sgcount );

    /* init service groups */
    for ( i = 0; i < sgcount; ++i ) {
        if ( !sg_init( &sgs[i], FALSE ) ) {
            printf( "service group init failed\n" );
            exit ( 1 );
        }
    }

    /* start the threads */
    for ( i = 0; i < sgcount; ++i ) {
        usleep( sgs[i].holdoff );
        pthread_create( &sgs[i].thread, NULL, sg_run_task, ( void* )&sgs[i] );
    }

    /* wait for the threads to end */
    for ( i = 0; i < sgcount; ++i ) {
        pthread_join( sgs[i].thread, NULL );
    }

    /* look for failure conditions in all service groups and stbs */
    exitcode = 0;
    for ( i = 0; i < sgcount; ++i ) {
        for ( j = 0; j < sgs[i].stbcnt; ++j ) {
            if ( sgs[i].stbbegin[j].tunefailcnt > 0 ) {
                printf( "server timeout failure on: " );
                print_stb( &( sgs[i].stbbegin[j] ) );
                printf( "\n" );
                exitcode = 1;
            }
        }
    }

    return exitcode;
}
