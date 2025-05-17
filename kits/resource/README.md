# Resource Kit

This package contains PowerShell, Python, and VBA templates that Cobalt Strike
uses throughout its workflows. You may modify these templates to evade
signature-based detections or to customize how Cobalt Strike uses these
templates in its workflows.

# Load into Cobalt Strike

To use the Resource Kit, load the resources.cna into Cobalt Strike.

Open the Scripts manager, Cobalt Strike -> Scripts

Load `<output directory>/resource/resources.cna`

# Resources
```
compress.ps1:
	This is the PowerShell template Cobalt Strike uses to run GZIP compressed
	PowerShell scripts. 

	Affected features:
		Payload Generator [PowerShell Command Output]
		Scripted Web Delivery (PowerShell)
		Beacon: powershell-import, psexec_psh, wmi
```

```
template.exe.hta  
	This is the HTA template Cobalt Strike uses to drop and run an executable
	artifact.

	Affected features:
		HTML Application (Executable)
```

```
template.psh.hta  
    This is the HTA template Cobalt Strike uses to run a PowerShell payload.

	Affected features:
		HTML Application (PowerShell)
```

```
template.py
	This is the Python template Cobalt Strike uses to run x86 and x64 payloads.

	Affected features:
		Scripted Web Delivery (Python)
```

```
template.vbs
	This is the VBScript template Cobalt Strike uses to run a VBA macro payload.

	Affected features:
		HTML Application (VBA)
		Scripted Web Delivery (regsvr32)
```

```
template.x64.ps1
	This is the PowerShell template Cobalt Strike uses to run an x64 payload.

	Affected features:
		Windows EXE (Stageless) [PowerShell Output]
```

```
template.x86.ps1
	This is the PowerShell template Cobalt Strike uses to run an x86 payload.

	Affected features:
		HTML Application (PowerShell)
		Scripted Web Delivery (PowerShell)
		Windows EXE (Stageless) [PowerShell Output]
		Beacon: spawnas, spawnu, psexec_psh, winrm, wmi
```

```
template.x86.vba
	This is the VBA template Cobalt Strike uses to run x86 payloads.

	Affected features:
		HTML Application (VBA)
		Microsoft Office Macro Attack
		Scripted Web Delivery (regsvr32)
```

# Other
```
set POWERSHELL_COMMAND
	Controls the form of the PowerShell commands throughout Cobalt Strike

	Affected features:
		HTML Application (PowerShell)
		Payload Generator Dialog
		Beacon: powershell, winrm, wmi, elevate uac-token-elevate, spawnas,
		        spawnu, psexec_psh
		Functions: &powershell		
```

```
set POWERSHELL_DOWNLOAD_CRADLE
	Controls the form of the PowerShell download cradle in Cobalt Strike's
	automation

	Affected features:
		Beacon: elevate, uac-token-bypass, powershell-import, spawnas, spawnu
		Functions: &beacon_host_script, &beacon_host_imported_script
``
