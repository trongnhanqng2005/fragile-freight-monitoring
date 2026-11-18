$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$firmwareRoot = Join-Path $repoRoot 'firmware'
$runnerPath = Join-Path $PSScriptRoot 'run-wokwi.ps1'
$runtimeRoot = Join-Path $firmwareRoot '.pio\wokwi-runtime'
$runId = 'suite-{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'), ([guid]::NewGuid().ToString('N').Substring(0, 8))
$startedUtc = (Get-Date).ToUniversalTime().ToString('o')
$suiteDirectory = Join-Path $runtimeRoot $runId
New-Item -ItemType Directory -Path $suiteDirectory -Force | Out-Null
$records = [Collections.Generic.List[object]]::new()
$smokePassed = $false

function Invoke-WokwiScenario([string] $Name, [string] $File, [string[]] $States, [int] $Bursts, [int] $Timeout, [switch] $MqttUnavailable) {
    $statesArg = $States -join ','
    $childArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $runnerPath,
        '-Scenario', $File, '-ScenarioName', $Name, '-TimeoutMs', [string]$Timeout,
        '-ExpectedStates', $statesArg, '-ExpectedBuzzerBursts', [string]$Bursts)
    if ($MqttUnavailable) { $childArgs += '-RequireMqttUnavailable' }
    $output = @(& powershell.exe @childArgs 2>&1 | ForEach-Object { $_.ToString() })
    $exit = $LASTEXITCODE
    $resultLine = @($output | Where-Object { $_ -like 'Results: *' } | Select-Object -Last 1)
    if ($resultLine.Count -gt 0) {
        $path = $resultLine[0].Substring('Results: '.Length)
        if (Test-Path -LiteralPath $path) {
            $record = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
            $record | Add-Member -NotePropertyName suite_scenario -NotePropertyValue $Name -Force
            $records.Add($record)
            return [pscustomobject]@{ status = $record.status; exit = $exit; output = $output; result = $record }
        }
    }
    $records.Add([pscustomobject]@{ scenario = $Name; suite_scenario = $Name; status = 'BLOCKED'; cli_exit_code = $exit; diagnostics = 'Runner did not produce results.json; inspect runner output.' })
    return [pscustomobject]@{ status = 'BLOCKED'; exit = $exit; output = $output; result = $records[$records.Count - 1] }
}

$smoke = Invoke-WokwiScenario 'smoke-safe' 'test/wokwi/smoke-safe.yaml' @('SAFE') 0 20000
$smokePassed = $smoke.status -eq 'PASS'
$scenarioPlan = @(
    @{ name = 'A-state-cycle'; file = 'test/wokwi/state-cycle.yaml'; states = @('SAFE', 'WARNING', 'DANGER'); bursts = 1; timeout = 20000 },
    @{ name = 'C-single-danger-buzzer'; file = 'test/wokwi/single-danger.yaml'; states = @('DANGER'); bursts = 1; timeout = 20000 },
    @{ name = 'D-persistent-danger-no-repeat'; file = 'test/wokwi/persistent-danger.yaml'; states = @('DANGER'); bursts = 1; timeout = 20000 },
    @{ name = 'E-warning-does-not-rearm'; file = 'test/wokwi/warning-no-rearm.yaml'; states = @('DANGER', 'WARNING'); bursts = 1; timeout = 25000 },
    @{ name = 'F-safe-before-cooldown'; file = 'test/wokwi/safe-before-cooldown.yaml'; states = @('DANGER', 'SAFE'); bursts = 1; timeout = 25000 },
    @{ name = 'G-safe-after-cooldown-rearms'; file = 'test/wokwi/safe-after-cooldown.yaml'; states = @('DANGER', 'SAFE'); bursts = 2; timeout = 25000 },
    @{ name = 'H-local-outputs-without-mqtt'; file = 'test/wokwi/local-outputs-offline.yaml'; states = @('SAFE', 'DANGER'); bursts = 1; timeout = 30000; mqttUnavailable = $true }
)

