$ErrorActionPreference = 'SilentlyContinue'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$base = 'D:\HJZ'
$map = @{
'Project_01_运动会分数统计'='sports.exe'
'Project_02_一元稀疏多项式计算器'='poly.exe'
'Project_03_幼儿园培训班学生信息管理'='kindergarten.exe'
'Project_04_航空客运订票系统'='airline.exe'
'Project_05_图的存储及相互转换'='graph.exe'
'Project_06_图遍历的演示'='graph.exe'
'Project_07_成绩信息管理'='score.exe'
'Project_08_图书管理'='library.exe'
'Project_09_模拟小商店管理系统'='shop.exe'
'Project_10_期刊管理'='journal.exe'
'Project_11_机动车违章信息管理系统'='vio.exe'
'Project_12_学生社团成员信息管理系统'='clubmgr.exe'
'Project_13_租房信息管理系统'='rental.exe'
'Project_14_看病预约信息管理系统'='hosp_v2.exe'
'Project_15_高考志愿填报信息管理系统'='gaokao.exe'
'Project_16_观影网站影评信息管理系统'='movie.exe'
'Project_17_场地使用申请信息管理系统'='venue.exe'
'Project_18_学生学籍管理系统'='stu.exe'
'Project_19_教师工资信息管理系统'='salary.exe'
'Project_20_书刊租借信息管理系统'='borrow.exe'
'Project_21_仓库管理信息系统'='warehouse.exe'
'Project_22_研究生管理信息系统'='grad.exe'
'Project_23_图书馆管理信息系统'='lib.exe'
'Project_24_学生宿舍管理信息系统'='dorm.exe'
'Project_25_理财管理信息系统'='finance.exe'
'Project_26_IT设备资产管理系统'='asset.exe'
'Project_27_员工管理信息系统'='emp.exe'
'Project_28_软件公司客户关系管理系统'='crm.exe'
'Project_29_车辆管理系统'='veh.exe'
'Project_30_快餐店点餐系统'='food.exe'
'Project_31_通讯录管理系统'='contact.exe'
'Project_32_教学计划编制问题'='sched.exe'
'Project_33_校园导游咨询'='guide.exe'
'Project_34_高校专用通信网络建设'='net.exe'
'Project_35_全国交通咨询模拟'='traff.exe'
'Project_36_关键路径问题'='cpm.exe'
'Project_37_学校超市选址问题'='loc.exe'
'Project_38_哈希表设计'='hash.exe'
'Project_40_简单银行账户系统'='bank.exe'
}
$pass = 0; $fail = 0; $fails = @()
foreach ($name in $map.Keys | Sort-Object) {
    $exe = $map[$name]
    $dir = Join-Path $base $name
    $exePath = Join-Path $dir $exe
    if (-not (Test-Path $exePath)) { Write-Host "$name : EXE MISSING" -ForegroundColor Red; $fail++; $fails += "$name: missing $exe"; continue }
    Push-Location $dir
    $out = & ".\$exe" -t 2>&1
    $ec = $LASTEXITCODE
    Pop-Location
    $txt = ($out | Out-String)
    $ok = ($ec -eq 0) -and ($txt -match '通过|PASS|完成|passed') -and ($txt -notmatch 'FAIL|\[FAIL\]|失败')
    if ($ok) { Write-Host "$name : PASS" -ForegroundColor Green; $pass++ }
    else { Write-Host "$name : FAIL (ec=$ec)" -ForegroundColor Red; $fail++; $fails += "$name" ; Write-Host ($out | Select-Object -Last 5 | Out-String) }
}
Write-Host "`n=== Results: $pass passed, $fail failed ===" -ForegroundColor Cyan
if ($fails.Count -gt 0) { Write-Host "Failures:" -ForegroundColor Yellow; $fails | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow } }
