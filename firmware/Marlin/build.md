# C:\Users\Paul\.platformio\penv\Scripts
$env:Path += ";C:\Users\Paul\.platformio\penv\Scripts"

platformio run --target clean --silent -e LPC1769
platformio run --silent -e LPC1769
platformio run --target upload --silent -e LPC1769
