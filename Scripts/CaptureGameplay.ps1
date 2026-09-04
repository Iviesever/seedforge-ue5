[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 180,
    [string]$ExpectedRevision
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters

& (Join-Path $PSScriptRoot 'TestGameplay.ps1') `
    -EngineRoot $EngineRoot `
    -Seed $Seed `
    -TimeoutSeconds $TimeoutSeconds `
    -ExpectedRevision $verificationContext.ExpectedRevision
