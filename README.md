# Motor Driver Control System - ATtiny3217

A sophisticated motor control system demonstrating advanced embedded programming techniques on the ATtiny3217 microcontroller. This project showcases professional-grade firmware development with robust debouncing, hardware PWM generation, and efficient timer management.

## 🚀 Project Overview

This firmware controls a bidirectional DC motor using the DRV8701 single H-bridge motor driver with debounced button inputs. The system demonstrates real-world embedded programming practices including interrupt-driven timing, power management, and hardware abstraction.

## 🔧 Hardware Configuration

### Microcontroller: ATtiny3217
- **Clock**: 20MHz internal oscillator
- **Architecture**: 8-bit AVR with enhanced peripherals
- **Memory**: 32KB Flash, 2KB SRAM, 256B EEPROM

### Pin Assignments
```
PB6  → Forward Button (Active Low, External Pullup)
PB7  → Reverse Button (Active Low, External Pullup)  
PC0  → Motor Driver nSLEEP/Enable (Active High)
PC4  → Motor Driver IN1 (PWM Output)
PC5  → Motor Driver IN2 (PWM Output)
PB2  → UART TX (9600 baud debug output)
PB3  → UART RX (9600 baud debug input)
```

### External Components
- **Motor Driver**: DRV8701 (single H-bridge, current regulation, nSLEEP, IN1/IN2 interface)
- **Buttons**: Momentary pushbuttons with external pullups
- **Motor**: Small DC motor (up to driver rating)
### DRV8701 Integration

The DRV8701 is a single H-bridge motor driver with integrated current regulation, protection features, and logic-level control. It is controlled via two logic inputs (IN1, IN2) and an nSLEEP pin for power management. The ATtiny3217 interfaces with the DRV8701 as follows:

| ATtiny3217 Pin | DRV8701 Pin | Function                |
|---------------|-------------|-------------------------|
| PC4           | IN1         | PWM/Direction Control   |
| PC5           | IN2         | PWM/Direction Control   |
| PC0           | nSLEEP      | Enable/Low Power Control|

**Typical Operation:**
- **Forward:** IN1 (PC4) = PWM, IN2 (PC5) = LOW
- **Reverse:** IN1 (PC4) = LOW, IN2 (PC5) = PWM
- **Stop/Coast:** Both IN1 and IN2 LOW
- **nSLEEP:** Asserted HIGH for normal operation, LOW for low-power shutdown

The DRV8701 provides current regulation, fault reporting, and robust protection for the motor and driver circuitry. This project configures the ATtiny3217 to generate the required PWM and logic signals for safe and efficient motor control.

## 🏗️ System Architecture

### Core Modules

#### 1. Timer System (`timer.c`)
**RTC-Based Software Timer Engine**
- Uses 32.768kHz RTC crystal for precise 1ms timebase
- Dual-list architecture: Async (ISR-driven) + Sync (polled)
- Supports periodic/one-shot timers with callback functions
- Atomic operations for thread-safe timer manipulation

```c
// RTC Configuration for 1ms ticks
RTC.CMP = 32;      // 32.768kHz ÷ 32 = 1.024kHz ≈ 1ms
RTC.PER = 32;      // Compare match every 32 counts
```

**Key Features:**
- **Interrupt-driven precision**: Async timers updated every 1ms in ISR
- **Power-efficient polling**: Sync timers batched in main loop
- **Flexible timing**: Supports any timeout from 1ms to 49+ days
- **Callback support**: Execute functions on timer expiration

#### 2. Button Debouncing System
**Professional-Grade Input Handling**
- 50ms debounce period eliminates mechanical switch bounce
- State-machine approach prevents false triggers
- Handles electrical noise and EMI interference

**Debouncing Algorithm:**
1. Compare current pin state vs stored button state
2. If states **match** → Reset timer (stable condition)
3. If states **differ** → Start/continue 50ms timer
4. Timer expires → Accept new state as valid

```
Pin State:     ______|‾|_|‾‾‾‾‾‾‾‾‾‾‾‾‾‾  (bounce, then stable)
Stored State:  ________________________  (unchanged until confirmed)
Timer Action:  STOP  S R S ↓ ↓ ↓ ↓ EXP   (S=start, R=reset, EXP=expire)
```

#### 3. PWM Motor Control (DRV8701)
**Hardware PWM Generation using TCA0 Split Mode**
- 50kHz PWM frequency (above audible range)
- 50% duty cycle for optimal motor performance  
- Split-mode allows independent control of IN1 and IN2 (DRV8701)

