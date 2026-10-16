{
  description = "Native Linux/KDE configurator for the AULA F75 keyboard";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (pkgs: {
        default = pkgs.callPackage ./nix/package.nix { src = self; };
        aula-f75 = pkgs.callPackage ./nix/package.nix { src = self; };
      });

      nixosModules.default = import ./nix/module.nix;
      nixosModules.aula-f75 = import ./nix/module.nix;

      overlays.default = final: prev: {
        aula-f75 = final.callPackage ./nix/package.nix { src = self; };
      };
    };
}
