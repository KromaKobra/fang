{
  description = "Fang — a lightweight, fully customizable Wayland-first file manager";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAll = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      devShells = forAll (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            cmake
            ninja
            gcc
            pkg-config
            qt6.qtbase
            qt6.qtdeclarative
            qt6.qtwayland
            tomlplusplus
          ];
          # Let the Qt tools (qmlcachegen etc.) and tests run inside the shell.
          shellHook = ''
            export QT_QPA_PLATFORM=''${QT_QPA_PLATFORM:-wayland;xcb}
          '';
        };
      });

      packages = forAll (pkgs: {
        default = pkgs.stdenv.mkDerivation {
          pname = "fang";
          version = "0.1.0";
          src = self;
          nativeBuildInputs = with pkgs; [ cmake ninja pkg-config qt6.wrapQtAppsHook ];
          buildInputs = with pkgs; [
            qt6.qtbase
            qt6.qtdeclarative
            qt6.qtwayland
            tomlplusplus
          ];
          doCheck = true;
          checkPhase = ''
            runHook preCheck
            QT_QPA_PLATFORM=offscreen ctest --output-on-failure
            runHook postCheck
          '';
        };
      });
    };
}
