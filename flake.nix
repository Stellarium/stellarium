{
  description = "Stellarium dev shell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";

  outputs = { self, nixpkgs }:
  let
    systems = [ "aarch64-darwin" ];
    forAllSystems = nixpkgs.lib.genAttrs systems;
  in
  {
    devShells = forAllSystems (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        qt = pkgs.qt6Packages;
      in
      {
        default = pkgs.mkShell {
          packages = [
            pkgs.clang
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
            pkgs.git
            pkgs.gettext

            qt.qtbase
            qt.qttools
            qt.qtsvg
            qt.qtcharts
            qt.qtmultimedia
            qt.qtpositioning
            qt.qtserialport
            qt.qtspeech
            qt.qxlsx

            pkgs.zlib
            pkgs.exiv2
            pkgs.nlopt
            pkgs.md4c
          ];

          shellHook = ''
            echo "Stellarium dev shell"
            echo "clang:                                   $(clang --version | head -n1)"
            echo "cmake:                                   $(cmake --version | head -n1)"
            echo "build with: cmake ../ -GNinja -DENABLE_QT6=ON -DENABLE_QTWEBENGINE=OFF"

            QT_FW_PATHS=(
              ${qt.qtbase}/lib
              ${qt.qtpositioning}/lib
              ${qt.qtserialport}/lib
              ${qt.qtmultimedia}/lib
              ${qt.qtcharts}/lib
              ${qt.qtsvg}/lib
            )

            for p in "''${QT_FW_PATHS[@]}"; do
              export NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE -F$p"
              export NIX_LDFLAGS="$NIX_LDFLAGS -F$p"
            done
          '';
        };
      });
  };
}
