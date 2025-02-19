#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>

// Define GPIO pins for the ultrasonic sensor
#define TRIGGER_PIN "68" // P8_11 is GPIO68
#define ECHO_PIN    "69" // P8_12 is GPIO69

// Speed of sound in cm/µs
#define SPEED_OF_SOUND 0.0343

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
float read_ultrasonic_distance() {
    // Send trigger pulse
    write_gpio(TRIGGER_PIN, 0);
    usleep(2);
    write_gpio(TRIGGER_PIN, 1);
    usleep(10);
    write_gpio(TRIGGER_PIN, 0);

    // Measure echo pulse duration
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (read_gpio(ECHO_PIN) == 0);
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (read_gpio(ECHO_PIN) == 1);
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate duration in microseconds
    long duration = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1e3;

    // Calculate distance in centimeters
    float distance = duration * SPEED_OF_SOUND / 2;
    return distance;
}

int main() {
    // Export GPIO pins
    FILE *export = fopen("/sys/class/gpio/export", "w");
    if (export == NULL) {
        perror("Failed to export GPIO");
        return 1;
    }
    fprintf(export, "%s", TRIGGER_PIN);
    fprintf(export, "%s", ECHO_PIN);
    fclose(export);

    // Initialize GPIO pins
    setup_gpio(TRIGGER_PIN, 1); // Trigger pin as output
    setup_gpio(ECHO_PIN, 0);    // Echo pin as input

    // Main loop
    while (1) {
        // Read distance from ultrasonic sensor
        float distance = read_ultrasonic_distance();

        // Display distance on console
        printf("Distance: %.2f cm\n", distance);

        // Delay
        usleep(500000); // 500 ms
    }

    return 0;
}
