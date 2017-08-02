# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.
add_executable ( dsmccgen glib-min.h dsmcc.c main.c sdvserver.c servicegroup.c settopbox.c )
