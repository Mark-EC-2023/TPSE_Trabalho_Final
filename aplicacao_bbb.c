#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>

// Define GPIO pins for the ultrasonic sensor
#define TRIGGER_PIN "P8_11" // Replace with your BBB GPIO pin
#define ECHO_PIN    "P8_12" // Replace with your BBB GPIO pin

// I2C LCD address
#define LCD_ADDRESS 0x27
#define LCD_WIDTH   16
#define LCD_HEIGHT  2

// Function to initialize GPIO
void setup_gpio(const char *pin, int direction) {
    char path[50];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/direction", pin);
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("Error opening GPIO direction file");
        exit(1);
    }
    fprintf(fp, "%s", direction ? "out" : "in");
    fclose(fp);
}

// Function to read GPIO value
int read_gpio(const char *pin) {
    char path[50];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", pin);
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error opening GPIO value file");
        exit(1);
    }
    int value;
    fscanf(fp, "%d", &value);
    fclose(fp);
    return value;
}

// Function to write GPIO value
void write_gpio(const char *pin, int value) {
    char path[50];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%s/value", pin);
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("Error opening GPIO value file");
        exit(1);
    }
    fprintf(fp, "%d", value);
    fclose(fp);
}

// Function to read ultrasonic sensor distance
long read_ultrasonic_distance(const char *trigger_pin, const char *echo_pin) {
    // Send trigger pulse
    write_gpio(trigger_pin, 0);
    usleep(2);
    write_gpio(trigger_pin, 1);
    usleep(10);
    write_gpio(trigger_pin, 0);

    // Measure echo pulse duration
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (read_gpio(echo_pin) == 0);
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (read_gpio(echo_pin) == 1);
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate duration in microseconds
    long duration = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;
    return duration;
}

// Function to initialize I2C LCD
void lcd_init(int i2c_fd) {
    // Initialize the LCD (specific commands depend on your LCD model)
    // Example: Send initialization commands via I2C
    // This is a placeholder; you'll need to implement the actual commands for your LCD.
}

// Function to write to I2C LCD
void lcd_print(int i2c_fd, const char *text) {
    // Send text to the LCD via I2C
    // This is a placeholder; you'll need to implement the actual I2C communication.
}

int main() {
    // Initialize GPIO pins
    setup_gpio(TRIGGER_PIN, 1); // Trigger pin as output
    setup_gpio(ECHO_PIN, 0);    // Echo pin as input

    // Initialize I2C LCD
    int i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        perror("Error opening I2C device");
        return 1;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, LCD_ADDRESS) < 0) {
        perror("Error setting I2C address");
        return 1;
    }
    lcd_init(i2c_fd);

    // Main loop
    while (1) {
        // Read distance from ultrasonic sensor
        long duration = read_ultrasonic_distance(TRIGGER_PIN, ECHO_PIN);
        int distance = duration * 0.01723; // Convert to cm

        // Display distance on LCD
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "Distance: %d cm", distance);
        lcd_print(i2c_fd, buffer);

        // Control servo (placeholder logic)
        if (distance <= 20) {
            // Rotate servo to 180 degrees (implement PWM control)
        } else {
            // Rotate servo to 0 degrees (implement PWM control)
        }

        // Delay
        usleep(300000); // 300 ms
    }

    // Cleanup
    close(i2c_fd);
    return 0;
}
