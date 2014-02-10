
#include "dsmcc.h"

gchar vsbuff[120];

gchar*
val_to_string( guint32 value, const value_string vs[] ) {
    int i = 0;
    while ( vs[i].strptr != NULL ) {
        if ( value == vs[i].value ) {
            return vs[i].strptr;
        }
        ++i;
    }

    snprintf( vsbuff, sizeof vsbuff, "Unknown value %i (%X)", value, value );
    return vsbuff;
}

void
print_dsmcc( gchar dsmccptr[], gint len ) {
    int i;
    for ( i = 0; i < len; ++i ) {
        printf( "%02X", dsmccptr[i] );
    }

    printf( "\n" );
}
