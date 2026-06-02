# STM32WB / STM32F7 Multi-Link Communication Firmware For Energetic Systems

Firmware se skládá ze dvou součástí:
- **STM32WB55** - BLE koprocesor BLE_DataThroughput (server a klient)
- **STM32F7** - hlavní procesor řídící výběr spojení

Projekt byl vygenerovaný pomocí **STM32CubeIDE / STM32CubeMX**, vlastní kód je v sekcích `USER CODE`.

---

## Struktura projektu STM32F7:

```
switching-algorithm/
├── Core/               # Vlastní kód aplikace
├── Drivers/            # HAL a CMSIS ovládače (generované CubeIDE)
├── LWIP/               # LwIP TCP/IP stack (generovaný CubeIDE)
├── Middlewares/        # FreeRTOS a ostatné middleware (generované CubeIDE)
├── switching-algorithm.ioc         # STM32CubeMX konfigurační soubor
├── STM32F767ZITX_FLASH.ld          # Flash
└── STM32F767ZITX_RAM.ld            # RAM

Vlastni soubory v Core/:
├── main.c              # Inicializace, FreeRTOS tasky, I2C slave callbacks
├── d_decision.c        # Algoritmy výběru nejlepší linky
├── eth_metrics.c       # Výpočet delay, jitteru, ztráty paketov, skóre reliability
├── eth_parser.c        # Parsovani hlavičky UDP testovacího rámce
├── drv_bt.c            # Čtení metrik z BT BT cez I2C (master read)
├── drv_eth.c           # UDP socket cez LwIP
└── drv_rs485.c         # RS485 driver s DE pinom (nepoužité)

PC nástroje (Python):
├── csv_logger.py       #Vizualizace metrík + ukladani do CSV
└── echotest.py         # UDP echo server pro testovani konektivity
```

---


## Hodinová konfigurace (STM32F7)

| Zdroj | Frekvencia | Použitie |
|-------|-----------|----------|
| HSE | 25 MHz | Vstup pre PLL |
| PLL | 216 MHz | SYSCLK (max pre STM32F7) |
| APB1 | 54 MHz | I2C, UART |
| APB2 | 108 MHz | Timery |
| TIM1 | - | HAL tick zdroj (namiesto SysTick) |

---

## Konfigurace periférií

| Periféria | Piny | Použitie |
|-----------|------|----------|
| USART3 | - | Debug UART (printf → `__io_putchar`) |
| I2C1 | PB8 (SCL), PB9 (SDA) | I2C slave — príjem BT dát |
| ETH | - | Ethernet, LwIP stack |
| DMA | - | I2C1 RX/TX |

---

## FreeRTOS tasky

| Task | Priorita | Popis |
|------|----------|-------|
| `StartDefaultTask` | Normal | LwIP + ETH metriky + rozhodovanie |
| `rs_taskENTRY` | Normal | I2C slave príjem BT dát |
| `eth_taskENTRY` | Normal | Rezervovaný (prázdny) |

---
Cizí knihovny (Middlewares/Drivers)
## FreeRTOS

Zdroj: `Middlewares/Third_Party/FreeRTOS/`
CMSIS wrapper: CMSIS_RTOS a CMSIS_RTOS_V2
Hlavičky: `FreeRTOS/Source/include/`

## LwIP (Lightweight IP stack)

Zdroj: `Middlewares/Third_Party/LwIP/`
Konfigurace projektu: `LWIP/App/`, `LWIP/Target/`
Používané moduly:

`lwip/sockets.h` - BSD socket API (UDP, TCP)
`lwip/udp.h` - RAW UDP PCB API
`lwip/netif.h` - síťové rozhraní (netif)
`lwip/ip4_addr.h` - IPv4 adresy
`lwip/stats.h` - statistiky stacku
`lwip/opt.h` - konfigurační makra
`lwip/sys.h` - systémová abstrakce





## STM32F7xx HAL Driver

Zdroj: `Drivers/STM32F7xx_HAL_Driver/`
Hlavičky: `Inc/` a `Inc/Legacy/`
Používané periferie: ETH, I2C, UART, DMA, GPIO, TIM, MPU, RCC

## CMSIS

Zdroj: `Drivers/CMSIS/`
`Device/ST/STM32F7xx/Include/` - STM32F7 definice registrů

## BSP LAN8742

Zdroj: `Drivers/BSP/Components/lan8742/`
Ovladač pro Ethernet PHY čip LAN8742 (RMII rozhraní)
---
## Python nástroje (PC strana)

### `csv_logger.py`
Live vizualizácia metrík prijímaných zo STM32 cez sériový port (UART) s ukladaním do CSV súboru.

Používané knižnice:
- `pyserial` (`serial`) — čítanie dát zo sériového portu
- `matplotlib` (`pyplot`, `animation.FuncAnimation`, `lines.Line2D`) — živé grafy s animáciou
- `csv` — zápis metrík do súboru
- `collections.defaultdict`, `collections.deque` — kĺzavé buffery s obmedzenou dĺžkou

**Konfigurácia (konštanty na začiatku súboru):**
- `PORT` — COM port STM32 zariadenia (napr. `"COM10"`)
- `BAUDRATE` — prenosová rýchlosť (115200)
- `MAX_POINTS` — počet zobrazených vzoriek v grafe (300)
- `SAVE_FILE` — výstupný CSV súbor
- `INCLUDE_PATHS` — množina `path_id` hodnôt, ktoré sa vykreslia (0=ETH_DIRECT, 1=ETH_PLC, 2=BT; RS485 je predvolene skrytý)

**Očakávaný formát riadku zo sériového portu:**
```
CSV,<tick_ms>,<link_type>,<path_id>,<rtt_ms>,<jitter_ms>,<throughput_bps>,<loss_permille>,<reliability_score>,<t_expected_ms>,<timing_error_ms>,<missed_intervals>,<seq>
```
Riadky bez prefixu `CSV,` sa ignorujú pre graf (vypíšu sa na konzolu).


---

### `echotest.py`
Jednoduchý UDP echo server pre testovanie sieťovej konektivity.

Používané knižnice:
- `socket` (štandardná Python knižnica) — UDP socket

**Funkcia:** Počúva na porte `12345` (všetky rozhrania), pri každom prijatom UDP pakete vypíše jeho veľkosť a zdrojovú adresu, potom paket odošle späť odosielateľovi (echo). Slúži na overenie základnej UDP komunikácie a meranie round-trip time zo strany STM32.
