Get-NetAdapter -Name *
Set-NetAdapter -Name "vEthernet*" -MacAddress "00-11-22-33-44-55" -Confirm:$false
Get-NetAdapter -Name *
