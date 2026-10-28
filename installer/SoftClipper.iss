; Instalador de Soft Clipper (VST3) para Windows - Inno Setup
[Setup]
AppName=Soft Clipper
AppVersion=1.0.0
AppPublisher=MiEstudio
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
CreateAppDir=no
Uninstallable=no
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
OutputDir=..\dist
OutputBaseFilename=SoftClipper-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "..\build\SoftClipper_artefacts\Release\VST3\Soft Clipper.vst3\*"; DestDir: "{commoncf64}\VST3\Soft Clipper.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[Messages]
FinishedLabel=Soft Clipper se instalo en la carpeta VST3. En FL Studio ve a Options > Manage plugins > Find installed plugins.