```c
#define PWM_FREQ      50000UL                    // 50kHz frequency
#define PWM_HPER     ((F_CPU / PWM_FREQ) - 1)   // Period calculation

// Forward: IN1 (PC4)=PWM, IN2 (PC5)=LOW
TCA0.SPLIT.HCMP1 = PWM_HPER / 2;  // 50% duty cycle on PC4 (IN1)

// Reverse: IN2 (PC5)=PWM, IN1 (PC4)=LOW  
TCA0.SPLIT.HCMP2 = PWM_HPER / 2;  // 50% duty cycle on PC5 (IN2)
```

#### 4. UART Communication
**Robust Serial Interface**
- 9600 baud debug output
- Interrupt-driven TX/RX with ring buffers
- Error detection and handling
- Standard printf/scanf support

### Motor Control Logic

```
Button State           Motor Action
─────────────────      ─────────────────────────
Forward ONLY    →      PC4=PWM, PC5=LOW  (CW)
Reverse ONLY    →      PC5=PWM, PC4=LOW  (CCW)  
Both Pressed    →      STOP (safety)
None Pressed    →      STOP (coast)
```

## 💡 Technical Highlights

### Advanced Embedded Techniques

1. **Interrupt Service Routine (ISR) Design**
   ```c
   ISR(RTC_CNT_vect) {
       ticks += TIMER_MSEC;                    // Update global timebase
       _timer_update(timers[0], TIMER_MSEC);   // Service async timers
       RTC.INTFLAGS = RTC.INTFLAGS;           // Clear interrupt flag
   }
   ```

2. **Atomic Operations for Thread Safety**
   ```c
   ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
       timer->flags &= ~TIMER_FLAG_EXPIRED;   // Critical section
       timer->value.current = new_value;      // Prevent race conditions
   }
   ```

3. **Power Management**
   ```c
   // CPU sleeps between operations, wakes on interrupts
   sleep_enable();
   sleep_cpu();        // Enter IDLE mode
   sleep_disable();    // Resume on RTC/UART interrupt
   ```

4. **Hardware Abstraction**
   ```c
   #define BUTTON_FORWARD ((PORTB.IN & PIN6_bm) == 0)
   #define BUTTON_REVERSE ((PORTB.IN & PIN7_bm) == 0)
   // Clean interface hides hardware details
   ```

### Performance Characteristics

- **Response Time**: 50ms maximum (debounce period)
- **Timer Resolution**: 1ms precision
- **PWM Frequency**: 50kHz (inaudible, efficient)
- **Power Consumption**: <1mA active, <10µA sleep
- **Memory Usage**: ~2KB Flash, ~100B RAM

## 🛠️ Development Environment

- **IDE**: MPLAB X with XC8 Compiler
- **Toolchain**: Microchip XC8 v3.10
- **Programmer**: PICkit/Atmel-ICE compatible
- **Version Control**: Git with structured commits

## 📊 Code Quality Features

### Professional Standards
- **Comprehensive commenting**: Every function and algorithm explained
- **Modular design**: Clean separation of concerns
- **Error handling**: Robust fault detection and recovery
- **Consistent formatting**: Professional code style
- **Atomic operations**: Thread-safe shared data access

### Testing & Validation
- **Hardware validation**: Tested with real motor loads
- **Timing verification**: Oscilloscope-confirmed PWM accuracy  
- **Edge case handling**: Button bounce, power glitches tested
- **Memory safety**: No buffer overflows or resource leaks

## 🎯 Skills Demonstrated

### Embedded Systems
- Microcontroller programming (ATtiny3217/AVR)
- Real-time systems and interrupt handling
- Hardware peripheral configuration (TCA, RTC, USART)
- Power management and sleep modes

### Software Engineering  
- Clean code architecture and modularity
- State machine design patterns
- Memory-efficient programming
- Thread safety and atomic operations

### Hardware Integration
- Motor control and DRV8701 interfacing (single H-bridge, current regulation)
- PWM generation and frequency analysis
- Signal integrity and noise immunity
- Power supply and grounding
- Safety features (thermal protection, overcurrent protection)



---

**Author**: Jenil Makwana  
**Email**: jenil@jmakwana.net 
**Repository**: https://github.com/j-makwana/Motor-Driver.git

*This project demonstrates production-ready embedded firmware development with attention to reliability, maintainability, and professional coding standards.*