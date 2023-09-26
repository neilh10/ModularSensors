#  .\doBuild.ps1  0_xx_y
#$param1=$args[0]
#Get-Executionpolicy
#Set-ExecutionPolicy -Scope CurrentUser unrestricted
#param ($rel = $(throw "rel required"))
#write-host "doing $($args[0])  $($args.count) something" 
#for ( $i = 0; $i -lt $args.count; $i++ ) {
#    write-host "Argument  $i is $($args[$i])"
#} 


$config1 = $($args)
$config2 = Get-Date -Format "yyMMdd_HHmm"
$dest_dir = "..\..\..\releases"

#function touch {set-content -Path ($args[0]) -Value ($null)} 
function Do-Build {
    $dest_file = -join($dest_dir,"\mayfly_",$config1, "_",$config2,$hext,".hex")
    $dest_file2= -join($dest_dir,"\mayfly_",$config1, "_",$config2,$hext,".elf")
    $dest_file3= -join($dest_dir,"\mayfly_",$config1, "_",$config2,$hext,".map")
    $src_file  = -join("src\ms_cfg.h","$hext" )

    if (-not (Test-Path -Path $src_file)) {
        #Write-Ouput  "The file does not exist $src_file"
        throw  "The file does not exist $src_file"
    } else {
        Write-Output "`n`r**************************************"
        Write-Output "**** Build with src\ms_cfg.h$hext ****"
        copy src\ms_cfg.h$hext .\src\ms_cfg.h 
        $(ls .\src\ms_cfg.h).LastWriteTime = Get-Date
        Write-host $(cmd /r dir /s .\src\ms_cfg.h )
        pio run
        move .\.pio\build\mayfly\firmware.hex  $dest_file
        #move .\.pio\build\mayfly\firmware.elf  $dest_file2
        move .\.pio\build\mayfly\firmware.map  $dest_file3
        Write-Output "**** Build Output in $dest_file"
    }
}

Write-Output "`n Building $config2 "

if (-not (Test-Path -Path $dest_dir)) {
    #Write-Host  "The file does not exist $src_file"
    throw  "The file does not exist $dest_dir"
}

$envirodiy_dir = ".pio\libdeps\mayfly\EnviroDIY_ModularSensors"
#Remove-item $envirodiy_dir -Recurse -Force
if (Test-Path -Path $envirodiy_dir) {
    Write-Output  "Removing $envirodiy_dir to refresh for build`n"
    Remove-item $envirodiy_dir -Recurse -Force
    if (Test-Path -Path $envirodiy_dir) {
        Write-Output  "ERROR $envirodiy_dir still present`n"
    }
} else {
    Write-Output  " $envirodiy_dir already removed`n"
}
pio run --target clean

$hext = "_LT5KA_DigiLteXB3"
Do-Build

$hext = "_LT5_Mdbus_wireless"
Do-Build

#$hext = "_LT5_SDI12_wireless"
#Do-Build

$hext = "_nano"
Do-Build

#$hext = "_mmw_test"
#Do-Build

#$hext = "_ts_test"
#Do-Build

#$hext = "_ub_test"
#Do-Build

$hext = "_EC"
Do-Build