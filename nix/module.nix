{ config, pkgs, lib, ... }:

let
  cfg = config.programs.aula-f75;
in
{
  options.programs.aula-f75 = {
    enable = lib.mkEnableOption "the AULA F75 keyboard configurator";

    package = lib.mkOption {
      type = lib.types.package;
      default = pkgs.callPackage ./package.nix { };
      defaultText = lib.literalExpression "pkgs.callPackage ./package.nix { }";
      description = "The aula-f75 package to install.";
    };

    udevRules = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = ''
        Install udev rules granting the "input" group access to the AULA F75
        hidraw nodes (both the wired 258a:010c and the Compx 3554:fa09
        wireless receiver).
      '';
    };
  };

  config = lib.mkIf cfg.enable {
    environment.systemPackages = [ cfg.package ];

    services.udev.extraRules = lib.mkIf cfg.udevRules ''
      # AULA F75 keyboard config interface
      SUBSYSTEM=="hidraw", ATTRS{idVendor}=="258a", ATTRS{idProduct}=="010c", GROUP="input", MODE="0660"
      SUBSYSTEM=="hidraw", ATTRS{idVendor}=="3554", ATTRS{idProduct}=="fa09", GROUP="input", MODE="0660"
    '';
  };
}
