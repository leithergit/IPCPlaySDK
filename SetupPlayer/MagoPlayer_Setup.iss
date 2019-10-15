; 脚本由 Inno Setup 脚本向导 生成！
; 有关创建 Inno Setup 脚本文件的详细资料请查阅帮助文档！

#define MyAppName "迈高视频播放器"
#define MyAppName_EN  "Mago Video Player"
#define MyAppVersion "1.0.0.3"
#define MyAppPublisher_CN "上海迈高网络技术有限公司"
#define MyAppPublisher_EN "Shanghai Mago Network Technology Co.,Ltd."
#define MyAppURL "http://www.shmgwl.com/"
#define MyAppExeName "MagoPlayer.exe"

[Setup]
; 注: AppId的值为单独标识该应用程序。
; 不要为其他安装程序使用相同的AppId值。
; (生成新的GUID，点击 工具|在IDE中生成GUID。)
AppId={{D956FCD4-01F1-4E39-98A7-81423261B203}
AppName={cm:MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
;AppPublisher={#MyAppPublisher}
AppPublisher={cm:AppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\MagoPlayer
DisableProgramGroupPage=yes
OutputDir=.\
OutputBaseFilename=MagoPlayer_{#MyAppVersion}
SetupIconFile=.\MagoPlayer.ico
Compression=lzma
SolidCompression=yes
UsePreviousAppDir=No

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Default.isl"
Name: "english"; MessagesFile: "compiler:Languages\english.isl"

[Messages]
english.BeveledLabel=englishlish
chinesesimp.BeveledLabel=Chineses

[CustomMessages]
chinesesimp.MyAppName={#MyAppName} 
chinesesimp.Type="zh_CN"
chinesesimp.Title="迈高视频播放器" 
chinesesimp.AppPublisher={#MyAppPublisher_CN}
english.MyAppName={#MyAppName_EN}
english.Type="en_US"
english.Title="MagoPlayer"
english.AppPublisher={#MyAppPublisher_EN}


[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkablealone; OnlyBelowVersion: 0,8.1

[Files]
Source: ".\vc2008redist_x86.exe"; DestDir: "{app}"; Flags: ignoreversionSource: ".\vc2013redist_x86.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: ".\Files\*"; DestDir: "{app}\"; Flags: ignoreversion recursesubdirs createallsubdirs

; 注意: 不要在任何共享系统文件上使用“Flags: ignoreversion”


[Icons]
Name: "{commonprograms}\{cm:MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{cm:MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
;根据不同的语言环境显示对应的快捷方式
Filename: "{app}\vc2008redist_x86.exe";Parameters:"/q";Flags: postinstall waituntilterminated unchecked;
Filename: "{app}\vc2013redist_x86.exe";Parameters:"/q";Flags: postinstall waituntilterminated unchecked;

