# Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

# Set project main file.
set(MAIN_SRC
  "${SRC_PATH}/d2d.cpp"
)

# Set project source files.
set(SRC
 "${SRC_PATH}/atomScanner.cpp"
 "${SRC_PATH}/catalogPruner.cpp"
 "${SRC_PATH}/lambertFetch.cpp"
 "${SRC_PATH}/lambertZoom.cpp"
 "${SRC_PATH}/lambertSequences.cpp"
 "${SRC_PATH}/lambertScanner.cpp"
 "${SRC_PATH}/pagmoScanner.cpp"
 "${SRC_PATH}/lambertTransfer.cpp"
 "${SRC_PATH}/sgp4Scanner.cpp"
 "${SRC_PATH}/j2Analysis.cpp"
 "${SRC_PATH}/tools.cpp"
)

# Set project test source files.
set(TEST_SRC
  "${TEST_SRC_PATH}/testD2D.cpp"
  "${TEST_SRC_PATH}/testTools.cpp"
  "${TEST_SRC_PATH}/testCatalogPruner.cpp"
  "${TEST_SRC_PATH}/testTypedefs.cpp"
)
