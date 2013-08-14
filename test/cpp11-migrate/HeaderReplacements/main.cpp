// The following block tests the following:
//   - Only 1 file is generated per translation unit and header file
//   - Replacements are written in YAML that matches the expected YAML file
// RUN: rm -rf %t/Test
// RUN: mkdir -p %t/Test
// RUN: cp %S/main.cpp %S/common.cpp %S/common.h %t/Test
// RUN: cpp11-migrate -loop-convert -headers -include=%t/Test %t/Test/main.cpp %t/Test/common.cpp --
// Check that only 1 file is generated per translation unit and header file.
// RUN: ls -1 %t/Test | grep -c "main.cpp_common.h_.*.yaml" | grep "^1$"
// RUN: ls -1 %t/Test | grep -c "common.cpp_common.h_.*.yaml" | grep "^1$"
// RUN: cp %S/common.h.yaml %t/Test/main.cpp_common.h.yaml
// We need to put the build path to the expected YAML file to diff against the generated one.
// RUN: sed -e 's#$(path)#%t/Test#g' %S/common.h.yaml > %t/Test/main.cpp_common.h.yaml
// RUN: sed -i -e 's#\\#/#g' %t/Test/main.cpp_common.h_*.yaml
// RUN: diff -b %t/Test/main.cpp_common.h.yaml %t/Test/main.cpp_common.h_*.yaml
// RUN: sed -e 's#$(path)#%t/Test#g' -e 's#main.cpp"#common.cpp"#g' %S/common.h.yaml > %t/Test/common.cpp_common.h.yaml
// RUN: sed -i -e 's#\\#/#g' %t/Test/common.cpp_common.h_*.yaml
// RUN: diff -b %t/Test/common.cpp_common.h.yaml %t/Test/common.cpp_common.h_*.yaml
//
// The following block tests the following:
//   - YAML files are written only when -headers is used
// RUN: rm -rf %t/Test
// RUN: mkdir -p %t/Test
// RUN: cp %S/main.cpp %S/common.cpp %S/common.h %t/Test
// RUN: cpp11-migrate -loop-convert -headers -include=%t/Test %t/Test/main.cpp --
// RUN: cpp11-migrate -loop-convert %t/Test/common.cpp --
// Check that only one YAML file is generated from main.cpp and common.h and not from common.cpp and common.h since -header is not specified
// RUN: ls -1 %t/Test | grep -c "main.cpp_common.h_.*.yaml" | grep "^1$"
// RUN: ls -1 %t/Test | not grep "common.cpp_common.h_.*.yaml"
// We need to put the build path to the expected YAML file to diff against the generated one.
// RUN: sed -e 's#$(path)#%t/Test#g' %S/common.h.yaml > %t/Test/main.cpp_common.h.yaml
// RUN: sed -i -e 's#\\#/#g' %t/Test/main.cpp_common.h_*.yaml
// RUN: diff -b %t/Test/main.cpp_common.h.yaml %t/Test/main.cpp_common.h_*.yaml

#include "common.h"

void test_header_replacement() {
  dostuff();
  func2();
}

// FIXME: Investigating on lit-win32.
// REQUIRES: shell
