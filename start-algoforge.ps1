param(
    [switch]$NoBrowser,
    [switch]$SkipMysql
)

$ErrorActionPreference = "Continue"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$FrontendDir = Join-Path $Root "frontend"
$BackendLinuxExe = Join-Path $Root "backend\build\AlgoArchiveServer"

$BackendPort = 8080
$FrontendPort = 5173
$MysqlPort = 3306

function Test-PortListening {
    param([int]$Port)

    try {
        $connections = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue
        return $null -ne $connections
    } catch {
        $netstat = netstat -ano | Select-String ":$Port "
        return $null -ne $netstat
    }
}

function Test-BackendAuthRoute {
    try {
        Invoke-WebRequest `
            -Uri "http://localhost:$BackendPort/api/auth/me" `
            -Method Get `
            -TimeoutSec 2 `
            -UseBasicParsing `
            -ErrorAction Stop | Out-Null
        return $true
    } catch {
        if ($_.Exception.Response -and [int]$_.Exception.Response.StatusCode -eq 401) {
            return $true
        }
        return $false
    }
}

function Convert-ToWslPath {
    param([string]$WindowsPath)

    $fullPath = (Resolve-Path $WindowsPath).Path
    if ($fullPath -notmatch "^([A-Za-z]):\\(.*)$") {
        throw "Cannot convert path to WSL path: $fullPath"
    }

    $drive = $Matches[1].ToLower()
    $rest = $Matches[2] -replace "\\", "/"
    return "/mnt/$drive/$rest"
}

function Start-PowerShellWindow {
    param(
        [string]$Title,
        [string]$Command
    )

    $safeTitle = $Title.Replace("'", "''")
    $wrapped = @"
`$Host.UI.RawUI.WindowTitle = '$safeTitle'
$Command
Write-Host ''
Write-Host 'Process ended. Press Enter to close this window.'
Read-Host
"@

    $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($wrapped))
    Start-Process powershell.exe -ArgumentList @(
        "-NoExit",
        "-ExecutionPolicy",
        "Bypass",
        "-EncodedCommand",
        $encoded
    )
}

function Start-MySqlIfPossible {
    if (Test-PortListening $MysqlPort) {
        Write-Host "[OK] MySQL appears to be listening on port $MysqlPort."
        return
    }

    Write-Host "[INFO] MySQL is not listening on port $MysqlPort. Trying to start a local MySQL/MariaDB service..."
    try {
        $services = Get-Service -ErrorAction Stop | Where-Object {
            $_.Name -match "mysql|mariadb" -or $_.DisplayName -match "MySQL|MariaDB"
        }

        if (-not $services) {
            Write-Warning "No local MySQL/MariaDB Windows service was found. Start your database manually if the backend cannot connect."
            return
        }

        foreach ($service in $services) {
            if ($service.Status -ne "Running") {
                Write-Host "[INFO] Starting service $($service.Name)..."
                Start-Service -Name $service.Name -ErrorAction Stop
            }
        }

        Start-Sleep -Seconds 3
        if (Test-PortListening $MysqlPort) {
            Write-Host "[OK] MySQL is now listening on port $MysqlPort."
        } else {
            Write-Warning "MySQL service was started, but port $MysqlPort is still not listening."
        }
    } catch {
        Write-Warning "Could not start MySQL automatically. If needed, run this script as Administrator or start MySQL manually."
        Write-Warning $_.Exception.Message
    }
}

Write-Host "=== AlgoForge startup ==="
Write-Host "Project: $Root"

if (-not $SkipMysql) {
    Start-MySqlIfPossible
}

if (Test-PortListening $BackendPort) {
    if (Test-BackendAuthRoute) {
        Write-Host "[OK] Backend port $BackendPort is already in use and the auth API is available."
    } else {
        Write-Warning "Backend port $BackendPort is already in use, but the auth API is missing."
        Write-Warning "This usually means an old backend is still running. Close the old backend window or stop the process on port $BackendPort, rebuild the WSL backend, then run this script again."
        Write-Warning "Rebuild in WSL:"
        Write-Warning "  cd /mnt/d/C_study/Algo"
        Write-Warning "  cmake -S backend -B backend/build -DALGOFORGE_USE_MYSQL=ON"
        Write-Warning "  cmake --build backend/build"
    }
} else {
    if (-not (Test-Path $BackendLinuxExe)) {
        Write-Warning "WSL backend executable was not found: $BackendLinuxExe"
        Write-Warning "Build it in WSL first:"
        Write-Warning "  cd /mnt/d/C_study/Algo"
        Write-Warning "  cmake -S backend -B backend/build -DALGOFORGE_USE_MYSQL=ON"
        Write-Warning "  cmake --build backend/build"
    } else {
        $wslRoot = Convert-ToWslPath $Root
        $wslCommand = "cd '$wslRoot/backend/build' && export ALGOFORGE_DB_HOST='127.0.0.1' ALGOFORGE_DB_PORT='3306' ALGOFORGE_DB_USER='algoforge' ALGOFORGE_DB_PASSWORD='algoforge' ALGOFORGE_DB_NAME='algoforge' && ./AlgoArchiveServer"
        $backendCommand = @"
Write-Host 'Starting AlgoForge backend through WSL...'
Write-Host 'Backend URL: http://localhost:$BackendPort'
& wsl.exe -e bash -lc "$wslCommand"
"@
        Start-PowerShellWindow -Title "AlgoForge Backend" -Command $backendCommand
        Write-Host "[OK] Backend window opened."
    }
}

if (Test-PortListening $FrontendPort) {
    Write-Host "[OK] Frontend port $FrontendPort is already in use. Assuming Vite is running."
} else {
    if (-not (Test-Path (Join-Path $FrontendDir "node_modules"))) {
        Write-Warning "frontend/node_modules was not found. Run this once before starting:"
        Write-Warning "  cd frontend"
        Write-Warning "  npm install"
    }

    $frontendCommand = @"
Set-Location '$FrontendDir'
Write-Host 'Starting AlgoForge frontend...'
Write-Host 'Frontend URL: http://localhost:$FrontendPort'
npm run dev
"@
    Start-PowerShellWindow -Title "AlgoForge Frontend" -Command $frontendCommand
    Write-Host "[OK] Frontend window opened."
}

if (-not $NoBrowser) {
    Start-Sleep -Seconds 3
    Start-Process "http://localhost:$FrontendPort"
    Write-Host "[OK] Browser opened: http://localhost:$FrontendPort"
}

Write-Host "=== Done ==="
Write-Host "Close the backend/frontend windows to stop the project."
