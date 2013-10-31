// RUN: pp-trace %s -undef -target x86_64 -std=c++11 | FileCheck --strict-whitespace %s

#include "Input/Level1A.h"
#include "Input/Level1B.h"

// CHECK: ---
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}pp-trace-include.cpp:1:1"
// CHECK-NEXT:   Reason: EnterFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "<built-in>:1:1"
// CHECK-NEXT:   Reason: EnterFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "<built-in>:1:1"
// CHECK-NEXT:   Reason: RenameFile
// CHECK-NEXT:   FileType: C_System
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: __STDC__
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: __STDC_HOSTED__
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: __cplusplus
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: __STDC_UTF_16__
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: __STDC_UTF_32__
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "<command line>:1:1"
// CHECK-NEXT:   Reason: EnterFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "<built-in>:1:1"
// CHECK-NEXT:   Reason: ExitFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}pp-trace-include.cpp:1:1"
// CHECK-NEXT:   Reason: ExitFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (getFileEntryForID failed)
// CHECK-NEXT: - Callback: InclusionDirective
// CHECK-NEXT:   IncludeTok: include
// CHECK-NEXT:   FileName: "Input/Level1A.h"
// CHECK-NEXT:   IsAngled: false
// CHECK-NEXT:   FilenameRange: "Input/Level1A.h"
// CHECK-NEXT:   File: "{{.*}}{{[/\\]}}Input/Level1A.h"
// CHECK-NEXT:   SearchPath: "{{.*}}{{[/\\]}}pp-trace"
// CHECK-NEXT:   RelativePath: "Input/Level1A.h"
// CHECK-NEXT:   Imported: (null)
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}Input/Level1A.h:1:1"
// CHECK-NEXT:   Reason: EnterFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: InclusionDirective
// CHECK-NEXT:   IncludeTok: include
// CHECK-NEXT:   FileName: "Level2A.h"
// CHECK-NEXT:   IsAngled: false
// CHECK-NEXT:   FilenameRange: "Level2A.h"
// CHECK-NEXT:   File: "{{.*}}{{[/\\]}}Input/Level2A.h"
// CHECK-NEXT:   SearchPath: "{{.*}}{{[/\\]}}Input"
// CHECK-NEXT:   RelativePath: "Level2A.h"
// CHECK-NEXT:   Imported: (null)
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}Input/Level2A.h:1:1"
// CHECK-NEXT:   Reason: EnterFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: MACRO_2A
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}Input/Level1A.h:2:1"
// CHECK-NEXT:   Reason: ExitFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: "{{.*}}{{[/\\]}}Input/Level2A.h"
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: MACRO_1A
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}pp-trace-include.cpp:4:1"
// CHECK-NEXT:   Reason: ExitFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: "{{.*}}{{[/\\]}}Input/Level1A.h"
// CHECK-NEXT: - Callback: InclusionDirective
// CHECK-NEXT:   IncludeTok: include
// CHECK-NEXT:   FileName: "Input/Level1B.h"
// CHECK-NEXT:   IsAngled: false
// CHECK-NEXT:   FilenameRange: "Input/Level1B.h"
// CHECK-NEXT:   File: "{{.*}}{{[/\\]}}Input/Level1B.h"
// CHECK-NEXT:   SearchPath: "{{.*}}{{[/\\]}}pp-trace"
// CHECK-NEXT:   RelativePath: "Input/Level1B.h"
// CHECK-NEXT:   Imported: (null)
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}Input/Level1B.h:1:1"
// CHECK-NEXT:   Reason: EnterFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: (invalid)
// CHECK-NEXT: - Callback: MacroDefined
// CHECK-NEXT:   MacroNameTok: MACRO_1B
// CHECK-NEXT:   MacroDirective: MD_Define
// CHECK-NEXT: - Callback: FileChanged
// CHECK-NEXT:   Loc: "{{.*}}{{[/\\]}}pp-trace-include.cpp:5:1"
// CHECK-NEXT:   Reason: ExitFile
// CHECK-NEXT:   FileType: C_User
// CHECK-NEXT:   PrevFID: "{{.*}}{{[/\\]}}Input/Level1B.h"
// CHECK-NEXT: - Callback: EndOfMainFile
// CHECK-NEXT: ...
