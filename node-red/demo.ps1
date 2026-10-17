# Mo phong ESP32: gui telemetry/alert theo MQTT contract chinh thuc
# (docs/mqtt/integration-contract.md trong repo nhom)
# Chay: powershell -ExecutionPolicy Bypass -File node-red/demo.ps1
param(
    [string]$Broker = "localhost",
    [int]$DelaySec = 4
)

$DeviceId = "esp32-01"
$TelemetryTopic = "fragile-freight/$DeviceId/telemetry"
$AlertTopic = "fragile-freight/$DeviceId/alert"

function Send-Mqtt {
    param([string]$Topic, $Payload)
    $json = $Payload | ConvertTo-Json -Compress
    $arg = $json -replace '"', '\"'
    # Contract: QoS 0, khong retain
    mosquitto_pub -h $Broker -t $Topic -q 0 -m $arg
}

# Gia tri mau de demo; nguong that do firmware quyet dinh
$steps = @(
    @{ Label = "SAFE";    Ax = 0.02;  Ay = -0.01; Az = 1.00; G = 1.00; Alert = $false },
    @{ Label = "SAFE";    Ax = 0.05;  Ay = 0.02;  Az = 0.98; G = 0.98; Alert = $false },
    @{ Label = "WARNING"; Ax = 0.30;  Ay = 0.20;  Az = 1.40; G = 1.45; Alert = $false },
    @{ Label = "DANGER";  Ax = 0.80;  Ay = 0.90;  Az = 1.40; G = 1.97; Alert = $true  },
    @{ Label = "SAFE";    Ax = 0.02;  Ay = -0.01; Az = 1.00; G = 1.00; Alert = $false }
)

foreach ($s in $steps) {
    $telemetry = [ordered]@{
        deviceId = $DeviceId
        ax       = $s.Ax
        ay       = $s.Ay
        az       = $s.Az
        gForce   = $s.G
        status   = $s.Label
    }
    Write-Host ("Gui telemetry: " + $s.Label + " (gForce=" + $s.G + ")")
    Send-Mqtt -Topic $TelemetryTopic -Payload $telemetry

    if ($s.Alert) {
        # Contract: alert chi gom deviceId, status (luon DANGER), gForce
        $alert = [ordered]@{
            deviceId = $DeviceId
            status   = "DANGER"
            gForce   = $s.G
        }
        Write-Host "Gui alert: DANGER"
        Send-Mqtt -Topic $AlertTopic -Payload $alert
    }
    Start-Sleep -Seconds $DelaySec
}
Write-Host "Xong kich ban demo."