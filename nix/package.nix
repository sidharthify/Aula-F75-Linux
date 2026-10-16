{ lib
, stdenv
, cmake
, ninja
, qt6
, src ? ../.
}:

let
  cleanSrc = lib.cleanSourceWith {
    inherit src;
    filter = path: type:
      let base = baseNameOf path;
      in base != "build" && base != ".direnv" && base != "result";
  };
in
stdenv.mkDerivation (finalAttrs: {
  pname = "aula-f75";
  version = "1.0.0";

  src = cleanSrc;

  nativeBuildInputs = [
    cmake
    ninja
    qt6.wrapQtAppsHook
  ];

  buildInputs = [
    qt6.qtbase
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
  ];

  meta = with lib; {
    description = "Native Linux/KDE configurator for the AULA F75 keyboard";
    longDescription = ''
      Controls the lighting, on-board effects, key matrix and settings of the
      AULA F75 keyboard by speaking its vendor HID protocol directly, with no
      Wine or vendor driver required.
    '';
    homepage = "https://github.com/sidharthify/Aula-F75-Linux";
    license = licenses.gpl3Plus;
    maintainers = [ ];
    platforms = platforms.linux;
    mainProgram = "aula-f75";
  };
})
