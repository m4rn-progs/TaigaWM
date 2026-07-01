{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
    nativeBuildInputs = with pkgs; [
        clang
        pkg-config
        wayland-scanner
        bear
        ninja
        meson
        cmake
    ];

    buildInputs = with pkgs; [
        lua5_4
        wayland
        wayland-protocols
        libxkbcommon
        libinput
    ];

    shellHook = ''
        export CC=clang
    '';
}
