#ifndef RECREATE_DIR_H
#define RECREATE_DIR_H

#ifndef _TEST_BUILD
#define PKG_ROOT_FOLDERNAME "/etc/nixos/pkgs"
#else
#define PKG_ROOT_FOLDERNAME "/tmp/etc/nixos/pkgs"
#endif

int recreate_dir();

#endif
