# P02 - drivers för ESP32-S3
* Driverbibliotek för mikrocontroller `ESP32-S3`
* Genomförs i kursen Hårdvarunära programmering C/C++

---

## Katalogstruktur
* **main**: Innehåller koden

---

## Ladda in mjukvaran i `ESP-32`

Öppna Powershell och kör
```powershell
C:\Espressif\tools\Microsoft.v6.0.PowerShell_profile.ps1
```

Möjligen kan du först behöva enabla script mha 
```powershell
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
```

skapa ett projekt genom att i (venv) miljön navigera till en lämplig plats
```powershell
cd c:\
mkdir temp
cd temp
mkdir blnk_demo
idf.py create-project blink_demo
```

## Kommandon för att bygga samt flasha
Öppna `ESP-32 IDF` och sätt mål:
```bash
idf.py set-target esp32s3
```

För att bygga: 
```bash
idf.py build
```

För att flasha till `ESP-32`:
```bash
idf.py -p COMx flash monitor 
```

