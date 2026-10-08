param(
    [Parameter(Mandatory = $true)][string]$BaseUrl,
    [ValidateRange(10, 45)][int]$PhaseSeconds = 30,
    [ValidateRange(1, 10)][int]$DownloadIntervalSeconds = 10,
    [string]$OutputDirectory = ''
)
$ErrorActionPreference = 'Stop'
$base = $BaseUrl.TrimEnd('/')
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path (Split-Path -Parent $PSScriptRoot) ('build\experiments\capture-timing-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
}
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Use a new output directory.' }
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
Write-Output ('Timing experiment: ' + $OutputDirectory)
$downloads = [Collections.Generic.List[object]]::new()
$summary = [Collections.Generic.List[object]]::new()
function Save-Snapshot([string]$Name) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $at = [DateTime]::UtcNow.ToString('o')
    $response = Invoke-WebRequest -Uri ($base + '/api/samples.csv') -TimeoutSec 10
    $timer.Stop()
    $content = [string]$response.Content
    if (-not $content.StartsWith('uptime_ms,quality,')) { throw 'Unexpected device CSV.' }
    [IO.File]::WriteAllText((Join-Path $OutputDirectory ($Name + '.csv')), $content, [Text.UTF8Encoding]::new($false))
    $downloads.Add([pscustomobject]@{snapshot=$Name;utc=$at;request_ms=$timer.ElapsedMilliseconds})
    return @($content | ConvertFrom-Csv)
}
foreach ($phase in @('quiet_before', 'downloads', 'quiet_after')) {
    $status = Invoke-RestMethod -Uri ($base + '/api/status') -TimeoutSec 5
    # Exclude the preceding snapshot's download pause and second-resolution boundary.
    $startMs = ([long]$status.uptime_seconds + 1) * 1000
    Write-Output ('Starting ' + $phase)
    if ($phase -eq 'downloads') {
        $clock = [Diagnostics.Stopwatch]::StartNew()
        $n = 0
        $downloadCount = [Math]::Ceiling($PhaseSeconds / [double]$DownloadIntervalSeconds)
        while ($n -lt $downloadCount -and $clock.Elapsed.TotalSeconds -lt $PhaseSeconds) {
            $n++
            $null = Save-Snapshot ('downloads-' + $n)
            $remaining = [Math]::Min($n * $DownloadIntervalSeconds, $PhaseSeconds) - $clock.Elapsed.TotalSeconds
            if ($remaining -gt 0) { Start-Sleep -Milliseconds ([int][Math]::Ceiling($remaining * 1000)) }
        }
    } else { Start-Sleep -Seconds $PhaseSeconds }
    $rows = @(Save-Snapshot ($phase + '-final') | Where-Object { [long]$_.uptime_ms -ge $startMs -and $_.quality -notin @('manual_marker','calibration_reset') })
    if ($rows.Count -lt 2) { throw 'Insufficient retained samples; possible reboot or connection problem.' }
    $intervals = @(for ($i=1; $i -lt $rows.Count; $i++) { [long]$rows[$i].uptime_ms - [long]$rows[$i-1].uptime_ms })
    $ordered = @($intervals | Sort-Object)
    $result = [pscustomobject]@{
        phase=$phase; sample_count=$rows.Count; first_ms=[long]$rows[0].uptime_ms; last_ms=[long]$rows[-1].uptime_ms
        median_interval_ms=$ordered[[int][Math]::Floor($ordered.Count/2)]
        p95_interval_ms=$ordered[[int][Math]::Ceiling($ordered.Count*0.95)-1]
        max_interval_ms=$ordered[-1]; gaps_over_200ms=@($intervals | Where-Object { $_ -gt 200 }).Count
        invalid_rows=@($rows | Where-Object quality -ne 'valid').Count
    }
    $summary.Add($result)
    $result | ConvertTo-Json -Compress | Write-Output
    @($summary.ToArray()) | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'summary.json') -Encoding utf8
    @($downloads.ToArray()) | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'downloads.json') -Encoding utf8
}