if ($smokePassed) {
    foreach ($case in $scenarioPlan) {
        $invokeParameters = @{ Name = $case.name; File = $case.file; States = $case.states; Bursts = $case.bursts; Timeout = $case.timeout }
        if ($case.mqttUnavailable) {
            $invokeParameters.MqttUnavailable = $true
            $result = Invoke-WokwiScenario @invokeParameters
        }
        else {
            $result = Invoke-WokwiScenario @invokeParameters
        }
        Write-Output ('{0}: {1}' -f $case.name, $result.status)
    }
    $stateCycle = @($records | Where-Object suite_scenario -eq 'A-state-cycle' | Select-Object -Last 1)
    if ($stateCycle.Count -gt 0) {
        $records.Add([pscustomobject]@{ scenario = 'B-led-gpio'; suite_scenario = 'B-led-gpio'; status = $stateCycle[0].status; cli_exit_code = $stateCycle[0].cli_exit_code; evidence_directory = $stateCycle[0].run_directory; diagnostics = 'GPIO25/26/27 SAFE, WARNING, and DANGER assertions are part of the A-state-cycle scenario.' })
        Write-Output ('B-led-gpio: {0} (same state-cycle evidence)' -f $stateCycle[0].status)
    }
}
else {
    foreach ($case in $scenarioPlan) {
        $records.Add([pscustomobject]@{ scenario = $case.name; suite_scenario = $case.name; status = 'NOT TESTED'; cli_exit_code = $null; diagnostics = 'Smoke scenario did not pass; full suite was not started.' })
    }
    $records.Add([pscustomobject]@{ scenario = 'B-led-gpio'; suite_scenario = 'B-led-gpio'; status = 'NOT TESTED'; cli_exit_code = $null; diagnostics = 'Smoke scenario did not pass; GPIO suite was not started.' })
}

$firmwareBin = Join-Path $firmwareRoot '.pio\build\esp32dev\firmware.bin'
$cli = Join-Path $repoRoot '.tools\wokwi\wokwi-cli.exe'
$gitHead = (& git -C $repoRoot rev-parse HEAD 2>$null).Trim()
$dirty = @(& git -C $repoRoot status --short 2>$null)
$suiteStatus = if (@($records | Where-Object status -eq 'FAIL').Count) { 'FAIL' } elseif (@($records | Where-Object status -eq 'BLOCKED').Count) { 'BLOCKED' } elseif (@($records | Where-Object status -eq 'NOT TESTED').Count) { 'NOT TESTED' } elseif (@($records | Where-Object status -ne 'PASS').Count) { 'FAIL' } else { 'PASS' }
$mqttRecord = [pscustomobject]@{ scenario = 'real-firmware-to-host-Mosquitto-delivery'; status = 'NOT TESTED'; diagnostics = 'The headless CLI suite does not measure broker or subscriber delivery. Verify real firmware-to-broker delivery separately with the VS Code Wokwi simulator and a host broker subscriber, as described in docs/development/local-setup.md.' }
$report = [ordered]@{
    suite = 'automated-wokwi-firmware-runtime'
    status = $suiteStatus
    started_utc = $startedUtc
    cli_version = 'v0.28.1'
    cli_sha256 = (Get-FileHash -LiteralPath $cli -Algorithm SHA256).Hash.ToLowerInvariant()
    git_head = $gitHead
    dirty_worktree_status = $dirty
    firmware_binary = '.pio/build/esp32dev/firmware.bin'
    firmware_sha256 = (Get-FileHash -LiteralPath $firmwareBin -Algorithm SHA256).Hash.ToLowerInvariant()
    scenarios = @($records.ToArray())
    mqtt_integration = $mqttRecord
    evidence_directory = $suiteDirectory
}
$reportPath = Join-Path $suiteDirectory 'results.json'
[IO.File]::WriteAllText($reportPath, ($report | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
Write-Output ('Suite: {0}' -f $suiteStatus)
Write-Output ('Results: {0}' -f $reportPath)
if ($suiteStatus -ne 'PASS') { exit 1 }
