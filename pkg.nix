{ lib, stdenv, cmake, stduuid, nlohmann_json }:
stdenv.mkDerivation {
  pname = "cpp-scripts";
  version = "0.1.0";
  src = ./.;
  dontStrip = false;
  stripAllList = [ "bin" ];
  buildInputs = [ stduuid nlohmann_json ];
  nativeBuildInputs = [ cmake ];
  outputs = [ "out" "dev" ];
}
