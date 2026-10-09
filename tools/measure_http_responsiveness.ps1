param(
    [Parameter(Mandatory = $true)]
    [string]$BaseUrl,
    [ValidateRange(1, 1000)]
    [int]$Requests = 30,
    [ValidateRange(0, 60000)]
    [int]$IntervalMs = 200,
    [ValidateRange(1, 30)]
    [int]$TimeoutSec = 3,
    [string]$OutputDirectory = 'build/experiments/http-responsiveness'
)

$ErrorActionPreference = 'Stop'

$base = $BaseUrl.TrimEnd('/')
$uri = $null
if (-not [Uri]::TryCreate($base, [UriKind]::Absolute, [ref]$uri) -or
    $uri.Scheme -notin @('http', 'https')) {
    throw 'BaseUrl must be an absolute HTTP or HTTPS URL.'
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$rows = [System.Collections.Generic.List[object]]::new()

for ($index = 1; $index -le $Requests; $index++) {
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $success = $false
    $errorKind = ''
    $mqttConnected = $null
    $mqttFailures = $null
    $rangeStatus = $null

    try {
        $status = Invoke-RestMethod -Uri ($base + '/api/status') -TimeoutSec $TimeoutSec
        $success = $true
        $mqttConnected = $status.mqtt_connected
        $mqttFailures = $status.mqtt_failures
        $rangeStatus = $status.range_status
    }
    catch {
        $errorKind = $_.Exception.GetType().Name
    }
    finally {
        $stopwatch.Stop()
    }

    $rows.Add([pscustomobject]@{
        request = $index
        captured_at = [DateTimeOffset]::Now.ToString('o')
        success = $success
        latency_ms = [Math]::Round($stopwatch.Elapsed.TotalMilliseconds, 1)
        error_kind = $errorKind
        mqtt_connected = $mqttConnected
        mqtt_failures = $mqttFailures
        range_status = $rangeStatus
    })

    if ($index -lt $Requests -and $IntervalMs -gt 0) {
        Start-Sleep -Milliseconds $IntervalMs
    }
}

$successRows = @($rows | Where-Object success)
$latencies = @($successRows | ForEach-Object latency_ms | Sort-Object)

function Get-Percentile([double[]]$Values, [double]$Percentile) {
    if ($Values.Count -eq 0) { return $null }
    $position = [Math]::Ceiling(($Percentile / 100.0) * $Values.Count) - 1
    $position = [Math]::Max(0, [Math]::Min($Values.Count - 1, $position))
    return $Values[$position]
}

$summary = [ordered]@{
    base_url = $base
    requested = $Requests
    succeeded = $successRows.Count
    failed = $Requests - $successRows.Count
    timeout_seconds = $TimeoutSec
    interval_ms = $IntervalMs
    min_latency_ms = if ($latencies.Count) { $latencies[0] } else { $null }
    average_latency_ms = if ($latencies.Count) {
        [Math]::Round(($latencies | Measure-Object -Average).Average, 1)
    } else { $null }
    p50_latency_ms = Get-Percentile $latencies 50
    p95_latency_ms = Get-Percentile $latencies 95
    max_latency_ms = if ($latencies.Count) { $latencies[-1] } else { $null }
}

$rows | Export-Csv -LiteralPath (Join-Path $OutputDirectory 'requests.csv') -NoTypeInformation
$summary | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'summary.json') -Encoding utf8
$summary
