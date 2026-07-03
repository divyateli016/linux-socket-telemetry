# linux-socket-telemetry

# Low-Level Embedded Linux SocketCAN Instrument Cluster

A production-grade, bare-metal-to-user-space implementation of an automotive telemetry processing engine. Developed natively in C on a Raspberry Pi environment using the Linux network subsystem, this project acts as a single-threaded functional prototype for an Automotive Virtual Instrument Cluster. It completely avoids high-level non-native wrapper libraries to demonstrate core Linux systems engineering, low-level memory handling, and fieldbus network architecture.

## Systems Architecture Pipeline

The application treats physical fieldbus hardware exactly like a native Linux network interface, following a highly decoupled, layered abstraction pattern:

```text
 [Physical Bus] ──► [Transceiver & Controller] ──► [Device Tree Overlay] ──► [Kernel Space] ──► [User Space]
  Car ECU Signals    MCP2515 SPI CAN HAT            bcm2711 / can0 interface   SocketCAN Driver   Your C Code
```

1. **Hardware Configuration (Boot / Device Tree):** The bootloader loads a Device Tree Overlay to activate the SPI bus interface and configure the interrupt line mapping for the discrete CAN controller.
2. **Kernel Subsystem Allocation (SocketCAN)**: The kernel initializes the network device driver and maps the card as a global network interface (`can0`).
3. **User-Space Network Bridge (POSIX Socket)**: The application runs system programming calls to request an abstract network file descriptor, binding it directly to the kernel's real-time hardware ring buffer queue.
4. **Data Ingestion Loop (Event-Driven Blocking):** The program thread is placed into an efficient kernel sleep state, consuming 0.00% CPU resource until an explicit physical hardware interrupt triggers data copying across the boundary space.
5. **Telemetry Parsing (Bitwise Slicing):** The binary payloads are parsed using bitwise masking and arithmetic left-shifting to translate raw frames into human-readable metric layouts.


## core Systems Programming Concepts Covered

* **Device Tree Overlays (DTO):** Manipulated system-level hardware configurations at boot without modifying the core kernel source code.
* **POSIX Networking Socket API:** Leveraged the Unix standard programming blueprint (`socket()`, `bind()`) to instantiate raw protocol sockets (`PF_CAN`, `SOCK_RAW`, `CAN_RAW`).
* **Hardware Interface Translation (`ioctl`):** Implemented Input/Output Control system operations paired with common Interface Request (`struct ifreq`) union structures to map human text flags (`"can0"`) into unique, fast internal kernel index integers.
* **Polymorphic C Memory Typecasting:** Typecasted protocol-specific address contracts (`struct sockaddr_can`) into generic socket address envelopes (`struct sockaddr *`) to safely pass configuration descriptors through unified system gates.
* **Automotive Endianness Mitigation:** Remapped pre-decided network Big-Endian (Network Byte Order) bytes to match the native Little-Endian memory register format of the ARM CPU host via bit-shifting (`<< 8`) and bitwise OR (`|`) statements.
* **Event-Driven Blocking I/O vs Polling:** Leveraged kernel-level scheduler blocks (`read()`) to enforce high-efficiency thread state management rather than burning clock cycles spinning in high-waste polling patterns.
* **Bitmask Data Filtering:** Utilized bitwise AND operations (`&`) against standard kernel filters (`CAN_EFF_MASK`) to clear low-level system status/error flags out of core application tracking spaces.


## Kernel Bring-Up & `config.txt` Customization

To map the discrete MCP2515 controller and its transceiver to the operating system network stack, the hardware platform blueprint must be updated.

1. Edit the primary system boot parameters:
   ```bash
   sudo nano /boot/firmware/config.txt
   ```
2. Enable the Serial Peripheral Interface (SPI) bus and append the hardware system overlay block specifying clock oscillation speeds and active GPIO interrupt lines:
   ```text
   dtparam=spi=on
   dtoverlay=mcp2515-can0,oscillator=12000000,interrupt=25
   ```
3. Save changes and force a system-level hardware reboot:
   ```bash
   sudo reboot
   ```

---

## Manual Network Initialization

Because application software is decoupled from native link lifecycle states, the system network topology must be initialized before launch. For isolated device testing, the link is driven in **Local Loopback Mode** to route transmitted telemetry back through internal system buffers without requiring a second physical car node.

```bash
# 1. Bring the link infrastructure down to clear memory resource locks
sudo ip link set can0 down

# 2. Re-configure the bitrate parameters to 500kbps (standard vehicle high-speed velocity) and engage loopback
sudo ip link set can0 up type can bitrate 500000 loopback on

# 3. Verify link status flags natively inside the network device layout
ip link show can0
```
*Expected Diagnostic Output:* `<NOARP,UP,LOWER_UP,LOOPBACK>` inside the network layer status parameters.


## Code Ingestion Blueprint & Structure References

The core application code utilizes standard, timeless header libraries explicitly tied to the Linux networking kernel layout:
* `<sys/socket.h>` / `<linux/can.h>`: Exposes structure contracts like `struct sockaddr_can` and raw tracking frames `struct can_frame`.
* `<net/if.h>` / `<sys/ioctl.h>`: Mandates the polymorphic configuration interface templates `struct ifreq`.

### Telemetry Payload Frame Specification (`CAN ID: 0x100`)
The simulated vehicle Engine Control Unit (ECU) packages telemetry variables into compressed arrays to optimize fieldbus bus efficiency:
* **Bytes 0 and 1:** Engine RPM (16-bit Big-Endian Integer)
* **Byte 2:** Vehicle Speed (8-bit Unsigned Integer, km/h)
* **Byte 3:** Coolant Temperature (8-bit Unsigned Integer, °C)

## Compilation, Launch, & System Verification

The program compiles cleanly using standard GNU C Toolchains with zero external dependencies:

```bash
# Compile native binary layout
gcc can_receiver.c -o can_receiver

# Launch the processing engine
./can_receiver
```
*The terminal will clear and hang silently. System evaluation tools like `ps`, `htop`, or `strace` will confirm that the process thread has been put into an efficient `STAT: S` (Interruptible Sleep) block right at the `read()` gateway.*

### Live Injection Testing
Open a second terminal interface side-by-side to act as an external vehicle computer injecting mock data payloads via the loopback channel:

```bash
# Inject Payload: 3000 RPM (0x0BB8), 120 km/h (0x78), 95°C Coolant (0x5F)
cansend can0 100#0BB8785F00000000
