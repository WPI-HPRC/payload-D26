# Isolated OpenMV RT1062 UART-to-PWM bridge for screw-drive ESC bring-up.
# MARS sends: SERVO L <leftUs> R <rightUs>

import time
from machine import Pin, PWM, UART


UART_BUS = 1
BAUDRATE = 115200

LEFT_ESC_PWM_PIN = "P7"
RIGHT_ESC_PWM_PIN = "P8"
PWM_HZ = 50
MIN_PULSE_US = 1000
MAX_PULSE_US = 2000
DEFAULT_PULSE_US = 1500

ENABLE_STANDALONE_FIXED_TEST = False
ENABLE_STANDALONE_SWEEP_TEST = False
ENABLE_RX_CONSOLE_LOG = True
ENABLE_RX_UART_ECHO = True
ENABLE_STATUS_DEBUG_ECHO = True

STANDALONE_FIXED_LEFT_US = DEFAULT_PULSE_US
STANDALONE_FIXED_RIGHT_US = DEFAULT_PULSE_US
SWEEP_MIN_US = 1300
SWEEP_MAX_US = 1700
SWEEP_STEP_US = 25
SWEEP_STEP_DELAY_MS = 250
STATUS_INTERVAL_MS = 1000


command_line_buffer = ""
left_pulse_us = DEFAULT_PULSE_US
right_pulse_us = DEFAULT_PULSE_US


def safe_print(message):
    try:
        print(message)
    except Exception:
        pass


def clamp_pulse(pulse_us):
    if pulse_us < MIN_PULSE_US:
        return MIN_PULSE_US
    if pulse_us > MAX_PULSE_US:
        return MAX_PULSE_US
    return pulse_us


def pulse_to_duty_ns(pulse_us):
    return int(clamp_pulse(pulse_us)) * 1000


def set_pwm_us(pwm, pulse_us):
    pwm.duty_ns(pulse_to_duty_ns(pulse_us))


def apply_pulses(left_pwm, right_pwm, left_us, right_us):
    global left_pulse_us, right_pulse_us

    left_pulse_us = clamp_pulse(int(left_us))
    right_pulse_us = clamp_pulse(int(right_us))
    set_pwm_us(left_pwm, left_pulse_us)
    set_pwm_us(right_pwm, right_pulse_us)


def write_status(uart, prefix="CFG_SERVO"):
    line = "%s L=%d R=%d\n" % (prefix, left_pulse_us, right_pulse_us)
    uart.write(line)

    if ENABLE_STATUS_DEBUG_ECHO and prefix != "DBG_SERVO_STATUS":
        uart.write("DBG_SERVO_STATUS L=%d R=%d\n" % (left_pulse_us, right_pulse_us))


def write_error(uart, reason):
    uart.write("CFG_SERVO_ERR reason=%s\n" % reason)


def log_rx(uart, line):
    message = "DBG_SERVO_RX line=%s" % line

    if ENABLE_RX_CONSOLE_LOG:
        safe_print(message)

    if ENABLE_RX_UART_ECHO:
        uart.write(message + "\n")


def parse_int(token):
    try:
        return int(token)
    except Exception:
        return None


def handle_servo_command(uart, left_pwm, right_pwm, parts):
    if len(parts) != 5 or parts[1] != "L" or parts[3] != "R":
        write_error(uart, "bad_servo_format")
        return

    left_us = parse_int(parts[2])
    right_us = parse_int(parts[4])

    if left_us is None or right_us is None:
        write_error(uart, "bad_servo_value")
        return

    if (left_us < MIN_PULSE_US or left_us > MAX_PULSE_US or
            right_us < MIN_PULSE_US or right_us > MAX_PULSE_US):
        write_error(uart, "servo_out_of_range")
        return

    apply_pulses(left_pwm, right_pwm, left_us, right_us)
    write_status(uart)


