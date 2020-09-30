#requires -version 6
<#
.SYNOPSIS
    Deploys the Stereognosis application.
.DESCRIPTION
    Deploys the contents of the local build dir to a target remote system using
    the WSMan system in Powershell.
.PARAMETER BuildDir
    Local Directory that will be compressed and copied to remote system.
.PARAMETER TargetHost
    Target IP or Hostname used to locate remote host.
.INPUTS
    Files in the Build dir
.OUTPUTS
    Modifies TargetDir on TargetHost.
.NOTES
    Version:        1.0
    Author:         Danielle MacDonald
    Creation Date:  2020-09

    This WILL deploy over anything in the target directory!
    If you are not logged in as "Somlab", you'll need to set the password for
    that account in this file. It is hardcoded, and .gitignore'd so the
    password won't be accidentally committed.

    You MUST setup WinRM, and configure the TargetHost as a TrustedHost in your
    local configuration for this script to work. Enable WinRM in pwsh as Admin:
    `Enable-PSRemoting -force`
    
    and to get your list of TrustedHosts:
    `Get-Item -Path WSMan:\localhost\Client\TrustedHosts`

    To add to trusted hosts (appends to list):
    `Set-Item -Path WSMan:\localhost\Client\TrustedHosts -Value '<IP or Hostname>' -Concatenate -Force`

.EXAMPLE
    deploy.ps1 -TargetHost '172.16.254.1'
#>

# Defaults TargetHost to the current Stereognosis Control Host IP
param (
    #[Parameter(Mandatory=$true)][String[]]$BuildDir,
    [String[]]$BuildDir = "E:\repos\stereognosis_protocol_main\ProtocolApp\App\",
    [String[]]$TargetHost='205.208.87.185'
)

# $BuildDir = "E:\repos\stereognosis_protocol_main\ProtocolApp\App\"
# $BuildDir = $(SolutionDir)ProtocolApp\App\

$TargetDir = 'C:\Somlab\bin\stereognosis\'

function Write-Info ($s) { Write-Host -ForegroundColor Green $s }

function Deploy-ToTargetHost {
    # Tempfile used to hold compressed copy    
    $TempFile = New-TemporaryFile
    
    # Compress, then push to remote
    # https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.archive/compress-archive?view=powershell-7
    $p = @{
        Path = "$BuildDir*"
        CompressionLevel = "Fastest"
        DestinationPath = "$TempFile"
    }
    Write-Info "Compressing to $tempFile"
    Compress-Archive @p -Force
    # TODO: Don't forget to clean up generated files
    
    # If you're not logged in as Somlab, prompt for cred
    if ($env:username -ne 'somlab') {
        $cred = Get-Credential -UserName 'somlab'
    }

    # This should just make it use the normal user if that's available.
    if ($env:username -ne 'somlab') {
        $sess = New-PSSession -ComputerName $TargetHost -Credential $cred
    } else {
        $sess = New-PSSession -ComputerName $TargetHost
    }
    if (!$sess) {
        Write-Error "Could not establish session."
        Exit 1
    }

    # Oneliner for testing dir existence:
    # Invoke-Command -Session $sess -ArgumentList $TargetDir {param($a);Test-Path $a}
    
    Write-Info "If not exist, create dir on remote sys"
    Invoke-Command -Session $sess -ArgumentList ($TargetDir) {
        param($arg1)
        if (-NOT (Test-Path $arg1)) {
            New-Item -ItemType Directory -Path $arg1 }
    }

    Write-Info "Copying to remote"
    Copy-Item -Path "$TempFile" -Destination "${TargetDir}deploy.zip" -ToSession $sess

    Write-Info "Unzipping package on the remote side"
    Invoke-Command -Session $sess -ArgumentList ($TargetDir){
        param($arg1)
        Expand-Archive -Path "${arg1}deploy.zip" -DestinationPath "$arg1" -Force
    }
    $sess | Remove-PSSession
    Remove-Item $TempFile
    Write-Info
}

Deploy-ToTargetHost
