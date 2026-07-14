# Generate improved WAV sound effects for Snake Game
# eat: ascending chime, die: noise burst, menu: sharp tick

$sampleRate = 44100
$PI = [Math]::PI

function New-Wav($path, $duration, $generator) {
    $count = [int]($sampleRate * $duration)
    $dataSize = $count * 2
    $fs = [System.IO.FileStream]::new($path, [System.IO.FileMode]::Create)
    $bw = [System.IO.BinaryWriter]::new($fs)

    $bw.Write([char[]]"RIFF")
    $bw.Write([int]($dataSize + 36))
    $bw.Write([char[]]"WAVE")
    $bw.Write([char[]]"fmt ")
    $bw.Write([int]16)
    $bw.Write([int16]1)
    $bw.Write([int16]1)
    $bw.Write([int]$sampleRate)
    $bw.Write([int]($sampleRate * 2))
    $bw.Write([int16]2)
    $bw.Write([int16]16)
    $bw.Write([char[]]"data")
    $bw.Write([int]$dataSize)

    for ($i = 0; $i -lt $count; $i++) {
        $val = & $generator $i $count
        if ($val -gt 32767) { $val = 32767 }
        if ($val -lt -32768) { $val = -32768 }
        $bw.Write([int16]$val)
    }
    $bw.Close()
    $fs.Close()
    Write-Output "Generated: $path ($count samples, $([int]($duration*1000))ms)"
}

# Eat sound: ascending two-tone chime (like coin pickup)
New-Wav "resource/eat.wav" 0.15 {
    param($i, $count)
    $t = $i / $sampleRate
    $env = [Math]::Max(0, 1.0 - $t / 0.15)
    # First tone 800Hz, second tone 1200Hz after 50ms
    $freq = if ($t -lt 0.05) { 800 } else { 1200 }
    $phase = $freq * $t
    [int]([Math]::Sin(2 * $PI * $phase) * $env * 22000)
}

# Die sound: white noise + descending sweep
New-Wav "resource/die.wav" 0.45 {
    param($i, $count)
    $t = $i / $sampleRate
    $env = [Math]::Max(0, 1.0 - $t / 0.45)
    $freq = 400 - 300 * ($t / 0.45)  # 400Hz -> 100Hz
    $phase = $freq * $t
    $tone = [Math]::Sin(2 * $PI * $phase) * 0.6
    $noise = ((Get-Random -Min -10000 -Max 10000) / 20000.0) * 0.4 * (1.0 - $t / 0.45)
    [int](($tone + $noise) * $env * 26000)
}

# Menu sound: short sharp click with tail
New-Wav "resource/menu.wav" 0.06 {
    param($i, $count)
    $t = $i / $sampleRate
    $env = if ($t -lt 0.01) { 1.0 } else { [Math]::Max(0, 1.0 - ($t - 0.01) / 0.05) }
    $freq = 2000
    [int]([Math]::Sin(2 * $PI * $freq * $t) * $env * 18000)
}