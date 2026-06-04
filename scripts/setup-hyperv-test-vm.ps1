# PET Retro Mini — Hyper-V Test VM Setup
# Run this from an elevated PowerShell prompt (Admin)

Write-Host "=== PET Retro Mini — Hyper-V Test VM Setup ===" -ForegroundColor Green

# 1. Check if Hyper-V is enabled
$hvState = (Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V).State
Write-Host "Hyper-V State: $hvState" -ForegroundColor Cyan

if ($hvState -ne "Enabled") {
    Write-Host "Enabling Hyper-V... (may require reboot)" -ForegroundColor Yellow
    Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V -All
    Write-Host "REBOOT REQUIRED. Run this script again after reboot." -ForegroundColor Red
    exit
}

# 2. Check for Hyper-V PowerShell module
try {
    Import-Module Hyper-V -ErrorAction Stop
    Write-Host "Hyper-V module loaded." -ForegroundColor Green
} catch {
    Write-Host "Installing Hyper-V PowerShell module..." -ForegroundColor Yellow
    Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-Management-PowerShell
}

# 3. Create virtual switch (if not exists)
$switchName = "PET-Test-Switch"
$switch = Get-VMSwitch -Name $switchName -ErrorAction SilentlyContinue
if (-not $switch) {
    Write-Host "Creating internal virtual switch: $switchName" -ForegroundColor Yellow
    New-VMSwitch -Name $switchName -SwitchType Internal
} else {
    Write-Host "Virtual switch '$switchName' already exists." -ForegroundColor Green
}

# 4. Define VM parameters
$vmName = "PET-Retro-Test"
$vmPath = "C:\VMs\PET-Retro-Test"
$isoPath = "C:\VMs\Win11_Dev_Eval.iso"  # Download from: https://developer.microsoft.com/en-us/windows/downloads/virtual-machines/
$ram = 4096
$diskSize = 60GB

# 5. Create VM directory
New-Item -ItemType Directory -Path $vmPath -Force | Out-Null

# 6. Check if VM already exists
$vm = Get-VM -Name $vmName -ErrorAction SilentlyContinue
if ($vm) {
    Write-Host "VM '$vmName' already exists." -ForegroundColor Green
} else {
    Write-Host "Creating VM: $vmName" -ForegroundColor Yellow
    
    # Create VM
    New-VM -Name $vmName -MemoryStartupBytes $ram -BootDevice VHD -Path $vmPath -Generation 2 -SwitchName $switchName
    
    # Create VHDX
    New-VHD -Path "$vmPath\$vmName.vhdx" -SizeBytes $diskSize -Dynamic
    
    # Attach VHDX
    Add-VMHardDiskDrive -VMName $vmName -Path "$vmPath\$vmName.vhdx"
    
    # Add DVD drive with ISO
    if (Test-Path $isoPath) {
        Add-VMDvdDrive -VMName $vmName -Path $isoPath
        Write-Host "ISO attached. Boot from DVD to install Windows." -ForegroundColor Green
    } else {
        Write-Host "ISO not found at $isoPath. Download Windows 11 Dev VM from:" -ForegroundColor Yellow
        Write-Host "  https://developer.microsoft.com/en-us/windows/downloads/virtual-machines/" -ForegroundColor Yellow
        Write-Host "Place the ISO at $isoPath, then attach manually." -ForegroundColor Yellow
    }
    
    # Configure VM
    Set-VM -Name $vmName -ProcessorCount 2 -AutomaticStartAction Nothing -AutomaticStopAction Shutdown
    Set-VM -Name $vmName -CheckpointType Production
    Enable-VMIntegrationService -VMName $vmName -Name "Guest Service Interface"
    
    # Enable Enhanced Session Mode
    Set-VMHost -EnableEnhancedSessionMode $true
}

# 7. Create build output share
$sharePath = "C:\VMs\PET-Retro-Builds"
New-Item -ItemType Directory -Path $sharePath -Force | Out-Null
Write-Host "Build output share: $sharePath" -ForegroundColor Cyan
Write-Host "  (Copy .exe here after build, mount as network drive in VM)" -ForegroundColor Cyan

# 8. Summary
Write-Host ""
Write-Host "=== Setup Complete ===" -ForegroundColor Green
Write-Host "VM Name:       $vmName" -ForegroundColor White
Write-Host "RAM:           $ram MB" -ForegroundColor White
Write-Host "Disk:          $diskSize GB (dynamic)" -ForegroundColor White
Write-Host "Switch:        $switchName" -ForegroundColor White
Write-Host "Build share:   $sharePath" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Start VM in Hyper-V Manager" -ForegroundColor Yellow
Write-Host "  2. Install Windows from ISO" -ForegroundColor Yellow
Write-Host "  3. Build pet-retro-mini.exe" -ForegroundColor Yellow
Write-Host "  4. Copy to $sharePath" -ForegroundColor Yellow
Write-Host "  5. Test in VM" -ForegroundColor Yellow
