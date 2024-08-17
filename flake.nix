{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat.url = "github:edolstra/flake-compat";
    flake-compat.flake = false;
  };

  outputs = inputs@{ self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = import nixpkgs { inherit system; };
      in rec {
        formatter = pkgs.nixpkgs-fmt;
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            formatter
            valgrind # test for memory errors & leaks
            bear # extract compile commands for lsp
            clang-tools # lsp (clangd)
            ccls # lsp
            pkg-config
            cmake
            samurai
          ];
          buildInputs = with pkgs; [ stduuid nlohmann_json ];
        };
        packages = rec {
          default = static;
          static = pkgs.pkgsStatic.callPackage ./pkg.nix { };
          dynamic = pkgs.callPackage ./pkg.nix { };
        };
      });
}
