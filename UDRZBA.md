# UDRŽBA.md

## Změna časového offsetu (letní/zimní čas)

Najdi řádek s `NTPClient`:
```cpp
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 1000);
```

Třetí parametr (`3600`) je offset v sekundách:
- **Zimní čas (UTC+1)**: `3600` (1 hodina)
- **Letní čas (UTC+2)**: `7200` (2 hodiny)

**Příklad pro letní čas:**
```cpp
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 1000);
```

## Změna WiFi sítě (SSID a hesla)

Najdi tyto řádky na začátku kódu:
```cpp
const char* ssid = "robotika";
const char* password = "19BAVL79";
```

Přepiš hodnoty podle nového routeru:
```cpp
const char* ssid = "TvujNovySSID";
const char* password = "TvojeNoveHeslo";
```

**Důležité:** Před nahráním kódu do Arduina Mega, nezapomeň změnit pořadí dip switchů na desce. Pokud nevíš jak, podívej se do [DIP_SWITCH.md](DIP_SWITCH.md)