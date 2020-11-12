#requires -version 5

function Write-Info ($s) { Write-Host -ForegroundColor Green $s }

function Get-VCPkgRoot {
    # Ask where.exe if vcpkg is on the path already. Otherwise, search known locations.
    $VCPkgRoot = ""
    $VCPkgRoot = Split-Path (where.exe vcpkg.exe)
    if ($VCPkgRoot -ne  "") {Return ($VCPkgRoot + '\')}
    $vcpkgPaths = @("C:\Repositories\vcpkg\", "C:\Repositories\vcpkg-master\", "D:\opt\vcpkg\")
    $vcpkgPaths | ForEach-Object {
        if (Test-Path $_) {
            Return $_
            Break
        }
    }
    if ($VCPkgRoot -eq  "") {
        Write-Error "Could not locate VCPkg."
        Exit 1
    }
}

function Test-VcpkgInstalled {
    param(
        [Parameter(Mandatory=$true)][String[]]$pkgName,
        [String[]]$VcPkg=(Get-VCPkgRoot) + "vcpkg.exe"
    )
    $results = Invoke-Expression "$VcPkg list $pkgName"
    if ($results.length -eq 0) {
        return $False
    }
    return $True
}

function Generate-gRPC {
    <#
    .SYNOPSIS
        Build gRPC generated source files from the .proto definition.
    .DESCRIPTION
        Finds vcpkg version of gRPC, and uses it to generate c/c++ files.
    .PARAMETER ProtoFile
        Path to the proto file used to build from.
    .PARAMETER outputPath
        Path for output. Assumes ProtocolApp dir relative to this script.
    .INPUTS
        ProtoFile parameter
    .OUTPUTS
        C/C++ source files built by the gRPC/protobuf generator, protoc.exe.
    .NOTES
        Version:        1.0
        Author:         Danielle MacDonald
        Creation Date:  2020-10

        Assumes you're using the Vcpkg version of gRPC. Built for BensmaiaLab.

    .EXAMPLE
        buildGRPC.ps1 -ProtoFile .\grpcs\tekscan_server.proto
    #>
    param (
        [Parameter(Mandatory=$true)][String[]]$ProtoFile,
        [String[]]$outputPath = "..\ProtocolApp\"
    )
    $VCPkgRoot = Get-VCPkgRoot
    $VCPkgTools = $VCPkgRoot + "installed\x64-windows\tools\"

    # Get the rest of our binaries
    $protoc =  $VCPkgTools + "protobuf\protoc.exe"
    $grpc_cpp_plugin = $VCPkgTools + "grpc\grpc_cpp_plugin.exe"
    $grpc_csharp_plugin = $VCPkgTools + "grpc\grpc_csharp_plugin.exe"

    # Builds message definitions
    # Builds the service
    Invoke-Expression "$protoc --plugin=protoc-gen-grpc=`"$grpc_cpp_plugin`" --grpc_out=`"$outputPath`" $ProtoFile"
    Invoke-Expression "$protoc --cpp_out=`"$outputPath`" $ProtoFile"

}

$protoList = Get-Item *.proto

$protoList | ForEach-Object {
    $filename = Split-Path -Leaf $_
    Write-Info "Generating gRPC for $filename"
    Generate-gRPC -ProtoFile "$filename"
}
