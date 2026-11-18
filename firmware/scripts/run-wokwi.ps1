param(
    [Parameter(Mandatory = $true)]
    [string] $Scenario,
    [Parameter(Mandatory = $true)]
    [string] $ScenarioName,
    [int] $TimeoutMs = 15000,
    [string[]] $ExpectedStates = @('SAFE'),
    [int] $ExpectedBuzzerBursts = -1,
    [switch] $RequireMqttUnavailable
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$firmwareRoot = Join-Path $repoRoot 'firmware'
$cliPath = Join-Path $repoRoot '.tools\wokwi\wokwi-cli.exe'
$tokenPath = Join-Path $repoRoot '.tools\wokwi\token.dpapi'
$binaryPath = Join-Path $firmwareRoot '.pio\build\esp32dev\firmware.bin'
$scenarioPath = Join-Path $firmwareRoot $Scenario
$artifactRoot = Join-Path $firmwareRoot '.pio\wokwi-runtime'

if (-not (Test-Path -LiteralPath $tokenPath -PathType Leaf)) {
    throw 'BLOCKED: project-local encrypted Wokwi token file is missing.'
}
if (-not (Test-Path -LiteralPath $cliPath -PathType Leaf)) {
    throw 'BLOCKED: project-local Wokwi CLI executable is missing.'
}
if (-not (Test-Path -LiteralPath $binaryPath -PathType Leaf)) {
    throw 'BLOCKED: ESP32 firmware binary is missing; build firmware first.'
}
if (-not (Test-Path -LiteralPath $scenarioPath -PathType Leaf)) {
    throw "BLOCKED: scenario file not found: $Scenario"
}

$runId = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss-fff'), ([guid]::NewGuid().ToString('N').Substring(0, 8))
$runDirectory = Join-Path $artifactRoot $runId
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null
$serialPath = Join-Path $runDirectory 'serial.log'
$stdoutPath = Join-Path $runDirectory 'cli.stdout.log'
$stderrPath = Join-Path $runDirectory 'cli.stderr.log'
$vcdPath = Join-Path $runDirectory 'signals.vcd'
$diagramPath = Join-Path $runDirectory 'diagram.json'
$resultPath = Join-Path $runDirectory 'results.json'
$process = $null
$processStarted = $false
$exitCode = $null
$status = 'BLOCKED'
$diagnostic = $null
$tokenText = $null
$secureToken = $null
$bstr = [IntPtr]::Zero

try {
    $ExpectedStates = @($ExpectedStates | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    try {
        $encryptedToken = (Get-Content -LiteralPath $tokenPath -Raw).Trim()
        $secureToken = ConvertTo-SecureString -String $encryptedToken -ErrorAction Stop
        $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secureToken)
        $tokenText = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
        if ($tokenText -notmatch '^wok_.{40}$') {
            throw 'Invalid token format.'
        }
    }
    catch {
        $diagnostic = 'BLOCKED: encrypted Wokwi token could not be decrypted or validated for this Windows user.'
        throw $diagnostic
    }
    finally {
        if ($bstr -ne [IntPtr]::Zero) {
            [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
            $bstr = [IntPtr]::Zero
        }
        if ($null -ne $secureToken) { $secureToken.Dispose() }
    }

    $diagram = Get-Content -LiteralPath (Join-Path $firmwareRoot 'diagram.json') -Raw | ConvertFrom-Json
    $diagram.parts += [pscustomobject]@{
        type = 'wokwi-logic-analyzer'
        id = 'runtime-analyzer'
        top = 400
        left = 450
        attrs = @{ bufferSize = '200000' }
    }
    $diagram.connections += ,@('esp:32', 'runtime-analyzer:D0', '', @())
    $diagram.connections += ,@('esp:25', 'runtime-analyzer:D1', '', @())
    $diagram.connections += ,@('esp:26', 'runtime-analyzer:D2', '', @())
    $diagram.connections += ,@('esp:27', 'runtime-analyzer:D3', '', @())
    [IO.File]::WriteAllText($diagramPath, ($diagram | ConvertTo-Json -Depth 100), [Text.UTF8Encoding]::new($false))

    [Environment]::SetEnvironmentVariable('WOKWI_CLI_TOKEN', $tokenText, 'Process')
    $tokenText = $null

    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $cliPath
    $startInfo.WorkingDirectory = $firmwareRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $arguments = @(
        '.',
        '--scenario', $Scenario,
        '--diagram-file', ('.pio/wokwi-runtime/{0}/diagram.json' -f $runId),
        '--timeout', [string]$TimeoutMs,
        '--serial-log-file', ('.pio/wokwi-runtime/{0}/serial.log' -f $runId),
        '--vcd-file', ('.pio/wokwi-runtime/{0}/signals.vcd' -f $runId)
    )
    $startInfo.Arguments = ($arguments | ForEach-Object { '"{0}"' -f ($_ -replace '"', '\"') }) -join ' '

    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $process.Start() | Out-Null
    $processStarted = $true
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    [IO.File]::WriteAllText($stdoutPath, $stdoutTask.Result, [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText($stderrPath, $stderrTask.Result, [Text.UTF8Encoding]::new($false))
    $exitCode = $process.ExitCode

    $serial = if (Test-Path -LiteralPath $serialPath) { Get-Content -LiteralPath $serialPath -Raw } else { '' }
    $combinedLog = (Get-Content -LiteralPath $stdoutPath -Raw) + (Get-Content -LiteralPath $stderrPath -Raw)
    $requiredEvidence = @('MPU6050 initialized') + @($ExpectedStates | ForEach-Object { "State=$($_)" })
    $missingEvidence = @($requiredEvidence | Where-Object { $serial -notmatch [regex]::Escape($_) })
    $buzzerBursts = @()
    if (Test-Path -LiteralPath $vcdPath) {
        $vcd = Get-Content -LiteralPath $vcdPath
        $buzzerCode = $null
        $unitScale = 1.0
        foreach ($line in $vcd) {
            if (-not $buzzerCode -and $line -match '^\$var\s+wire\s+1\s+(\S+)\s+(?:BUZZER|D0)\s+\$end') { $buzzerCode = $Matches[1] }
            if ($line -match '^\$timescale\s+(\d+)\s*(s|ms|us|ns|ps|fs)\s+\$end') {
                $unitScale = [double]$Matches[1] * @{ s = 1e9; ms = 1e6; us = 1e3; ns = 1; ps = 1e-3; fs = 1e-6 }[$Matches[2]]
            }
        }
        if ($buzzerCode) {
            $edges = [Collections.Generic.List[object]]::new()
            $time = 0.0
            foreach ($line in $vcd) {
                if ($line -match '^#(\d+)') { $time = [double]$Matches[1] * $unitScale; continue }
                if ($line -match ( '^([01])' + [regex]::Escape($buzzerCode) + '$' )) {
                    $edges.Add([pscustomobject]@{ value = [int]$Matches[1]; timeNs = $time })
                }
            }
            $groups = [Collections.Generic.List[object]]::new()
            $current = [Collections.Generic.List[object]]::new()
            foreach ($edge in $edges) {
                if ($current.Count -gt 0 -and ($edge.timeNs - $current[$current.Count - 1].timeNs) -gt 1000000) {
                    $groups.Add(@($current.ToArray()))
                    $current.Clear()
                }
                $current.Add($edge)
            }
            if ($current.Count -gt 0) { $groups.Add(@($current.ToArray())) }
            foreach ($group in $groups) {
                $rising = @($group | Where-Object value -eq 1)
                $falling = @($group | Where-Object value -eq 0)
                if ($rising.Count -lt 20 -or $falling.Count -eq 0) { continue }
                $durationMs = ($group[$group.Count - 1].timeNs - $group[0].timeNs) / 1e6
                $frequencyHz = if ($durationMs -gt 0) { $group.Count / (2.0 * ($durationMs / 1000.0)) } else { 0 }
                $buzzerBursts += [pscustomobject]@{ duration_ms = [math]::Round($durationMs, 3); frequency_hz = [math]::Round($frequencyHz, 1); transitions = $group.Count }
            }
        }
    }
    $buzzerFailure = $false
    if ($ExpectedBuzzerBursts -ge 0) {
        if ($buzzerBursts.Count -ne $ExpectedBuzzerBursts) { $buzzerFailure = $true }
        foreach ($burst in $buzzerBursts) {
            if ($burst.frequency_hz -lt 1800 -or $burst.frequency_hz -gt 2200 -or $burst.duration_ms -lt 100 -or $burst.duration_ms -gt 200) { $buzzerFailure = $true }
        }
    }
    $mqttConnected = $serial -match 'MQTT: connected \(CONNACK received\)'
    $mqttUnavailableSatisfied = -not $RequireMqttUnavailable -or (-not $mqttConnected -and $serial -match 'MQTT: starting connection attempt')
    if ($exitCode -eq 0 -and $missingEvidence.Count -eq 0 -and $combinedLog -match 'Scenario completed successfully') {
        $status = 'PASS'
    }
    elseif ($combinedLog -match '401|unauthorized|invalid token|quota|simulation time limit') {
        $status = 'BLOCKED'
        $diagnostic = 'Wokwi service rejected authentication or simulation allowance; see CLI logs.'
    }
    else {
        $status = 'FAIL'
        $diagnostic = if ($missingEvidence.Count) { 'Missing serial evidence: ' + ($missingEvidence -join ', ') } elseif ($buzzerFailure) { 'Buzzer waveform burst count or measured frequency/duration was outside the expected range.' } elseif (-not $mqttUnavailableSatisfied) { 'MQTT unavailability was not established by serial evidence.' } else { 'Scenario did not complete successfully; inspect CLI logs.' }
    }
    if ($status -eq 'PASS' -and ($buzzerFailure -or -not $mqttUnavailableSatisfied)) {
        $status = 'FAIL'
        $diagnostic = if ($buzzerFailure) { 'Buzzer waveform burst count or measured frequency/duration was outside the expected range.' } else { 'MQTT unavailability was not established by serial evidence.' }
    }
}
catch {
    if (-not $diagnostic) {
        $diagnostic = $_.Exception.Message
    }
}
finally {
    if ($processStarted -and $null -ne $process -and -not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
    [Environment]::SetEnvironmentVariable('WOKWI_CLI_TOKEN', $null, 'Process')
    $tokenText = $null
    if ($null -ne $process) { $process.Dispose() }
}

$firmwareHash = (Get-FileHash -LiteralPath $binaryPath -Algorithm SHA256).Hash.ToLowerInvariant()
$result = [ordered]@{
    scenario = $ScenarioName
    scenario_file = $Scenario
    status = $status
    cli_exit_code = $exitCode
    cli_version = 'v0.28.1'
    token_authentication = if ((Test-Path -LiteralPath $stdoutPath) -and (Get-Content -LiteralPath $stdoutPath -Raw) -match 'Connected to Wokwi Simulation API') { 'ACCEPTED by simulation API' } elseif ($status -eq 'BLOCKED') { 'BLOCKED or not established' } else { 'not independently exposed by CLI' }
    firmware_binary = '.pio/build/esp32dev/firmware.bin'
    firmware_sha256 = $firmwareHash
    simulation_timeout_ms = $TimeoutMs
    serial_log = 'serial.log'
    stdout_log = 'cli.stdout.log'
    stderr_log = 'cli.stderr.log'
    vcd = if (Test-Path -LiteralPath $vcdPath) { 'signals.vcd' } else { $null }
    measured_timing = @{ buzzer_bursts = @($buzzerBursts) }
    mqtt_delivery = if ($mqttConnected) { 'CONNACK observed; host broker message delivery not established' } elseif ($serial -match 'MQTT: starting connection attempt') { 'connection attempted; no CONNACK observed' } else { 'not observed' }
    diagnostics = $diagnostic
    run_directory = $runDirectory
}
[IO.File]::WriteAllText($resultPath, ($result | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
Write-Output ('{0}: {1}' -f $ScenarioName, $status)
Write-Output ('Evidence: {0}' -f $runDirectory)
Write-Output ('Results: {0}' -f $resultPath)
if ($status -ne 'PASS') { exit 1 }
