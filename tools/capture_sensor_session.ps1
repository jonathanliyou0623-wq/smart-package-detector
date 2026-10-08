param(
    [Parameter(Mandatory = $true)]
    [string]$BaseUrl,
    [ValidateRange(1, 86400)]
    [int]$DurationSeconds = 60,
    [ValidateRange(1, 60)]
    [int]$IntervalSeconds = 10,
    [string]$OutputDirectory = ''
)

# Preserve overlapping raw snapshots: timestamps may restart when the ESP32 reboots.
# A failed download is logged, never silently treated as an empty measurement set.
$ErrorActionPreference = 'Stop'
$base = $BaseUrl.TrimEnd('/')
$uri = [uri]$base
if (-not $uri.IsAbsoluteUri -or $uri.Scheme -notin @('http', 'https')) {
    throw 'BaseUrl must be an absolute HTTP or HTTPS device URL.'
}
if (-not $OutputDirectory) {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $sessionName = 'session-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff')
    $OutputDirectory = Join-Path $projectRoot ('build\experiments\' + $sessionName)
}
$outputPath = [System.IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $outputPath) {
    throw 'Choose a new output directory so an earlier session cannot be overwritten.'
}
New-Item -ItemType Directory -Path $outputPath | Out-Null
$journal = Join-Path $outputPath 'captures.jsonl'
$expectedHeader = 'uptime_ms,quality,api_error,range_status,raw_mm,filtered_mm,baseline_mm,state,event,marker'
$watch = [Diagnostics.Stopwatch]::StartNew()
$sequence = 0
$successful = 0
$previousUptime = $null
$bootSegment = 0
$nextCaptureAt = 0.0
Write-Output ('Saving sensor snapshots to: ' + $outputPath)
while ($nextCaptureAt -lt $DurationSeconds -and $watch.Elapsed.TotalSeconds -lt $DurationSeconds) {
    $sequence++
    $capturedAt = [DateTime]::UtcNow.ToString('o')
    $entry = [ordered]@{sequence=$sequence; captured_at_utc=$capturedAt; success=$false}
    try {
        $status = Invoke-RestMethod -Uri ($base + '/api/status') -TimeoutSec 5
        $response = Invoke-WebRequest -Uri ($base + '/api/samples.csv') -TimeoutSec 8
        $csv = [string]$response.Content
        if (($csv -split '\r?\n', 2)[0] -ne $expectedHeader) {
            throw 'Unexpected CSV header; snapshot was not accepted.'
        }
        $rows = @($csv | ConvertFrom-Csv)
        if ($rows.Count -gt 1200) { throw 'Snapshot exceeds the known device buffer capacity.' }
        if ($null -ne $previousUptime -and [double]$status.uptime_seconds -lt $previousUptime) {
            $bootSegment++
            $entry['restart_observed'] = $true
        }
        $previousUptime = [double]$status.uptime_seconds
        $snapshotName = 'samples-{0:D6}.csv' -f $sequence
        [IO.File]::WriteAllText((Join-Path $outputPath $snapshotName), $csv, [Text.UTF8Encoding]::new($false))
        $entry['success'] = $true
        $entry['snapshot'] = $snapshotName
        $entry['rows'] = $rows.Count
        $entry['observed_boot_segment'] = $bootSegment
        $entry['status'] = $status
        if ($rows.Count -gt 0) {
            $entry['first_sample_uptime_ms'] = [long]$rows[0].uptime_ms
            $entry['last_sample_uptime_ms'] = [long]$rows[-1].uptime_ms
        }
        $successful++
        Write-Output ('Capture {0}: {1} rows, distance={2} mm, state={3}, invalid={4}' -f $sequence, $rows.Count, $status.distance_mm, $status.detector_state, $status.invalid_samples)
    } catch {
        $entry['error'] = $_.Exception.Message
        Write-Warning ('Capture {0} failed: {1}' -f $sequence, $_.Exception.Message)
    }
    ($entry | ConvertTo-Json -Depth 6 -Compress) | Add-Content -LiteralPath $journal -Encoding utf8
    $nextCaptureAt += $IntervalSeconds
    $waitSeconds = [Math]::Min($nextCaptureAt, $DurationSeconds) - $watch.Elapsed.TotalSeconds
    if ($waitSeconds -gt 0) { Start-Sleep -Milliseconds ([int]($waitSeconds * 1000)) }
}
Write-Output ('Completed: {0}/{1} snapshots saved.' -f $successful, $sequence)
if ($successful -eq 0) { throw 'No sensor snapshots were saved. See captures.jsonl for connection errors.' }
