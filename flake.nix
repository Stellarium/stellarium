
{
  description = "Stellarium dev shell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";

  outputs = { self, nixpkgs }:
  let
    systems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];

    forAllSystems = nixpkgs.lib.genAttrs systems;
  in
  {
    devShells = forAllSystems (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        isDarwin = pkgs.stdenv.isDarwin;
      in
      {
        default = pkgs.mkShell {
          packages =
            [
              pkgs.clang
              pkgs.cmake
              pkgs.ninja
              pkgs.pkg-config
              pkgs.git
              pkgs.gettext

              pkgs.qt6.qtbase
              pkgs.qt6.qtbase.dev
              pkgs.qt6.qttools
              pkgs.qt6.qtsvg
              pkgs.qt6.qtcharts
              pkgs.qt6.qtmultimedia
              pkgs.qt6.qtpositioning
              pkgs.qt6.qtserialport
              pkgs.qt6Packages.qxlsx

              pkgs.zlib
              pkgs.exiv2
              pkgs.nlopt
              pkgs.md4c
            ];

          shellHook = ''
            export NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -F${pkgs.qt6.qtserialport}/lib"
            export NIX_LDFLAGS="$NIX_LDFLAGS -F${pkgs.qt6.qtserialport}/lib"
            echo "Stellarium build-doc dev shell"
            echo "clang:  $(clang --version | head -n1)"
            echo "cmake:  $(cmake --version | head -n1)"
            echo "qtpaths: $(which qtpaths6 || true)"

            export CMAKE_PREFIX_PATH=${pkgs.qt6.qtbase}
          '';
        };
      }
    );
  };
}

