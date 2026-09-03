[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [UInt64]$Seed = 24301,
    [int]$TimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

& (Join-Path $PSScriptRoot 'TestGameplay.ps1') `
    -EngineRoot $EngineRoot `
    -Seed $Seed `
    -TimeoutSeconds $TimeoutSeconds