def handle_uart_command(uart, left_pwm, right_pwm, line):
    line = line.strip()
    if len(line) == 0:
        return

    parts = line.upper().split()
    if len(parts) == 0:
        return

    if parts[0] == "SERVO":
        log_rx(uart, line)
        handle_servo_command(uart, left_pwm, right_pwm, parts)
        return

    if parts[0] == "PING_SERVO":
        log_rx(uart, line)
        token = ""
        if len(line.split(None, 1)) == 2:
            token = line.split(None, 1)[1]
        uart.write("DBG_SERVO_PONG token=%s\n" % token)
        return

    if parts[0] == "GET_SERVO":
        log_rx(uart, line)
        write_status(uart)
        return

    write_error(uart, "unknown_command")


def poll_uart_commands(uart, left_pwm, right_pwm):
    global command_line_buffer

    try:
        available = uart.any()
    except Exception:
        available = 0

    while available > 0:
        try:
            data = uart.read(1)
        except Exception:
            data = None

        if not data:
            break

        try:
            next_char = data.decode()
        except Exception:
            next_char = chr(data[0])

        if next_char == "\r":
            pass
        elif next_char == "\n":
            handle_uart_command(uart, left_pwm, right_pwm, command_line_buffer)
            command_line_buffer = ""
        else:
            command_line_buffer += next_char
            if len(command_line_buffer) > 96:
                command_line_buffer = ""
                write_error(uart, "line_overflow")

        try:
            available = uart.any()
        except Exception:
            available = 0


def run_fixed_test(uart, left_pwm, right_pwm):
    apply_pulses(left_pwm, right_pwm, STANDALONE_FIXED_LEFT_US, STANDALONE_FIXED_RIGHT_US)
    safe_print("DBG_SERVO_STANDALONE_FIXED L=%d R=%d" % (left_pulse_us, right_pulse_us))
    write_status(uart)

    last_status_ms = time.ticks_ms()
    while True:
        now = time.ticks_ms()
        if time.ticks_diff(now, last_status_ms) >= STATUS_INTERVAL_MS:
            safe_print("DBG_SERVO_STANDALONE_FIXED L=%d R=%d" % (left_pulse_us, right_pulse_us))
            write_status(uart)
            last_status_ms = now
        time.sleep_ms(20)


def run_sweep_test(uart, left_pwm, right_pwm):
    pulse_us = SWEEP_MIN_US
    direction = 1
    while True:
        apply_pulses(left_pwm, right_pwm, pulse_us, pulse_us)
        safe_print("DBG_SERVO_STANDALONE_SWEEP L=%d R=%d" % (left_pulse_us, right_pulse_us))
        write_status(uart)

        pulse_us += direction * SWEEP_STEP_US
        if pulse_us >= SWEEP_MAX_US:
            pulse_us = SWEEP_MAX_US
            direction = -1
        elif pulse_us <= SWEEP_MIN_US:
            pulse_us = SWEEP_MIN_US
            direction = 1

        time.sleep_ms(SWEEP_STEP_DELAY_MS)


uart = UART(UART_BUS, baudrate=BAUDRATE)
left_pwm = PWM(Pin(LEFT_ESC_PWM_PIN), freq=PWM_HZ, duty_ns=pulse_to_duty_ns(DEFAULT_PULSE_US))
right_pwm = PWM(Pin(RIGHT_ESC_PWM_PIN), freq=PWM_HZ, duty_ns=pulse_to_duty_ns(DEFAULT_PULSE_US))
apply_pulses(left_pwm, right_pwm, DEFAULT_PULSE_US, DEFAULT_PULSE_US)

uart.write("CFG_SERVO_READY left_pin=%s right_pin=%s hz=%d min=%d max=%d default=%d\n" % (
    LEFT_ESC_PWM_PIN,
    RIGHT_ESC_PWM_PIN,
    PWM_HZ,
    MIN_PULSE_US,
    MAX_PULSE_US,
    DEFAULT_PULSE_US,
))
write_status(uart)
safe_print("DBG_SERVO_READY left_pin=%s right_pin=%s" % (LEFT_ESC_PWM_PIN, RIGHT_ESC_PWM_PIN))

if ENABLE_STANDALONE_FIXED_TEST:
    run_fixed_test(uart, left_pwm, right_pwm)

if ENABLE_STANDALONE_SWEEP_TEST:
    run_sweep_test(uart, left_pwm, right_pwm)

while True:
    poll_uart_commands(uart, left_pwm, right_pwm)
    time.sleep_ms(5)
