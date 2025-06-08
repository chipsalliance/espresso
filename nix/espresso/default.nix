# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 Jiuyang Liu <liu@jiuyang.me>

{
  stdenv,
  fetchFromGitHub,
  cmake,
  ninja,
  asciidoctor,
}:
stdenv.mkDerivation rec {
  name = "espresso";
  nativeBuildInputs = [
    cmake
    ninja
    asciidoctor
  ];
  src = ../../.;
}
