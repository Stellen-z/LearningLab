# Generate 3 simple WAV sound effects for Snake Game

$sampleRate = 44100

function New-Wav($path, $duration, $generator) {
    $count = [int]($sampleRate * $duration)
    $dataSize = $count * 2
    $fs = [System.IO.FileStream]::new($path, [System.IO.FileMode]::Create)
    $bw = [System.IO.BinaryWriter]::new($fs)

    # RIFF header
    $bw.Write([char[]]"RIFF")
    $bw.Write([int]($dataSize + 36))
    $bw.Write([char[]]"WAVE")

    # fmt subchunk
    $bw.Write([char[]]"fmt ")
    $bw.Write([int]16)              # subchunk size
    $bw.Write([int16]1)             # PCM
    $bw.Write([int16]1)             # mono
    $bw.Write([int]$sampleRate)
    $bw.Write([int]($sampleRate * 2)) # byte rate
    $bw.Write([int16]2)             # block align
    $bw.Write([int16]16)            # bits per sample

    # data subchunk
    $bw.Write([char[]]"data")
    $bw.Write([int]$dataSize)

    # PCM samples
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

# Eat sound: 800Hz sine, 100ms, linear decay
New-Wav "resource/eat.wav" 0.10 {
    param($i, $count)
    $env = 1.0 - $i / $count
    [int]([Math]::Sin(2 * [Math]::PI * 800 * $i / $sampleRate) * $env * 26000)
}

# Die sound: descending frequency 300Hz->80Hz, 500ms, sawtooth
New-Wav "resource/die.wav" 0.50 {
    param($i, $count)
    $t = $i / $sampleRate
    $freq = 300 + (80 - 300) * ($t / 0.50)
    $phase = ($freq * $i / $sampleRate) % 1.0
    $env = 1.0 - $t / 0.50
    [int](($phase * 2 - 1) * $env * 20000)
}

# Menu sound: 1200Hz sine, 50ms, quick decay
New-Wav "resource/menu.wav" 0.05 {
    param($i, $count)
    $env = 1.0 - $i / $count
    [int]([Math]::Sin(2 * [Math]::PI * 1200 * $i / $sampleRate) * $env * 16000)
}

Write-Output "All sounds generated."
