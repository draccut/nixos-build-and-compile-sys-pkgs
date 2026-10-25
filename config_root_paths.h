#ifndef _TEST_BUILD

#define PKG_ROOT "/etc/nixos/pkgs"
#define OVERLAY_FILE "/etc/nixos/overlays/my-overlay.nix"

#else

#define PKG_ROOT "/tmp/etc/nixos/pkgs"
#define OVERLAY_FILE "/tmp/etc/nixos/overlays/my-overlay.nix"

#endif
