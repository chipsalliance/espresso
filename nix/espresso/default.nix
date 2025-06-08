# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 Jiuyang Liu <liu@jiuyang.me>

{
  stdenv,
  fetchFromGitHub,
  cmake,
  ninja,
  asciidoctor,
  catch2
}:
stdenv.mkDerivation rec {
  name = "espresso";
  nativeBuildInputs = [
    cmake
    ninja
    asciidoctor
    catch2
  ];
  src = ../../.;
}
