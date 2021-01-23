add_library(utility utility/cpu_time.c utility/prtime.c utility/strsav.c)
target_include_directories(
  utility
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/port
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/st)

add_executable(
  espresso
  espresso/cofactor.c
  espresso/cols.c
  espresso/compl.c
  espresso/contain.c
  espresso/cubestr.c
  espresso/cvrin.c
  espresso/cvrm.c
  espresso/cvrmisc.c
  espresso/cvrout.c
  espresso/dominate.c
  espresso/equiv.c
  espresso/espresso.c
  espresso/essen.c
  espresso/exact.c
  espresso/expand.c
  espresso/gasp.c
  espresso/getopt.c
  espresso/gimpel.c
  espresso/globals.c
  espresso/hack.c
  espresso/indep.c
  espresso/irred.c
  espresso/main.c
  espresso/map.c
  espresso/matrix.c
  espresso/mincov.c
  espresso/opo.c
  espresso/pair.c
  espresso/part.c
  espresso/primes.c
  espresso/reduce.c
  espresso/rows.c
  espresso/set.c
  espresso/setc.c
  espresso/sharp.c
  espresso/sminterf.c
  espresso/solution.c
  espresso/sparse.c
  espresso/unate.c
  espresso/verify.c
  espresso/signature.c
  espresso/signature_exact.c
  espresso/util_signature.c
  espresso/black_white.c
  espresso/canonical.c
  espresso/essentiality.c
  espresso/sigma.c)
target_link_libraries(espresso utility)
target_include_directories(
  espresso
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/port
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/utility)
