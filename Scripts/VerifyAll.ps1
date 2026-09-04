[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\program\UnrealEngine\Epic Games\UE_5.8',
    [string]$ExpectedRevision
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'VerificationContract.ps1')
$verificationContext = New-SeedForgeScriptContext -ProjectRoot $projectRoot -Parameters $PSBoundParameters
# Preserve one authoritative pipeline; Phase 3 includes the original Phase 2 gates.
$result = Invoke-SeedForgeScriptStep -Context $verificationContext -Name 'Complete Phase 3 verification' -Action {
    & (Join-Path $PSScriptRoot 'VerifyPhase3.ps1') -EngineRoot $EngineRoot -ExpectedRevision $verificationContext.ExpectedRevision
}
Assert-SeedForgeScriptContext -Context $verificationContext
return $result
